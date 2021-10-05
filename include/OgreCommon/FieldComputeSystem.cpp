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
		mTestComputeJob = 0;
		mAdvectionCopyComputeJob = 0;
		mVelocityAdvectionComputeJob = 0;
		mInkAdvectionComputeJob = 0;
		mAddImpulsesComputeJob = 0;
		mDivergenceComputeJob = 0;
		mJacobiPressureComputeJob = 0;
		mSubtractPressureGradientComputeJob = 0;

		mRenderTargetTexture = 0;
		mVelocityTexture = 0;
		mSecondaryVelocityTexture = 0;
		mPressureTexture = 0;
		mPressureGradientTexture = 0;
		mDivergenceTexture = 0;
		mInkTexture = 0;
		mSecondaryInkTexture = 0;

		mVelocityStagingTexture = 0;
		mInkStagingTexture = 0;

		mGraphicsSystem = 0;

		mTextureTicket2D = 0;
		mTextureTicket3D = 0;

		mTextureType2D = Ogre::TextureTypes::Type2D;
		mTextureType3D = Ogre::TextureTypes::Type3D;

		mPixelFormat2D = Ogre::PixelFormatGpu::PFG_RGBA8_UNORM;
		mPixelFormat3D = Ogre::PixelFormatGpu::PFG_RGBA32_FLOAT;

		mUavBuffers = new Ogre::UavBufferPackedVec({});

		mPlaneEntity = 0;

		mPlaneMoDef = new MovableObjectDefinition();
		mPlaneMoDef->moType = MoTypeDynamicTriangleList;
		mPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

		mDebugPlaneMoDef = new MovableObjectDefinition();
		mDebugPlaneMoDef->moType = MoTypeDynamicTriangleList;
		mDebugPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

#if OGRE_DEBUG_MODE
		mBufferResolutionWidth = 256.0f;
		mBufferResolutionHeight = 256.0f;
#else
		mBufferResolutionWidth = 512.0f;
		mBufferResolutionHeight = 512.0f;
#endif

		mBufferResolutionDepth = 1.0f;

		mFieldWidth = 64.0f;
		mFieldHeight = 64.0f;

		// When I say "resolution" I mean "how many of the buffer indices are inside a leaf"
		mLeafResolutionX = 16.0f;
		mLeafResolutionZ = 16.0f;

		mLeafCountX = mBufferResolutionWidth / mLeafResolutionX;
		mLeafWidth = mFieldWidth / mLeafCountX;

		mLeafCountZ = mBufferResolutionHeight / mLeafResolutionZ;
		mLeafHeight = mFieldHeight / mLeafCountZ;
		
		mInkInputBuffer = std::vector<Particle>({});

		for (auto i = 0; i < mBufferResolutionWidth * mBufferResolutionHeight; i++) {
			mInkInputBuffer.push_back({ Ogre::Vector4(0.0f, 0.0f, 0.0f, 1.0f), Ogre::Vector3::ZERO });
		}
			
		mDrawFromUavBufferMat = Ogre::MaterialPtr();

		mGameEntityManager = geMgr;

		mDeinitialised = false;

		mDownloadingTextureViaTicket = false;

		mHaveSetTestComputeShaderParameters = false;
		mHaveSetAdvectionCopyComputeShaderParameters = false;
		mHaveSetVelocityAdvectionComputeShaderParameters = false;
		mHaveSetAddImpulsesComputeShaderParameters = false;
		mHaveSetDivergenceComputeShaderParameters = false;
		mHaveSetJacobiPressureComputeShaderParameters = false;
		mHaveSetSubtractPressureGradientComputeShaderParameters = false;

		mTimeAccumulator = 0.0f;

		mHand = 0;

		mDebugFieldBoundingHierarchy = false;

		mFieldBoundingHierarchy = std::vector<FieldComputeSystem_BoundingHierarchyBox>({});

		mLeaves = std::map<CellCoord, FieldComputeSystem_BoundingHierarchyBox*>({});

		mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
			(mBufferResolutionWidth * mBufferResolutionHeight) * sizeof(Particle), Ogre::MEMCATEGORY_GENERAL));

		mInstanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

		mInstanceBufferStart = mInstanceBuffer;

		memset(mInstanceBuffer, 0, (mBufferResolutionWidth * mBufferResolutionHeight * sizeof(Particle)) -
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
			mTransform[0]->vPos + Ogre::Vector3::ZERO,
			Ogre::Quaternion::IDENTITY,
			Ogre::Vector3::UNIT_SCALE,
			true,
			0.9f,
			true,
			Ogre::Vector3::ZERO,
			mRenderTargetTexture);

		createBoundingHierarchy();
	}

	void FieldComputeSystem::deinitialise(void)
	{
		if (!mDeinitialised)
		{
			mDrawFromUavBufferMat.setNull();

			if (mTestComputeJob) {
				mTestComputeJob->clearUavBuffers();
				mTestComputeJob->clearTexBuffers();
			}

			//if (mLeapMotionStagingTexture) 
			//{
			//	//mStagingTexture->upload
			//}

			if (mRenderTargetTexture)
			{
				auto listeners = mRenderTargetTexture->getListeners();

				if (listeners.size()) {
					mRenderTargetTexture->removeListener(mTestComputeJob);

					// It would seem that we do not need to remove HLMSUnlitDatablock listeners.
					// These are probably being handled higher up the chain by the game entity destructors.
					/*mTextures[FieldComputeSystemTexture::Velocity]->removeListener(static_cast<Ogre::HlmsUnlitDatablock*>(static_cast<Ogre::v1::Entity*>(mPlaneEntity->mMovableObject)->getSubEntity(0)->getDatablock()));*/
				}
			}

			//if (mVelocityTexture) {

			//	auto listeners = mVelocityTexture->getListeners();

			//	if (listeners.size()) {
			//		mVelocityTexture->removeListener(mTestComputeJob);
			//		mVelocityTexture->removeListener(mAdvectionComputeJob);
			//	}
			//}

			//if (mInkTexture) {

			//	auto listeners = mInkTexture->getListeners();

			//	if (listeners.size()) {
			//		mVelocityTexture->removeListener(mTestComputeJob);
			//		mInkTexture->removeListener(mAdvectionComputeJob);
			//	}
			//}


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

	void FieldComputeSystem::createBoundingHierarchy(void)
	{
		float xRes = mFieldWidth / 2;
		float zRes = mFieldHeight / 2;

		float depthCount = 0.5f;

		mFieldBoundingHierarchy.push_back(FieldComputeSystem_BoundingHierarchyBox(
			mTransform[0]->vPos + Ogre::Vector3(0, 0, 0),
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
					mTransform[0]->vPos + Ogre::Vector3(0, depthCount, 0),
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
				Ogre::Vector3 leafCenter = mTransform[0]->vPos + Ogre::Vector3(
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

				for (size_t j = 0; j < mLeafResolutionZ; ++j) {
					for (size_t i = 0; i < mLeafResolutionX; ++i) {
						box->mBufferIndices.push_back(FieldComputeSystem_BufferIndexPosition(
							offset + j + i,
							Ogre::Vector3(
								corner.x + (offsetX * (float)i) + halfOffsetX,
								0.0f,
								corner.z + (offsetZ * (float)j) + halfOffsetZ
							)));
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
		if (msg->mStagingTexture == mVelocityStagingTexture) 
		{
			mVelocityStagingTexture = 0;
		}
		else if (msg->mStagingTexture == mInkStagingTexture) 
		{
			mInkStagingTexture = 0;
		}
	} 

	void FieldComputeSystem::addUavBuffer(Ogre::UavBufferPacked* buffer) 
	{
		mUavBuffers->push_back(buffer);
	}

	void FieldComputeSystem::setTestComputeJob(Ogre::HlmsComputeJob* job)
	{
		mTestComputeJob = job;
	}

	void FieldComputeSystem::setAdvectionCopyComputeJob(Ogre::HlmsComputeJob* job)
	{
		mAdvectionCopyComputeJob = job;
	}

	void FieldComputeSystem::setVelocityAdvectionComputeJob(Ogre::HlmsComputeJob* job)
	{
		mVelocityAdvectionComputeJob = job;
	}

	void FieldComputeSystem::setInkAdvectionComputeJob(Ogre::HlmsComputeJob* job)
	{
		mInkAdvectionComputeJob = job;
	}

	void FieldComputeSystem::setAddImpulsesComputeJob(Ogre::HlmsComputeJob* job)
	{
		mAddImpulsesComputeJob = job;
	}

	void FieldComputeSystem::setDivergenceComputeJob(Ogre::HlmsComputeJob* job)
	{
		mDivergenceComputeJob = job;
	}

	void FieldComputeSystem::setJabobiPressureComputeJob(Ogre::HlmsComputeJob* job)
	{
		mJacobiPressureComputeJob = job;
	}

	void FieldComputeSystem::setSubtractPressureGradientComputeJob(Ogre::HlmsComputeJob* job)
	{
		mSubtractPressureGradientComputeJob = job;
	}

	void FieldComputeSystem::setMaterial(Ogre::MaterialPtr mat)
	{
		mDrawFromUavBufferMat = mat;
	}

	void FieldComputeSystem::setRenderTargetTexture(Ogre::TextureGpu* texture)
	{
		mRenderTargetTexture = texture;
	}

	void FieldComputeSystem::setVelocityTexture(Ogre::TextureGpu* texture)
	{
		mVelocityTexture = texture;
	}

	void FieldComputeSystem::setSecondaryVelocityTexture(Ogre::TextureGpu* texture)
	{
		mSecondaryVelocityTexture = texture;
	}

	void FieldComputeSystem::setPressureTexture(Ogre::TextureGpu* texture)
	{
		mPressureTexture = texture;
	}

	void FieldComputeSystem::setPressureGradientTexture(Ogre::TextureGpu* texture)
	{
		mPressureGradientTexture = texture;
	}

	void FieldComputeSystem::setDivergenceTexture(Ogre::TextureGpu* texture)
	{
		mDivergenceTexture = texture;
	}

	void FieldComputeSystem::setInkTexture(Ogre::TextureGpu* texture)
	{
		mInkTexture = texture;
	}

	void FieldComputeSystem::setSecondaryInkTexture(Ogre::TextureGpu* texture)
	{
		mSecondaryInkTexture = texture;
	}

	void FieldComputeSystem::setVelocityStagingTexture(Ogre::StagingTexture* sTex) 
	{
		mVelocityStagingTexture = sTex;
	}

	void FieldComputeSystem::setInkStagingTexture(Ogre::StagingTexture* sTex)
	{
		mInkStagingTexture = sTex;
	}

	void FieldComputeSystem::setAsyncTextureTicket2D(Ogre::AsyncTextureTicket* texTicket) 
	{
		mTextureTicket2D = texTicket;
	}

	void FieldComputeSystem::setAsyncTextureTicket3D(Ogre::AsyncTextureTicket* texTicket)
	{
		mTextureTicket3D = texTicket;
	}

	void FieldComputeSystem::update(float timeSinceLast)
	{
		mTimeAccumulator += timeSinceLast;

		if (mAdvectionCopyComputeJob) {

			if (!mHaveSetAdvectionCopyComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mAdvectionCopyComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* reciprocalDeltaX = shaderParams.findParameter("reciprocalDeltaX");

				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));

				reciprocalDeltaX->setManualValue(1.0f);
				
				shaderParams.setDirty();

				mHaveSetAdvectionCopyComputeShaderParameters = true;
			}
		}

		if (mVelocityAdvectionComputeJob) {

			if (!mHaveSetVelocityAdvectionComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mVelocityAdvectionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* reciprocalDeltaX = shaderParams.findParameter("reciprocalDeltaX");
				Ogre::ShaderParams::Param* velocityDissipationConstant = shaderParams.findParameter("velocityDissipationConstant");
				Ogre::ShaderParams::Param* inkDissipationConstant = shaderParams.findParameter("inkDissipationConstant");
				tsl->setManualValue(timeSinceLast);
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				reciprocalDeltaX->setManualValue(1.0f);
				velocityDissipationConstant->setManualValue(0.997f);
				inkDissipationConstant->setManualValue(0.997f);
				//inkDissipationConstant->setManualValue(1.0f);
				shaderParams.setDirty();

				mHaveSetVelocityAdvectionComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mVelocityAdvectionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
				shaderParams.setDirty();
			}
		}


		if (mInkAdvectionComputeJob) {

			if (!mHaveSetInkAdvectionComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mInkAdvectionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* reciprocalDeltaX = shaderParams.findParameter("reciprocalDeltaX");
				Ogre::ShaderParams::Param* velocityDissipationConstant = shaderParams.findParameter("velocityDissipationConstant");
				Ogre::ShaderParams::Param* inkDissipationConstant = shaderParams.findParameter("inkDissipationConstant");
				tsl->setManualValue(timeSinceLast);
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				reciprocalDeltaX->setManualValue(1.0f);
				velocityDissipationConstant->setManualValue(0.997f);
				inkDissipationConstant->setManualValue(0.997f);
				//inkDissipationConstant->setManualValue(1.0f);
				shaderParams.setDirty();

				mHaveSetInkAdvectionComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mInkAdvectionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				Ogre::ShaderParams::Param* reciprocalDeltaX = shaderParams.findParameter("reciprocalDeltaX");
				tsl->setManualValue(timeSinceLast);
				reciprocalDeltaX->setManualValue(1.0f);
				shaderParams.setDirty();
			}
		}

		if (mAddImpulsesComputeJob) {

			if (!mHaveSetAddImpulsesComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mAddImpulsesComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				shaderParams.setDirty();

				mHaveSetAddImpulsesComputeShaderParameters = true;
			}
		}

		if (mDivergenceComputeJob) {

			if (!mHaveSetDivergenceComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mDivergenceComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				halfDeltaX->setManualValue(0.5f);
				shaderParams.setDirty();

				mHaveSetDivergenceComputeShaderParameters = true;
			}
		}

		if (mJacobiPressureComputeJob) {

			if (!mHaveSetJacobiPressureComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mJacobiPressureComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				halfDeltaX->setManualValue(0.5f);
				shaderParams.setDirty();

				mHaveSetJacobiPressureComputeShaderParameters = true;
			}
		}

		if (mSubtractPressureGradientComputeJob) {

			if (!mHaveSetSubtractPressureGradientComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				Ogre::ShaderParams& shaderParams = mSubtractPressureGradientComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				halfDeltaX->setManualValue(0.5f);
				shaderParams.setDirty();

				mHaveSetSubtractPressureGradientComputeShaderParameters = true;
			}
		}

		if (mTestComputeJob)
		{
			if (!mHaveSetTestComputeShaderParameters) {
				Ogre::uint32 res[2];
				res[0] = mBufferResolutionWidth;
				res[1] = mBufferResolutionHeight;

				//Update the compute shader's
				Ogre::ShaderParams& shaderParams = mTestComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				auto pb = shaderParams.findParameter("pixelBuffer");
				texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
				shaderParams.setDirty();

				mTestComputeJob->setNumThreadGroups(
					(res[0] + mTestComputeJob->getThreadsPerGroupX() - 1u) / mTestComputeJob->getThreadsPerGroupX(),
					(res[1] + mTestComputeJob->getThreadsPerGroupY() - 1u) / mTestComputeJob->getThreadsPerGroupY(), 
					1u);

				mAdvectionCopyComputeJob->setNumThreadGroups(
					(res[0] + mAdvectionCopyComputeJob->getThreadsPerGroupX() - 1u) / mAdvectionCopyComputeJob->getThreadsPerGroupX(),
					(res[1] + mAdvectionCopyComputeJob->getThreadsPerGroupY() - 1u) / mAdvectionCopyComputeJob->getThreadsPerGroupY(),
					1u);

				mVelocityAdvectionComputeJob->setNumThreadGroups(
					(res[0] + mVelocityAdvectionComputeJob->getThreadsPerGroupX() - 1u) / mVelocityAdvectionComputeJob->getThreadsPerGroupX(),
					(res[1] + mVelocityAdvectionComputeJob->getThreadsPerGroupY() - 1u) / mVelocityAdvectionComputeJob->getThreadsPerGroupY(),
					1u);

				mInkAdvectionComputeJob->setNumThreadGroups(
					(res[0] + mInkAdvectionComputeJob->getThreadsPerGroupX() - 1u) / mInkAdvectionComputeJob->getThreadsPerGroupX(),
					(res[1] + mInkAdvectionComputeJob->getThreadsPerGroupY() - 1u) / mInkAdvectionComputeJob->getThreadsPerGroupY(),
					1u);

				mAddImpulsesComputeJob->setNumThreadGroups(
					(res[0] + mAddImpulsesComputeJob->getThreadsPerGroupX() - 1u) / mAddImpulsesComputeJob->getThreadsPerGroupX(),
					(res[1] + mAddImpulsesComputeJob->getThreadsPerGroupY() - 1u) / mAddImpulsesComputeJob->getThreadsPerGroupY(),
					1u);

				mDivergenceComputeJob->setNumThreadGroups(
					(res[0] + mDivergenceComputeJob->getThreadsPerGroupX() - 1u) / mDivergenceComputeJob->getThreadsPerGroupX(),
					(res[1] + mDivergenceComputeJob->getThreadsPerGroupY() - 1u) / mDivergenceComputeJob->getThreadsPerGroupY(),
					1u);

				mJacobiPressureComputeJob->setNumThreadGroups(
					(res[0] + mJacobiPressureComputeJob->getThreadsPerGroupX() - 1u) / mJacobiPressureComputeJob->getThreadsPerGroupX(),
					(res[1] + mJacobiPressureComputeJob->getThreadsPerGroupY() - 1u) / mJacobiPressureComputeJob->getThreadsPerGroupY(),
					1u);

				mSubtractPressureGradientComputeJob->setNumThreadGroups(
					(res[0] + mSubtractPressureGradientComputeJob->getThreadsPerGroupX() - 1u) / mSubtractPressureGradientComputeJob->getThreadsPerGroupX(),
					(res[1] + mSubtractPressureGradientComputeJob->getThreadsPerGroupY() - 1u) / mSubtractPressureGradientComputeJob->getThreadsPerGroupY(),
					1u);

				////Update the pass that draws the UAV Buffer into the RTT (we could
				////use auto param viewport_size, but this more flexible)
				Ogre::GpuProgramParametersSharedPtr psParams = mDrawFromUavBufferMat->getTechnique(0)->
					getPass(0)->getFragmentProgramParameters();

				psParams->setNamedConstant("texResolution", res, 1u, 2u);

				if (this->getInkStagingTexture())
				{
					this->getInkStagingTexture()->startMapRegion();

					auto tBox = this->getInkStagingTexture()->mapRegion(
						this->getBufferResolutionWidth(),
						this->getBufferResolutionHeight(),
						this->getBufferResolutionDepth(),
						1u,
						this->getPixelFormat3D());

					for (size_t z = 0; z < this->getBufferResolutionDepth(); ++z)
					{
						for (size_t y = 0; y < this->getBufferResolutionHeight(); ++y)
						{
							for (size_t x = 0; x < this->getBufferResolutionWidth(); ++x)
							{
								float* data = reinterpret_cast<float*>(tBox.at(x, y, z));
								data[0] = 0.0f;
								data[1] = (x > 56 && x < 66) ? 0.85f : 0.0f;
								data[2] = 0.0f;
								data[3] = 1.0f;
							}
						}
					}

					this->getInkStagingTexture()->stopMapRegion();

					this->getInkStagingTexture()->upload(tBox, this->getPrimaryInkTexture(), 0, 0, 0);

					this->mGameEntityManager->mLogicSystem->queueSendMessage(
						this->mGameEntityManager->mGraphicsSystem,
						Mq::REMOVE_STAGING_TEXTURE,
						mInkStagingTexture);

					mInkStagingTexture = 0;
				}

				mHaveSetTestComputeShaderParameters = true;

			}

			auto c = Ogre::ColourValue(0.0f, 0.32f, 0.12f, 1.0f);

			/*for (auto& leaf : mLeaves) {
				if (leaf.second->mLeafVisible) {
					leaf.second->mLeafVisible = false;
					mGameEntityManager->toggleGameEntityVisibility(leaf.second->mLeafEntity, false);
				}
			}*/






			for (auto& iter : mInkInputBuffer) {
				// the input buffer MUST be cleared as often as possible
				iter.colour = Ogre::Vector4(0.0f, 0.0f, 0.0f, 1.0f); // the ink field is always green and invisible
				iter.velocity = Ogre::Vector3::ZERO;
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

					Ogre::Real rHandSphereSquared = mHand->getBoundingSphere()->getRadius() * mHand->getBoundingSphere()->getRadius();

					Ogre::Vector3 vHandPos = mHand->getBoundingSphere()->getCenter();

					for (auto& l : leaves) {

						float x = l.mAaBb.getMinimum().x;
						float z = l.mAaBb.getMinimum().z;

						float zOffset = 0;
						float xOffset = 0;

						for (size_t i = 0; i < l.mBufferIndices.size(); i++) {

							auto& index = l.mBufferIndices[i];

							Ogre::Vector3 pos = index.mPosition;
							Ogre::Vector3 dist = vHandPos - pos;
							Ogre::Real distLen = dist.length();

							if (distLen < rHandSphereSquared) {

								//mInkInputBuffer[index.mIndex].colour.x = 0.0f;
								//mInkInputBuffer[index.mIndex].colour.y = 0.0f;
								mInkInputBuffer[index.mIndex].colour.z = 1000.0f; // mHand->getState().rInk;

																				// the ink field is always green and invisible
								//mInkInputBuffer[index.mIndex].colour.w = 1.0f; // except when the hand influences the alpha

								// how do I indicate that I want to upload ONLY THESE indices?
								// is that even what I want to do?

								//mInkInputBuffer[index.mIndex].velocity = Ogre::Vector3::ZERO;

								//mInkInputBuffer[index.mIndex].velocity = mHand->getState().vVel * 100;

								//mInkInputBuffer[index.mIndex].velocity = mHand->getState().vVel * 100;

								mInkInputBuffer[index.mIndex].velocity = mHand->getState().vVel * 100;

								//mInkInputBuffer[index.mIndex].velocity += mHand->getState().vVel;
							

								//auto m_CpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(sizeof(Particle), Ogre::MEMCATEGORY_GENERAL));

								//auto m_InstanceBuffer = reinterpret_cast<float*>(m_CpuInstanceBuffer);
								//
								//auto m_InstanceBufferStart = m_InstanceBuffer;

								//memset(m_InstanceBuffer, 0, (sizeof(Particle)) -
								//	(static_cast<size_t>(m_InstanceBuffer - m_InstanceBufferStart) * sizeof(float)));

								//auto buffer = getUavBuffer(0);
								//
								//float* instanceBuffer = const_cast<float*>(m_InstanceBufferStart);

								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].colour.r;
								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].colour.g;
								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].colour.b;
								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].colour.a;

								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].velocity.x;
								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].velocity.y;
								//*instanceBuffer++ = mInkInputBuffer[index.mIndex].velocity.z;

								//*instanceBuffer++ = 0.0f; // pressure

								//*instanceBuffer++ = 0.0f; // pressureGradient.x
								//*instanceBuffer++ = 0.0f; // pressureGradient.y
								//*instanceBuffer++ = 0.0f; // pressureGradient.z

								//buffer->upload(m_CpuInstanceBuffer, index.mIndex, 1);
							}
						}
					}
				}
			}
			//
			auto buffer = getUavBuffer(0);
			auto uavBufferNumElements = buffer->getNumElements();

			float* instanceBuffer = const_cast<float*>(mInstanceBufferStart);

			for (const auto& iter : mInkInputBuffer) {

				*instanceBuffer++ = iter.colour.x;
				*instanceBuffer++ = iter.colour.y;
				*instanceBuffer++ = iter.colour.z;
				*instanceBuffer++ = iter.colour.w;// iter.colour.a;// (float)sin(mTimeAccumulator);
				
				*instanceBuffer++ = iter.velocity.x;
				*instanceBuffer++ = iter.velocity.y;
				*instanceBuffer++ = iter.velocity.z;

				*instanceBuffer++ = 0.0f; // pressure

				*instanceBuffer++ = 0.0f; // pressureGradient.x
				*instanceBuffer++ = 0.0f; // pressureGradient.y
				*instanceBuffer++ = 0.0f; // pressureGradient.z
			}

			auto tsb = buffer->getTotalSizeBytes();
			auto calc = (size_t)(instanceBuffer - mInstanceBufferStart) * sizeof(float);

			OGRE_ASSERT_LOW(calc <= tsb);

			buffer->upload(mCpuInstanceBuffer, 0u, buffer->getNumElements());






			

			// my advection copy might be BS: copy the inkTexture to the previousInkTexture here?


			//if (mDownloadingTextureViaTicket && mTextureTicket2D->queryIsTransferDone()) 
			//{
			//	mDownloadingTextureViaTicket = false;

			//	for (int i = 0; i < mTextureTicket2D->getNumSlices(); i++) {

			//		auto texBox = mTextureTicket2D->map(i);

			//		auto colourValue = texBox.getColourAt(212, 230, 0, mPixelFormat2D);

			//		if (this->getVelocityStagingTexture())
			//		{
			//			this->getVelocityStagingTexture()->startMapRegion();

			//			auto tBox = this->getVelocityStagingTexture()->mapRegion(
			//				this->getBufferResolutionWidth(),
			//				this->getBufferResolutionHeight(),
			//				this->getBufferResolutionDepth(),
			//				1u,
			//				this->getPixelFormat3D());

			//			for (size_t z = 0; z < this->getBufferResolutionDepth(); ++z)
			//			{
			//				for (size_t y = 0; y < this->getBufferResolutionHeight(); ++y)
			//				{
			//					for (size_t x = 0; x < this->getBufferResolutionWidth(); ++x)
			//					{
			//						float* data = reinterpret_cast<float*>(tBox.at(x, y, z));
			//						float* copyFrom = reinterpret_cast<float*>(texBox.at(x, y, z));
			//						data[0] = *copyFrom++;
			//						data[1] = *copyFrom++;
			//						data[2] = *copyFrom++;
			//						data[3] = *copyFrom++;

			//						if (data[0] != 0 || data[1] != 0 || data[2] != 0) {
			//							int f = 0;
			//						}
			//					}
			//				}
			//			}

			//			this->getVelocityStagingTexture()->stopMapRegion();

			//			this->getVelocityStagingTexture()->upload(tBox, this->getPrimaryVelocityTexture(), 0, 0, 0);

			//			/*this->mGameEntityManager->mLogicSystem->queueSendMessage(
			//				this->mGameEntityManager->mGraphicsSystem,
			//				Mq::REMOVE_STAGING_TEXTURE,
			//				mVelocityStagingTexture);

			//			mVelocityStagingTexture = 0;*/
			//		}

			//		// Yay! I can read a texture which was written BY THE GPU!!!
			//	}
			//	mTextureTicket2D->unmap();
			//}
			//else 
			//{
			//	mTextureTicket2D->download(mRenderTargetTexture, 0, false);
			//	mDownloadingTextureViaTicket = true;
			//}
		}
	}

	void FieldComputeSystem::traverseBoundingHierarchy(
		const FieldComputeSystem_BoundingHierarchyBox& level, 
		const std::vector<FieldComputeSystem_BoundingHierarchyBox>* leaves,
		int& aabbIntersectionCount)
	{
		if (level.mAaBb.intersects(*mHand->getOuterBoundingSphere())) {
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
		if (mTestComputeJob) 
		{
			mGameEntityManager->mLogicSystem->queueSendMessage(
				mGameEntityManager->mGraphicsSystem,
				Mq::FIELD_COMPUTE_SYSTEM_WRITE_FILE_TESTING,
				NULL);
		}
	}
}






//if (mStagingTexture) 
//{
//	mStagingTexture->startMapRegion();

//	auto tBox = mStagingTexture->mapRegion(
//		mBufferResolutionWidth,
//		mBufferResolutionHeight,
//		mBufferResolutionDepth,
//		1u,
//		getPixelFormat3D());

//	for (size_t z = 0; z < mBufferResolutionDepth; ++z)
//	{
//		for (size_t y = 0; y < mBufferResolutionHeight; ++y)
//		{
//			for (size_t x = 0; x < mBufferResolutionWidth; ++x)
//			{
//				float* data = reinterpret_cast<float*>(tBox.at(x, y, z));
//				data[0] = 0.0f;
//				data[1] = 0.75f;
//				data[2] = 0.0f;
//			}
//		}
//	}

//	mStagingTexture->stopMapRegion();
//	mStagingTexture->upload(tBox, mVelocityTexture, 0, 0, 0);


//	// this works nicely because the message wrapper retains the pointer
//	// the object is destroyed correctly on the graphics thread
//	this->mGameEntityManager->mLogicSystem->queueSendMessage(
//		this->mGameEntityManager->mGraphicsSystem,
//		Mq::REMOVE_STAGING_TEXTURE,
//		mStagingTexture);
//	// it's safe - and probably best - to set the pointer to 0 here
//	// even if the graphics thread hasn't yet destroyed the object
//	// because it will prevent us from messing with it further here when it was 
//	// already marked for destruction
//	mStagingTexture = 0;
//}