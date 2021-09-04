#include "OgreRoot.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreStagingTexture.h"
#include "OgreShaderParams.h"
#include "Threading\MessageQueueSystem.h"
#include "FieldComputeSystem.h"
#include "OgreAsyncTextureTicket.h"
#include "OgreEntity.h"
#include "OgreHlmsUnlitDatablock.h"
#include "Compositor\OgreCompositorWorkspace.h"
#include "Vao\OgreUavBufferPacked.h"
#include "Hand.h"
#include <vector>

namespace MyThirdOgre 
{
	FieldComputeSystem::FieldComputeSystem(Ogre::uint32 id, const MovableObjectDefinition* moDefinition,
		Ogre::SceneMemoryMgrTypes type, GameEntityManager* geMgr) : GameEntity(id, moDefinition, type)
	{
		mGraphicsSystem = 0;

		mTextureType = Ogre::TextureTypes::Type2D;
		mPixelFormat = Ogre::PixelFormatGpu::PFG_RGBA8_UNORM;

		mUavBuffers = new Ogre::UavBufferPackedVec({});

		mPlaneEntity = 0;

		mPlaneMoDef = new MovableObjectDefinition();
		mPlaneMoDef->moType = MoTypeDynamicTriangleList;
		mPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

		mDebugPlaneMoDef = new MovableObjectDefinition();
		mDebugPlaneMoDef->moType = MoTypeDynamicTriangleList;
		mDebugPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
		/*
		mBufferResolutionWidth = 1024.0f;
		mBufferResolutionHeight = 1024.0f;
		mBufferResolutionDepth = 1.0f;

		mFieldWidth = 64.0f;
		mFieldHeight = 64.0f;
		mLeafResolutionX = 0.5f;
		mLeafResolutionZ = 0.5f;*/

		mBufferResolutionWidth = 256.0f;
		mBufferResolutionHeight = 256.0f;

		mBufferResolutionDepth = 1.0f;

		mFieldWidth = 64.0f;
		mFieldHeight = 64.0f;

		// When I say "resolution" I mean "how many of the buffer indices are inside a leaf"
		mLeafResolutionX = 16.0f;
		mLeafResolutionZ = 16.0f;

		// This means that leaf width and height must be calculated, not predefined - they depend upon the width of the plane used to display 
		// the buffer.

		mLeafCountX = mBufferResolutionWidth / mLeafResolutionX;
		mLeafWidth = mFieldWidth / mLeafCountX;

		mLeafCountZ = mBufferResolutionHeight / mLeafResolutionZ;
		mLeafHeight = mFieldHeight / mLeafCountZ;
		
		// Riiiight... so how many times do I need to split my field up in order to achieve mLeafCountX & mLeafCountZ 
		// leaves?

		// mBufferResolutionWidth = 512
		// /2 = 256
		// /2 = 128
		// /2 = 64
		// /2 = 32
		// /2 = 16
		// /2 = 8

		// 6 times

		mOtherBuffer = std::vector<Ogre::ColourValue>({});

		for (auto i = 0; i < mBufferResolutionWidth * mBufferResolutionHeight; i++) {
			mOtherBuffer.push_back(Ogre::ColourValue(0.0f, 0.0f, 0.0f, 1.0f));
		}
			
		mDrawFromUavBufferMat = Ogre::MaterialPtr();

		mDeinitialised = false;

		mGameEntityManager = geMgr;

		mDownloadingTextureViaTicket = false;

		mHaveSetShaderParamsOnce = false;

		mTimeAccumulator = 0.0f;

		mHand = 0;

		mDebugFieldBoundingHierarchy = false;

		mFieldBoundingHierarchy = std::vector<FieldComputeSystem_BoundingHierarchyBox>({});

		mLeaves = std::map<CellCoord, FieldComputeSystem_BoundingHierarchyBox*>({});

		
		mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
			(mBufferResolutionWidth * mBufferResolutionHeight) * sizeof(Ogre::ColourValue), Ogre::MEMCATEGORY_GENERAL));

		mInstanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

		mInstanceBufferStart = mInstanceBuffer;

		memset(mInstanceBuffer, 0, (mBufferResolutionWidth * mBufferResolutionHeight * sizeof(Ogre::ColourValue)) -
			(static_cast<size_t>(mInstanceBuffer - mInstanceBufferStart) * sizeof(float)));
	}

	FieldComputeSystem::~FieldComputeSystem() 
	{
		if (mPlaneMoDef) {
			delete mPlaneMoDef;
			mPlaneMoDef = 0;
		}

		if (mDebugPlaneMoDef) {
			delete mDebugPlaneMoDef;
			mDebugPlaneMoDef = 0;
		}

		for (auto i = mLeaves.begin(); i != mLeaves.end(); i++) {
			delete i->second;
			i->second = 0;
		}

		deinitialise();
	}

	void FieldComputeSystem::initialise()
	{
		Ogre::Quaternion qRot = Ogre::Quaternion::IDENTITY;

		qRot.FromAngleAxis(Ogre::Radian(Ogre::Degree(-90)), Ogre::Vector3::UNIT_X);

		mPlaneEntity = mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
			Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
			mPlaneMoDef,
			std::vector<Ogre::Vector3>({
				Ogre::Vector3(-(mFieldWidth / 2), 0, mFieldHeight / 2),
				Ogre::Vector3(mFieldWidth / 2, 0, mFieldHeight / 2),
				Ogre::Vector3(mFieldWidth / 2, 0, -(mFieldHeight / 2)),
				Ogre::Vector3(-(mFieldWidth / 2), 0, -(mFieldHeight / 2))
			}),
			Ogre::BLANKSTRING,
			Ogre::Vector3::ZERO,
			Ogre::Quaternion::IDENTITY,
			Ogre::Vector3::UNIT_SCALE,
			true,
			1.0f,
			true,
			Ogre::Vector3::ZERO,
			mRenderTargetTexture);

		// this works nicely because the message wrapper retains the pointer
		// the object is destroyed correctly on the graphics thread
		this->mGameEntityManager->mLogicSystem->queueSendMessage(
			this->mGameEntityManager->mGraphicsSystem, 
			Mq::REMOVE_STAGING_TEXTURE, 
			mLeapMotionStagingTexture);
		// it's safe - and probably best - to set the pointer to 0 here
		// even if the graphics thread hasn't yet destroyed the object
		// because it will prevent us from messing with it further here when it was 
		// already marked for destruction
		mLeapMotionStagingTexture = 0;

		createBoundingHierarchy();
	}

	void FieldComputeSystem::createBoundingHierarchy(void)
	{
		float xRes = mFieldWidth / 2;
		float zRes = mFieldHeight / 2;

		float depthCount = 0.5f;

		mFieldBoundingHierarchy.push_back(FieldComputeSystem_BoundingHierarchyBox(
			Ogre::Vector3(0, 0, 0),
			Ogre::Vector3(xRes, 0, zRes)
		));

		if (mDebugFieldBoundingHierarchy) {
			mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
				Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
				mDebugPlaneMoDef,
				std::vector<Ogre::Vector3>({
					Ogre::Vector3(-xRes, 0.0f, zRes),
					Ogre::Vector3(xRes, 0.0f, zRes),
					Ogre::Vector3(xRes, 0.0f, -zRes),
					Ogre::Vector3(-xRes, 0.0f, -zRes),
					}),
					"White",
					Ogre::Vector3(0, depthCount, 0),
					Ogre::Quaternion::IDENTITY,
					//Ogre::Vector3(0.05f, 0.05f, 0.05f),
					Ogre::Vector3::UNIT_SCALE,
					true,
					0.35f);
		}

		int leafIndexX = 0;
		int leafIndexZ = 0;

		float offsetX = mLeafWidth / mLeafResolutionX;
		float offsetZ = mLeafHeight / mLeafResolutionZ;

		float halfOffsetX = offsetX * 0.5f;
		float halfOffsetZ = offsetZ * 0.5f;
		
		Ogre::Vector3 leafHalfWidths = Ogre::Vector3(mLeafWidth / 2, 0, mLeafHeight / 2);

		for (float z = 0; z < mLeafCountZ * mLeafWidth; z += mLeafWidth) 
		{
			leafIndexX = 0;

			for (float x = 0; x < mLeafCountX * mLeafHeight; x += mLeafHeight) 
			{
				Ogre::Vector3 leafCenter = Ogre::Vector3(
					(x + leafHalfWidths.x + ((mFieldWidth / 2) - mFieldWidth)),
					0,
					(z + leafHalfWidths.z + ((mFieldHeight / 2) - mFieldHeight))
				);

				FieldComputeSystem_BoundingHierarchyBox* box = new FieldComputeSystem_BoundingHierarchyBox(
					leafCenter,
					leafHalfWidths
				);

				int offset = leafIndexX * mLeafResolutionX + (leafIndexZ * mLeafResolutionZ * mBufferResolutionWidth);

				auto corner = box->mAaBb.getCorner(Ogre::AxisAlignedBox::FAR_LEFT_BOTTOM);

				for (size_t j = 0; j < mLeafResolutionZ; j++) {
					for (size_t i = 0; i < mLeafResolutionX; i++) {
						box->mBufferIndices.push_back(FieldComputeSystem_BufferIndexPosition(
							offset + j + i,
							Ogre::Vector3(
								corner.x + (offsetX * i) + halfOffsetX,
								0.0f,
								corner.z + (offsetZ * j) + halfOffsetZ
							),	// HERE
							&mTransform[0]->vPos));
					}

					offset += (mBufferResolutionWidth - 1);
				}

				mLeaves.insert({ CellCoord(leafIndexX++, 0, leafIndexZ), box });
			}

			leafIndexZ++;
		}

		subdivideBoundingHierarchy(xRes, zRes, mFieldBoundingHierarchy[0], leafIndexX, leafIndexZ, depthCount);
	}

	void FieldComputeSystem::subdivideBoundingHierarchy(
		float xRes,
		float zRes,
		FieldComputeSystem_BoundingHierarchyBox& box, 
		int& leafIndexX,
		int& leafIndexZ,
		float& depthCount)
	{
		/*if (depthCount > 20)
			return;*/

		Ogre::Quaternion qRot = Ogre::Quaternion::IDENTITY;

		depthCount += 0.2f;

		if (xRes > mLeafWidth && zRes > mLeafHeight) {

			Ogre::Vector3 hw = Ogre::Vector3(box.mHalfWidths.x, box.mHalfWidths.y, box.mHalfWidths.z) / 2; // these SHOULD be quarter-widths:
																										// as we're dividing space in quads

			auto a = FieldComputeSystem_BoundingHierarchyBox(
						Ogre::Vector3(box.mCenter.x + -hw.x, hw.y, box.mCenter.z + hw.z), 
						hw);

			auto b = FieldComputeSystem_BoundingHierarchyBox(
						Ogre::Vector3(box.mCenter.x + hw.x, hw.y, box.mCenter.z + hw.z), 
						hw);

			auto c = FieldComputeSystem_BoundingHierarchyBox(
						Ogre::Vector3(box.mCenter.x + hw.x, hw.y, box.mCenter.z + -hw.z), 
						hw);

			auto d = FieldComputeSystem_BoundingHierarchyBox(
						Ogre::Vector3(box.mCenter.x + -hw.x, hw.y, box.mCenter.z + -hw.z), 
						hw);

			if (mDebugFieldBoundingHierarchy) {

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
							Ogre::Vector3(-a.mHalfWidths.x, 0.0f, a.mHalfWidths.z),
							Ogre::Vector3(a.mHalfWidths.x, 0.0f, a.mHalfWidths.z),
							Ogre::Vector3(a.mHalfWidths.x, 0.0f, -a.mHalfWidths.z),
							Ogre::Vector3(-a.mHalfWidths.x, 0.0f, -a.mHalfWidths.z),
						}),
						"White",
						a.mCenter + Ogre::Vector3(0, depthCount, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						true,
						0.35f);

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
							Ogre::Vector3(-b.mHalfWidths.x, 0.0f, b.mHalfWidths.z),
							Ogre::Vector3(b.mHalfWidths.x, 0.0f, b.mHalfWidths.z),
							Ogre::Vector3(b.mHalfWidths.x, 0.0f, -b.mHalfWidths.z),
							Ogre::Vector3(-b.mHalfWidths.x, 0.0f, -b.mHalfWidths.z),
						}),
						"White",
						b.mCenter + Ogre::Vector3(0, depthCount, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						true,
						0.35f);

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
							Ogre::Vector3(-c.mHalfWidths.x, 0.0f, c.mHalfWidths.z),
							Ogre::Vector3(c.mHalfWidths.x, 0.0f, c.mHalfWidths.z),
							Ogre::Vector3(c.mHalfWidths.x, 0.0f, -c.mHalfWidths.z),
							Ogre::Vector3(-c.mHalfWidths.x, 0.0f, -c.mHalfWidths.z),
						}),
						"White",
						c.mCenter + Ogre::Vector3(0, depthCount, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						true,
						0.35f);

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
							Ogre::Vector3(-d.mHalfWidths.x, 0.0f, d.mHalfWidths.z),
							Ogre::Vector3(d.mHalfWidths.x, 0.0f, d.mHalfWidths.z),
							Ogre::Vector3(d.mHalfWidths.x, 0.0f, -d.mHalfWidths.z),
							Ogre::Vector3(-d.mHalfWidths.x, 0.0f, -d.mHalfWidths.z),
						}),
						"White",
						d.mCenter + Ogre::Vector3(0, depthCount, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						true,
						0.35f);
			}

			box.mChildren.push_back(a);
			box.mChildren.push_back(b);
			box.mChildren.push_back(c);
			box.mChildren.push_back(d);
		}

		xRes /= 2;
		zRes /= 2;

		for (auto& iter : box.mChildren) {
			subdivideBoundingHierarchy(xRes, zRes, iter, leafIndexX, leafIndexZ, depthCount);
		}

		if (box.mChildren.size() == 0) {

			for (auto& iter : mLeaves) {
				if (box.mAaBb.intersects(iter.second->mAaBb))
					box.mChildren.push_back(*iter.second);
			}

			if (mDebugFieldBoundingHierarchy) {
				box.mLeafEntity = mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
							Ogre::Vector3(-box.mHalfWidths.x, 0.0f, box.mHalfWidths.z),
							Ogre::Vector3(box.mHalfWidths.x, 0.0f, box.mHalfWidths.z),
							Ogre::Vector3(box.mHalfWidths.x, 0.0f, -box.mHalfWidths.z),
							Ogre::Vector3(-box.mHalfWidths.x, 0.0f, -box.mHalfWidths.z),
						}),
						"Blue",
						box.mCenter + Ogre::Vector3(0, 1.0f, 0),
						qRot,
						Ogre::Vector3::UNIT_SCALE,
						true,
						0.25f,
						mDebugFieldBoundingHierarchy);
			}

			box.mIsLeaf = true;
			box.mLeafIndexX = leafIndexX;
			box.mLeafIndexZ = leafIndexZ;

			/*leafIndexX++;

			if (leafIndexX > mLeafCountX - 1) {
				leafIndexZ++;
				leafIndexX = 0;
			}

			mLeaves.insert({ CellCoord(box.mLeafIndexX, 0, box.mLeafIndexZ), &box });*/
		}
	}

	//////
	/*

		

	*/
	void FieldComputeSystem::buildBoundingDivisionIntersections(const size_t i, FieldComputeSystem_BoundingHierarchyBox& box)
	{
		// determine the position in space of this point in my buffer.

		// This is what's causing the fuckuppery, isn't it. 
		// I'm dictating that the buffer resolution has a direct correlation to positions in 3D space: this is not true.
		// This must be scaled by the actual plane dimensions.

		Ogre::Vector3 pos = Ogre::Vector3(
			(i % (int)mBufferResolutionHeight - (mBufferResolutionHeight / 2)),
			0,
			(i / (int)mBufferResolutionWidth - (mBufferResolutionWidth / 2)));

		if (box.mAaBb.intersects(pos)) {
			if (box.mChildren.size()) {
				for (auto& iter : box.mChildren)
					buildBoundingDivisionIntersections(i, iter);
			}
			else {
				//box.mBufferIndices.push_back(i);
			}
		}
	}

	void FieldComputeSystem::_notifyGraphicsSystem(GraphicsSystem* gs) 
	{
		mGraphicsSystem = gs;
	}
	
	void FieldComputeSystem::_notifyStagingTextureRemoved(const FieldComputeSystem_StagingTextureMessage* msg) 
	{
		if (msg->mStagingTexture == mLeapMotionStagingTexture) 
		{
			mLeapMotionStagingTexture = 0;
		}
	} 

	void FieldComputeSystem::deinitialise(void) 
	{
		if (!mDeinitialised)
		{
			mDrawFromUavBufferMat.setNull();

			if (mComputeJob) {
				mComputeJob->clearUavBuffers();
				mComputeJob->clearTexBuffers();
			}

			//if (mLeapMotionStagingTexture) 
			//{
			//	//mStagingTexture->upload
			//}

			if (mRenderTargetTexture) 
			{
				auto listeners = mRenderTargetTexture->getListeners();

				if (listeners.size()) {
					mRenderTargetTexture->removeListener(mComputeJob);

					// It would seem that we do not need to remove HLMSUnlitDatablock listeners.
					// These are probably being handled higher up the chain by the game entity destructors.
					/*mTextures[FieldComputeSystemTexture::Velocity]->removeListener(static_cast<Ogre::HlmsUnlitDatablock*>(static_cast<Ogre::v1::Entity*>(mPlaneEntity->mMovableObject)->getSubEntity(0)->getDatablock()));*/
				}
			}

			//if (mTextures[FieldComputeSystemTexture::LeapMotion])
			//{
			//	auto listeners = mTextures[FieldComputeSystemTexture::LeapMotion]->getListeners();

			//	if (listeners.size()) {
			//		// it would seem that this second texture, if "unused" in some capacity, 
			//		// does not actually need to be removed here.

			//		//mTextures[FieldComputeSystemTexture::LeapMotion]->removeListener(mComputeJob);
			//	}
			//}

			delete mUavBuffers;
			mUavBuffers = 0;

			mDeinitialised = true;
		}
	}

	void FieldComputeSystem::addUavBuffer(Ogre::UavBufferPacked* buffer) 
	{
		mUavBuffers->push_back(buffer);
	}

	void FieldComputeSystem::setComputeJob(Ogre::HlmsComputeJob* job)
	{
		mComputeJob = job;
	}

	void FieldComputeSystem::setMaterial(Ogre::MaterialPtr mat)
	{
		mDrawFromUavBufferMat = mat;
	}

	void FieldComputeSystem::setTexture(Ogre::TextureGpu* texture)
	{
		mRenderTargetTexture = texture;
	}

	void FieldComputeSystem::setLeapMotionStagingTexture(Ogre::StagingTexture* sTex) 
	{
		mLeapMotionStagingTexture = sTex;
	}

	void FieldComputeSystem::setAsyncTextureTicket(Ogre::AsyncTextureTicket* texTicket) 
	{
		mTextureTicket = texTicket;
	}

	void FieldComputeSystem::update(float timeSinceLast)
	{
		mTimeAccumulator += timeSinceLast;

		if (mComputeJob)
		{
			if (!mHaveSetShaderParamsOnce) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				//Update the compute shader's
				Ogre::ShaderParams& shaderParams = mComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				auto pb = shaderParams.findParameter("pixelBuffer");
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				shaderParams.setDirty();

				auto tpgx = mComputeJob->getThreadsPerGroupX();
				auto tpgy = mComputeJob->getThreadsPerGroupY();
				auto tpgz = mComputeJob->getThreadsPerGroupZ();

				mComputeJob->setNumThreadGroups(
					(res[0] + mComputeJob->getThreadsPerGroupX() - 1u) / mComputeJob->getThreadsPerGroupX(),
					(res[1] + mComputeJob->getThreadsPerGroupY() - 1u) / mComputeJob->getThreadsPerGroupY(), 
					1u);

				////Update the pass that draws the UAV Buffer into the RTT (we could
				////use auto param viewport_size, but this more flexible)
				Ogre::GpuProgramParametersSharedPtr psParams = mDrawFromUavBufferMat->getTechnique(0)->
					getPass(0)->getFragmentProgramParameters();

				psParams->setNamedConstant("texResolution", res, 1u, 2u);

				mHaveSetShaderParamsOnce = true;

			}

			//if (mDownloadingTextureViaTicket && mTextureTicket->queryIsTransferDone()) 
			//{
			//	mDownloadingTextureViaTicket = false;

			//	for (int i = 0; i < mTextureTicket->getNumSlices(); i++) {
			//		auto texBox = mTextureTicket->map(i);
			//		auto colourValue = texBox.getColourAt(212, 230, 0, mPixelFormat);
			//		int f = 0;
			//		// Yay! I can read a texture which was written BY THE GPU!!!
			//	}
			//	mTextureTicket->unmap();
			//}
			
			auto c = Ogre::ColourValue(0.0f, 0.0f, 0.0f, 1.0f);

			/*for (auto& leaf : mLeaves) {
				if (leaf.second->mLeafVisible) {
					leaf.second->mLeafVisible = false;
					mGameEntityManager->toggleGameEntityVisibility(leaf.second->mLeafEntity, false);
				}
			}*/

			for (auto& iter : mOtherBuffer) {
				if (iter.b > 0)
					iter.b *= 0.996f;
			}

			if (mHand && mFieldBoundingHierarchy.size())
			{
				auto leaves = std::vector<FieldComputeSystem_BoundingHierarchyBox>({});

				int aabbIntersectionCount = 0;

				traverseBoundingHierarchy(mFieldBoundingHierarchy[0], &leaves, aabbIntersectionCount);

				if (mGraphicsSystem) {
					mGraphicsSystem->setAdditionalDebugText(
						Ogre::String("\nIntersecting: ") + Ogre::StringConverter::toString(aabbIntersectionCount) + Ogre::String(" Leaf AaBbs"));
				}

				if (leaves.size()) {

					int f = 0;

					if (mGraphicsSystem) {
						mGraphicsSystem->setAdditionalDebugText(
							Ogre::String("\nIntersecting: ") + Ogre::StringConverter::toString(aabbIntersectionCount) + Ogre::String(" Leaf AaBbs") +
							Ogre::String("\nSeeing: " + Ogre::StringConverter::toString(leaves.size() * mLeafResolutionX * mLeafResolutionZ) + Ogre::String(" buffer indices")));
					}

					//auto buffer = getUavBuffer(0);
					//size_t uavBufferNumElements = buffer->getNumElements();

					Ogre::Real rHandSphereSquared = mHand->getBoundingSphere()->getRadius() * mHand->getBoundingSphere()->getRadius();

					Ogre::Vector3 vHandPos = mHand->getBoundingSphere()->getCenter();

					for (const auto& l : leaves) {

						float x = l.mAaBb.getMinimum().x;
						float z = l.mAaBb.getMinimum().z;

						float zOffset = 0;
						float xOffset = 0;

						for (size_t i = 0; i < l.mBufferIndices.size(); i++) {

							auto& index = l.mBufferIndices[i];

							float xPos = x + xOffset;
							float yPos = 0;
							float zPos = z + zOffset;

							xOffset += 1 / mLeafResolutionX;

							if (xOffset > mLeafResolutionX) {
								xOffset = 0;
								zOffset += 1 / mLeafResolutionZ;
							}

							Ogre::Vector3 pos = index.mPosition;// Ogre::Vector3(xPos, yPos, zPos);
							Ogre::Vector3 dist = vHandPos - pos;
							Ogre::Real distLen = dist.length();

							if (distLen < rHandSphereSquared) {

								mOtherBuffer[index.mIndex].r = 0.0f;
								mOtherBuffer[index.mIndex].g = 0.0f;
								mOtherBuffer[index.mIndex].b = 0.34f;
								mOtherBuffer[index.mIndex].a = 1.0f;

								//// intersecting

								////// thanks https://forums.ogre3d.org/viewtopic.php?t=96286
								//auto mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
								//	1 * sizeof(Ogre::ColourValue), Ogre::MEMCATEGORY_GENERAL));

								//float* RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

								//const float* instanceBufferStart = instanceBuffer;

								//*instanceBuffer++ = c.r;
								//*instanceBuffer++ = c.g;
								//*instanceBuffer++ = 0.34f;
								//*instanceBuffer++ = c.a;

								//getUavBuffer(0)->upload(mCpuInstanceBuffer, index.mIndex, 1);
							}
							//else {

							//	mOtherBuffer[index.mIndex].r = 0.0f;
							//	mOtherBuffer[index.mIndex].g = 0.0f;
							//	mOtherBuffer[index.mIndex].b = 0.0f;
							//	mOtherBuffer[index.mIndex].a = 1.0f;


							//	// not intersecting
							//	//auto mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
							//	//	1 * sizeof(Ogre::ColourValue), Ogre::MEMCATEGORY_GENERAL));

							//	//float* RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

							//	//const float* instanceBufferStart = instanceBuffer;

							//	//*instanceBuffer++ = c.r;
							//	//*instanceBuffer++ = c.g;
							//	//*instanceBuffer++ = c.b;
							//	//*instanceBuffer++ = c.a;// (float)sin(mTimeAccumulator);
							//	//getUavBuffer(0)->upload(mCpuInstanceBuffer, index.mIndex, 1);
							//}
						}
					}
				}
			}




			auto buffer = getUavBuffer(0);
			auto uavBufferNumElements = buffer->getNumElements();
			
			auto s = sizeof(size_t);
			auto f = sizeof(float);
			auto co = sizeof(Ogre::ColourValue);

			float* instanceBuffer = const_cast<float*>(mInstanceBufferStart);

			for (const auto &iter : mOtherBuffer) {
			
				*instanceBuffer++ = iter.r;
				*instanceBuffer++ = iter.g;
				*instanceBuffer++ = iter.b;
				*instanceBuffer++ = iter.a;// (float)sin(mTimeAccumulator);
			}

			auto tsb = buffer->getTotalSizeBytes();
			auto calc = (size_t)(instanceBuffer - mInstanceBufferStart) * sizeof(float);

			OGRE_ASSERT_LOW(calc <= tsb);

			buffer->upload(mCpuInstanceBuffer, 0u, buffer->getNumElements());
		}
	}

	void FieldComputeSystem::traverseBoundingHierarchy(
		const FieldComputeSystem_BoundingHierarchyBox& level, 
		const std::vector<FieldComputeSystem_BoundingHierarchyBox>* leaves,
		int& aabbIntersectionCount)
	{
		//if (mHand->getBoundingSphere()->intersects(level.mAaBb)) {
		if (level.mAaBb.intersects(*mHand->getBoundingSphere())) {
			if (level.mChildren.size()) {
				for (const auto& iter : level.mChildren)
					traverseBoundingHierarchy(iter, leaves, aabbIntersectionCount);
			}
			else {
				aabbIntersectionCount++;
				if (level.mLeafEntity) {
					const_cast<FieldComputeSystem_BoundingHierarchyBox&>(level).mLeafVisible = true;
					mGameEntityManager->toggleGameEntityVisibility(level.mLeafEntity, true);
				}

				const_cast<std::vector<FieldComputeSystem_BoundingHierarchyBox>*>(leaves)->push_back(level);
			}
		}
	}

	void FieldComputeSystem::writeDebugImages(float timeSinceLast) 
	{
		if (mComputeJob) 
		{
			mGameEntityManager->mLogicSystem->queueSendMessage(
				mGameEntityManager->mGraphicsSystem,
				Mq::FIELD_COMPUTE_SYSTEM_WRITE_FILE_TESTING,
				NULL);
		}
	}
}