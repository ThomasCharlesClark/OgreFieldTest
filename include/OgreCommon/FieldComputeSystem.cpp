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

		mBufferResolutionWidth = 512.0f;
		mBufferResolutionHeight = 512.0f;

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

		mDrawFromUavBufferMat = Ogre::MaterialPtr();

		mDeinitialised = false;

		mGameEntityManager = geMgr;

		mDownloadingTextureViaTicket = false;

		mHaveSetShaderParamsOnce = false;

		mTimeAccumulator = 0.0f;

		mHand = 0;

		mDebugFieldBoundingHierarchy = false;
		mFieldBoundingHierarchy = std::vector<FieldComputeSystem_BoundingHierarchyBox>({});
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
		float xRes = mFieldWidth;
		float zRes = mFieldHeight;

		int depthCount = 2;

		mFieldBoundingHierarchy.push_back(FieldComputeSystem_BoundingHierarchyBox(
			Ogre::Vector3(0, 0, 0),
			Ogre::Vector3(mFieldWidth / 2, 0, mFieldHeight / 2)
		));

		int leafIndexX = 0;
		int leafIndexZ = 0;

		auto leaves = std::vector<FieldComputeSystem_BoundingHierarchyBox*>({});

		subdivideBoundingHierarchy(xRes, zRes, mFieldBoundingHierarchy[0], leafIndexX, leafIndexZ, leaves, depthCount);

		for (auto& leaf : leaves) {

			int offsetX = leaf->mAaBb.getMinimum().x;
			int offsetZ = leaf->mAaBb.getMinimum().z;

			int offset = leaf->mLeafIndexX * mLeafResolutionX + (leaf->mLeafIndexZ * mLeafResolutionZ * mBufferResolutionWidth);
			
			for (size_t i = 0; i < mLeafResolutionX; i++) {
				for (size_t j = 0; j < mLeafResolutionZ; j++) {
					leaf->mBufferIndices.push_back(offset + j + i);
				}

				offset += (mBufferResolutionWidth - 1);
			}
		}

		int f = 0;

		//for (size_t i = 0; i < (mBufferResolutionWidth * mBufferResolutionHeight); ++i)
		//for (size_t i = 0; i < (mBufferResolutionWidth * mBufferResolutionHeight); ++i)
		//{
		//	// for each of the data points in my buffer...
		//	buildBoundingDivisionIntersections(i, mFieldBoundingHierarchy[0]);
		//}

		
			//// we know we're a leaf at this point... can we KNOW what indices we will have, without having to iterate...
			//// over more than a million points, testing each of them...?

			//// how many will it contain
			//// well we know how wide the whole thing is, and how many that infers total


			//int offsetX = box.mAaBb.getMinimum().x;
			//int offsetZ = box.mAaBb.getMinimum().z;

			//
			//


			//// You've got a 1D array. You want to know what sections of that 1D array this particular "leaf" contains.
			////
			////		 leafIndexX adds an offset of N * containsX 
			////		 _______ _______ _______
			////		|		|		|		|
			////		|	A	|	B	|	C	|
			////		|_______|_______|_______|
			////		|		|		|		|
			////		|	D	|	E	|	F	| leafIndexZ adds an offset of N * containsZ * mBufferResolutionHeight
			////		|_______|_______|_______|
			////		|		|		|		|
			////		|		|		|		|
			////	
			////

			//int offset = leafIndexX * containsX + (leafIndexZ * containsZ * mBufferResolutionWidth);
			//
			//for (size_t i = 0; i < containsX; i++) {
			//	for (size_t j = 0; j < containsZ; j++) {
			//		box.mBufferIndices.push_back(offset + j + i);
			//	}

			//	offset += (mBufferResolutionWidth - 1);
			//}
	}

	void FieldComputeSystem::subdivideBoundingHierarchy(
		float xRes,
		float zRes,
		FieldComputeSystem_BoundingHierarchyBox& box, 
		int& leafIndexX,
		int& leafIndexZ,
		std::vector<FieldComputeSystem_BoundingHierarchyBox*>& leaves,
		int& depthCount)
	{
		Ogre::Quaternion qRot = Ogre::Quaternion::IDENTITY;

		xRes /= 2;
		zRes /= 2;

		if (xRes >= mLeafWidth && zRes >= mLeafHeight) {

			Ogre::Vector3 hw = Ogre::Vector3(box.mHalfWidths.x, box.mHalfWidths.y, box.mHalfWidths.z) / 2;

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
						a.mCenter + Ogre::Vector3(-(a.mHalfWidths.x * 2), 0.0f, (a.mHalfWidths.z * 2)),
						a.mCenter + Ogre::Vector3((a.mHalfWidths.x * 2), 0.0f, (a.mHalfWidths.z * 2)),
						a.mCenter + Ogre::Vector3((a.mHalfWidths.x * 2), 0.0f, -(a.mHalfWidths.z * 2)),
						a.mCenter + Ogre::Vector3(-(a.mHalfWidths.x * 2), 0.0f, -(a.mHalfWidths.z * 2)),
						}),
						"Green",
						a.mCenter + Ogre::Vector3(0, depthCount++, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						false,
						0.35f);

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
						b.mCenter + Ogre::Vector3(-(b.mHalfWidths.x * 2), 0.0f, (b.mHalfWidths.z * 2)),
						b.mCenter + Ogre::Vector3((b.mHalfWidths.x * 2), 0.0f, (b.mHalfWidths.z * 2)),
						b.mCenter + Ogre::Vector3((b.mHalfWidths.x * 2), 0.0f, -(b.mHalfWidths.z * 2)),
						b.mCenter + Ogre::Vector3(-(b.mHalfWidths.x * 2), 0.0f, -(b.mHalfWidths.z * 2)),
						}),
						"Green",
						b.mCenter + Ogre::Vector3(0, depthCount++, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						false,
						0.35f);

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
						c.mCenter + Ogre::Vector3(-(c.mHalfWidths.x * 2), 0.0f, (c.mHalfWidths.z * 2)),
						c.mCenter + Ogre::Vector3((c.mHalfWidths.x * 2), 0.0f, (c.mHalfWidths.z * 2)),
						c.mCenter + Ogre::Vector3((c.mHalfWidths.x * 2), 0.0f, -(c.mHalfWidths.z * 2)),
						c.mCenter + Ogre::Vector3(-(c.mHalfWidths.x * 2), 0.0f, -(c.mHalfWidths.z * 2)),
						}),
						"Green",
						c.mCenter + Ogre::Vector3(0, depthCount++, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						false,
						0.35f);

				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
						d.mCenter + Ogre::Vector3(-(d.mHalfWidths.x * 2), 0.0f, (d.mHalfWidths.z * 2)),
						d.mCenter + Ogre::Vector3((d.mHalfWidths.x * 2), 0.0f, (d.mHalfWidths.z * 2)),
						d.mCenter + Ogre::Vector3((d.mHalfWidths.x * 2), 0.0f, -(d.mHalfWidths.z * 2)),
						d.mCenter + Ogre::Vector3(-(d.mHalfWidths.x * 2), 0.0f, -(d.mHalfWidths.z * 2)),
						}),
						"Green",
						d.mCenter + Ogre::Vector3(0, depthCount++, 0),
						qRot,
						//Ogre::Vector3(0.05f, 0.05f, 0.05f),
						Ogre::Vector3::UNIT_SCALE,
						false,
						0.35f);
			}

			box.mChildren.push_back(a);
			box.mChildren.push_back(b);
			box.mChildren.push_back(c);
			box.mChildren.push_back(d);
		}

		for (auto& iter : box.mChildren) {
			subdivideBoundingHierarchy(xRes, zRes, iter, leafIndexX, leafIndexZ, leaves, depthCount);
		}

		if (box.mChildren.size() == 0) {

			depthCount++;

			if (mDebugFieldBoundingHierarchy) {
				mGameEntityManager->addGameEntity(Ogre::BLANKSTRING,
					Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
					mDebugPlaneMoDef,
					std::vector<Ogre::Vector3>({
						box.mCenter + Ogre::Vector3(-(box.mHalfWidths.x * 2), 0.0f, (box.mHalfWidths.z * 2)),
						box.mCenter + Ogre::Vector3((box.mHalfWidths.x * 2), 0.0f, (box.mHalfWidths.z * 2)),
						box.mCenter + Ogre::Vector3((box.mHalfWidths.x * 2), 0.0f, -(box.mHalfWidths.z * 2)),
						box.mCenter + Ogre::Vector3(-(box.mHalfWidths.x * 2), 0.0f, -(box.mHalfWidths.z * 2)),
						}),
						"Blue",
						box.mCenter + Ogre::Vector3(0, 0, 0),
						qRot,
						Ogre::Vector3::UNIT_SCALE,
						true,
						0.25f,
						true);
			}

			box.mIsLeaf = true;
			box.mLeafIndexX = leafIndexX;
			box.mLeafIndexZ = leafIndexZ;

			leafIndexX++;

			if (leafIndexX > mLeafCountX - 1) {
				leafIndexZ++;
				leafIndexX = 0;
			}

			leaves.push_back(&box);
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
			


			// bye-bye, performance!
			// so what I need here is nested bounding boxes, with the final leaves "owning" chunks of the 
			// complete index set
			// how do I know what leaf owns which indices? either by predetermination or by an initial sweep test
			// predetermination should absolutely be possible...
			// let's say I want no more than 16 by 16 in each leaf

			// how many boxes is that?

			// well, you keep dividing width and height by two until... yeah what is 1024 the power of two of

			// looks like I'll want... a lot of boxes. 64 by 64 is a lot.

			auto c = Ogre::ColourValue(0.0f, 0.0f, 0.24f, 0.0f);

			if (mHand && mFieldBoundingHierarchy.size())
			{
				auto indices = std::vector<size_t>({});

				traverseBoundingHierarchy(mFieldBoundingHierarchy[0], &indices);

				if (indices.size()) {

					//auto buffer = getUavBuffer(0);
					//size_t uavBufferNumElements = buffer->getNumElements();

					Ogre::Real rHandSphereSquared = mHand->getBoundingSphere()->getRadius() * mHand->getBoundingSphere()->getRadius();

					for (const auto& i : indices) {
						float x = i % (int)mBufferResolutionHeight - (mBufferResolutionHeight / 2);
						float y = 0;
						float z = i / (int)mBufferResolutionWidth - (mBufferResolutionWidth / 2);

						Ogre::Vector3 pos = Ogre::Vector3(x, y, z);
						Ogre::Vector3 dist = mHand->getBoundingSphere()->getCenter() - pos;

						if (dist.length() < rHandSphereSquared) {
							// not intersecting

							//// thanks https://forums.ogre3d.org/viewtopic.php?t=96286
							auto mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
								1 * sizeof(Ogre::ColourValue), Ogre::MEMCATEGORY_GENERAL));

							float* RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

							const float* instanceBufferStart = instanceBuffer;

							*instanceBuffer++ = c.r;
							*instanceBuffer++ = c.g;
							*instanceBuffer++ = c.b;
							*instanceBuffer++ = c.a;

							getUavBuffer(0)->upload(mCpuInstanceBuffer, i, 1);
						}
						else {
							// should be intersecting
							auto mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
								1 * sizeof(Ogre::ColourValue), Ogre::MEMCATEGORY_GENERAL));

							float* RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

							const float* instanceBufferStart = instanceBuffer;

							*instanceBuffer++ = c.r;
							*instanceBuffer++ = c.g;
							*instanceBuffer++ = c.b;
							*instanceBuffer++ = 1.0f;// (float)sin(mTimeAccumulator);
							getUavBuffer(0)->upload(mCpuInstanceBuffer, i, 1);
						}
					}
				}
			}


			//auto c = Ogre::ColourValue(0.0f, 0.0f, 0.24f, 0.0f);

			////for (size_t i = 0; i < uavBufferNumElements; ++i) {
			
			////	if (mHand) {

			////		float x = i % (int)mHeight - (mHeight / 2);
			////		float y = 0;
			////		float z = i / (int)mWidth - (mWidth / 2);

			////		Ogre::Vector3 pos = Ogre::Vector3(x, y, z);
			////		Ogre::Vector3 dist = mHand->getBoundingSphere()->getCenter() - pos;

			////		*instanceBuffer++ = c.r;
			////		*instanceBuffer++ = c.g;
			////		*instanceBuffer++ = c.b;

			////		if (dist.length() < rHandSphereSquared) {
			////			// not intersecting
			////			*instanceBuffer++ = c.a;
			////		}
			////		else {
			////			// should be intersecting
			////			*instanceBuffer++ = 1.0f;// (float)sin(mTimeAccumulator);
			////		}

			////	}
			////	else {
			////		if (i % 2 == 0) {
			////			c.b = 0.24f;
			////		}
			////		else {
			////			c.g = 0.30f;
			////		}

			////		*instanceBuffer++ = c.r;
			////		*instanceBuffer++ = c.g;
			////		*instanceBuffer++ = c.b;
			////		*instanceBuffer++ = (float)sin(mTimeAccumulator);// c.a;
			////	}
			////}

			//OGRE_ASSERT_LOW((size_t)(instanceBuffer - instanceBufferStart) * sizeof(float) <=
			//	buffer->getTotalSizeBytes());

			//memset(instanceBuffer, 0, buffer->getTotalSizeBytes() -
			//	(static_cast<size_t>(instanceBuffer - instanceBufferStart) * sizeof(float)));

			//buffer->upload(mCpuInstanceBuffer, 0u, buffer->getNumElements());
		}
	}

	void FieldComputeSystem::traverseBoundingHierarchy(
		const FieldComputeSystem_BoundingHierarchyBox& level, 
		const std::vector<size_t>* indicesList)
	{
		//if (mHand->getBoundingSphere()->intersects(level.mAaBb)) {
		if (level.mAaBb.intersects(*mHand->getBoundingSphere())) {
			if (level.mChildren.size()) {
				for (const auto& iter : level.mChildren)
					traverseBoundingHierarchy(iter, indicesList);
			}
			else {
				for (const auto& iter : level.mBufferIndices)
					const_cast<std::vector<size_t>*>(indicesList)->push_back(iter);
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