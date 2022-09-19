#include "OgreRoot.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreStagingTexture.h"
#include "OgreTextureGpuManager.h"
#include "OgreShaderParams.h"
#include "Threading\MessageQueueSystem.h"
#include "FieldComputeSystem.h"
#include "OgreAsyncTextureTicket.h"
#include "OgreEntity.h"
#include "OgreHlmsUnlitDatablock.h"
#include "Compositor\OgreCompositorWorkspace.h"
#include "Vao\OgreUavBufferPacked.h"
#include "Hand.h"
#include <Field.h>
#include <vector>

namespace MyThirdOgre
{
	FieldComputeSystem::FieldComputeSystem(
		Ogre::uint32 id,
		const MovableObjectDefinition* moDefinition,
		Ogre::SceneMemoryMgrTypes type,
		GameEntityManager* geMgr,
		const int columnCount,
		const int rowCount
	) : GameEntity(id, moDefinition, type)
	{
		mParent = 0;
		mRenderComputeJob = 0;
		mBoundaryConditionsComputeJob = 0;
		mClearBuffersComputeJob = 0;
		mVelocityAdvectionComputeJob = 0;
		mAddImpulsesComputeJob = 0;
		mDivergenceComputeJob = 0;
		mJacobiPressureComputeJob = 0;
		mSubtractPressureGradientComputeJob = 0;
		mVorticityConfinementComputeJob = 0;
		mVorticityComputationComputeJob = 0;

		mDeltaX = 1.0f;
		mHalfDeltaX = 0.5f;

		mInkDissipationConstant = 0.98f;
		mVelocityDissipationConstant = 0.98f;
		mPressureDissipationConstant = 0.988f;
		mViscosity = 0.15f;
		mVorticityConfinementScale = 0.35f;

		mReceivedStagingTexture = 0;

		textureTicketFrameCounter = 0;

		mRenderTargetTexture = 0;
		mVelocityTexture = 0;
		mPressureTexture = 0;
		mPressureGradientTexture = 0;
		mDivergenceTexture = 0;
		mInkTexture = 0;

		mVelocityStagingTexture = 0;
		mInkStagingTexture = 0;

		mGraphicsSystem = 0;

		mTextureTicket2D = 0;
		mTextureTicket3D = 0;

		mTextureType2D = Ogre::TextureTypes::Type2D;
		mTextureType3D = Ogre::TextureTypes::Type3D;

		mPixelFormat2D = Ogre::PixelFormatGpu::PFG_RGBA8_UNORM;
		mPixelFormat3D = Ogre::PixelFormatGpu::PFG_RGBA32_FLOAT;
		mPixelFormatFloat3D = Ogre::PixelFormatGpu::PFG_R32_FLOAT;

		mUavBuffers = new Ogre::UavBufferPackedVec({});

		mPlaneEntity = 0;

		mPlaneMoDef = new MovableObjectDefinition();
		mPlaneMoDef->moType = MoTypeDynamicTriangleList;
		mPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

		mDebugPlaneMoDef = new MovableObjectDefinition();
		mDebugPlaneMoDef->moType = MoTypeDynamicTriangleList;
		mDebugPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

		mImpulses = std::vector<HandInfluence> { };
//
//#if OGRE_DEBUG_MODE
//	/*	mBufferResolutionWidth = 23.0f;
//		mBufferResolutionHeight = 23.0f;
//		mFieldWidth = 23.0f;
//		mFieldHeight = 23.0f;*/
//		mBufferResolutionWidth = 128.0f;
//		mBufferResolutionHeight = 128.0f;
//		mFieldWidth = 64.0f;
//		mFieldHeight = 64.0f;
//#else
//		/*mBufferResolutionWidth = 128.0f;
//		mBufferResolutionHeight = 128.0f;
//		mFieldWidth = 128.0f;
//		mFieldHeight = 128.0f;*/
//		mBufferResolutionWidth = 128.0f; // 512.0f // not above 64 if in release mode visualising velocity arrows
//		mBufferResolutionHeight = 128.0f;
//		mFieldWidth = 64.0f;
//		mFieldHeight = 64.0f;
//#endif

		mColumnCount = columnCount;
		mRowCount = rowCount;

		mThreadGroupsX = 256;
		mThreadGroupsY = 256;

		mFieldWidth = mColumnCount;
		mFieldHeight = mRowCount;

		mBufferResolutionWidth = mThreadGroupsX;
		mBufferResolutionHeight = mThreadGroupsY;

		resolution[0] = mBufferResolutionWidth;
		resolution[1] = mBufferResolutionHeight;

		mBufferResolutionDepth = 1.0f;

		// When I say "resolution" I mean "how many of the buffer indices are inside a leaf"
		//mLeafResolutionX = 16.0f;
		//mLeafResolutionZ = 16.0f;

		mLeafResolutionX = mBufferResolutionWidth / 4;
		mLeafResolutionZ = mBufferResolutionHeight / 4;

		mLeafCountX = mBufferResolutionWidth / mLeafResolutionX;
		mLeafWidth = mFieldWidth / mLeafCountX;

		mLeafCountZ = mBufferResolutionHeight / mLeafResolutionZ;
		mLeafHeight = mFieldHeight / mLeafCountZ;

		mInkInputBuffer = std::vector<Particle>({});

		for (auto i = 0; i < mBufferResolutionWidth * mBufferResolutionHeight; i++) {
			mInkInputBuffer.push_back({
				Ogre::Real(0.0f),
				Ogre::Vector4(0.0f, 0.0f, 0.0f, 1.0f),
				Ogre::Vector3::ZERO
			});
		}

		mDrawFromUavBufferMat = Ogre::MaterialPtr();

		mGameEntityManager = geMgr;

		mDeinitialised = false;

		mDownloadingTextureViaTicket = false;

		mHaveSetRenderComputeShaderParameters = false;
		mHaveSetVelocityAdvectionComputeShaderParameters = false;
		mHaveSetAddImpulsesComputeShaderParameters = false;
		mHaveSetDivergenceComputeShaderParameters = false;
		mHaveSetJacobiPressureComputeShaderParameters = false;
		mHaveSetSubtractPressureGradientComputeShaderParameters = false;
		mHaveSetJacobiDiffusionComputeShaderParameters = false;
		mHaveSetBoundaryConditionsComputeShaderParameters = false;
		mHaveSetClearBuffersComputeShaderParameters = false;
		mHaveSetVorticityComputationComputeShaderParameters = false;
		mHaveSetVorticityConfinementComputeShaderParameters = false;

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
			mTransform[0]->vPos,
			Ogre::Quaternion::IDENTITY,
			Ogre::Vector3::UNIT_SCALE,
			true,
			1.0f,
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

				for (size_t j = 0; j < mLeafResolutionZ; j++) {
					for (size_t i = 0; i < mLeafResolutionX; i++) {

						auto p = Ogre::Vector3(
							corner.x + (offsetX * (float)i) + halfOffsetX,
							0.0f,
							corner.z + (offsetZ * (float)j) + halfOffsetZ
						);

						box->mBufferIndices.push_back(FieldComputeSystem_BufferIndexPosition(
							offset + j + i,
							p));
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
		Ogre::Quaternion qRot = Ogre::Quaternion::IDENTITY;

		depthCount += 0.2f;

		if (xRes > mLeafWidth&& zRes > mLeafHeight) {

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
		}
	}
	
	void FieldComputeSystem::_notifyField(Field* f) 
	{
		mParent = f;
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

	void FieldComputeSystem::setRenderComputeJob(Ogre::HlmsComputeJob* job)
	{
		mRenderComputeJob = job;
	}

	void FieldComputeSystem::setBoundaryConditionsComputeJob(Ogre::HlmsComputeJob* job)
	{
		mBoundaryConditionsComputeJob = job;
	}

	void FieldComputeSystem::setClearBuffersComputeJob(Ogre::HlmsComputeJob* job)
	{
		mClearBuffersComputeJob = job;
	}

	void FieldComputeSystem::setVelocityAdvectionComputeJob(Ogre::HlmsComputeJob* job)
	{
		mVelocityAdvectionComputeJob = job;
	}

	void FieldComputeSystem::setJacobiDiffusionComputeJob(Ogre::HlmsComputeJob* job)
	{
		mJacobiDiffusionComputeJob = job;
	}

	void FieldComputeSystem::setAddImpulsesComputeJob(Ogre::HlmsComputeJob* job)
	{
		mAddImpulsesComputeJob = job;
	}

	void FieldComputeSystem::setDivergenceComputeJob(Ogre::HlmsComputeJob* job)
	{
		mDivergenceComputeJob = job;
	}

	void FieldComputeSystem::setJacobiPressureComputeJob(Ogre::HlmsComputeJob* job)
	{
		mJacobiPressureComputeJob = job;
	}

	void FieldComputeSystem::setSubtractPressureGradientComputeJob(Ogre::HlmsComputeJob* job)
	{
		mSubtractPressureGradientComputeJob = job;
	}

	void FieldComputeSystem::setVorticityComputationComputeJob(Ogre::HlmsComputeJob* job)
	{
		mVorticityComputationComputeJob = job;
	}

	void FieldComputeSystem::setVorticityConfinementComputeJob(Ogre::HlmsComputeJob* job)
	{
		mVorticityConfinementComputeJob = job;
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

		Ogre::uint32 res[2];
		res[0] = mBufferResolutionWidth;
		res[1] = mBufferResolutionHeight;

		if (mAddImpulsesComputeJob) {

			if (!mHaveSetAddImpulsesComputeShaderParameters) {

				mAddImpulsesComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mAddImpulsesComputeJob->getShaderParams("default");

				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");

				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint32));

				shaderParams.setDirty();

				mHaveSetAddImpulsesComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mAddImpulsesComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mVelocityAdvectionComputeJob) {

			if (!mHaveSetVelocityAdvectionComputeShaderParameters) {

				mVelocityAdvectionComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mVelocityAdvectionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* reciprocalDeltaX = shaderParams.findParameter("reciprocalDeltaX");
				Ogre::ShaderParams::Param* velocityDissipationConstant = shaderParams.findParameter("velocityDissipationConstant");
				Ogre::ShaderParams::Param* inkDissipationConstant = shaderParams.findParameter("inkDissipationConstant");
				tsl->setManualValue(timeSinceLast);
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				reciprocalDeltaX->setManualValue(mDeltaX);
				velocityDissipationConstant->setManualValue(mVelocityDissipationConstant);
				inkDissipationConstant->setManualValue(mInkDissipationConstant);
				shaderParams.setDirty();

				mHaveSetVelocityAdvectionComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mVelocityAdvectionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mJacobiDiffusionComputeJob) {

			if (!mHaveSetJacobiDiffusionComputeShaderParameters) {

				mJacobiDiffusionComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mJacobiDiffusionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				Ogre::ShaderParams::Param* vis = shaderParams.findParameter("viscosity");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				halfDeltaX->setManualValue(mHalfDeltaX);
				vis->setManualValue(mViscosity);
				tsl->setManualValue(timeSinceLast);
				shaderParams.setDirty();

				mHaveSetJacobiDiffusionComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mJacobiDiffusionComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mDivergenceComputeJob) {

			if (!mHaveSetDivergenceComputeShaderParameters) {

				mDivergenceComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mDivergenceComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				halfDeltaX->setManualValue(mHalfDeltaX);
				shaderParams.setDirty();

				mHaveSetDivergenceComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mDivergenceComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mJacobiPressureComputeJob) {

			if (!mHaveSetJacobiPressureComputeShaderParameters) {

				mJacobiPressureComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mJacobiPressureComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				halfDeltaX->setManualValue(mHalfDeltaX);
				shaderParams.setDirty();

				mHaveSetJacobiPressureComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mJacobiPressureComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mBoundaryConditionsComputeJob) {

			if (!mHaveSetBoundaryConditionsComputeShaderParameters) {

				mBoundaryConditionsComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mBoundaryConditionsComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				Ogre::ShaderParams::Param* reciprocalDeltaX = shaderParams.findParameter("reciprocalDeltaX");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");

				tsl->setManualValue(timeSinceLast);

				reciprocalDeltaX->setManualValue(mDeltaX);

				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint32));

				shaderParams.setDirty();

				mHaveSetBoundaryConditionsComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mBoundaryConditionsComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mSubtractPressureGradientComputeJob) {

			if (!mHaveSetSubtractPressureGradientComputeShaderParameters) {

				mSubtractPressureGradientComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mSubtractPressureGradientComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				halfDeltaX->setManualValue(mHalfDeltaX);
				shaderParams.setDirty();

				mHaveSetSubtractPressureGradientComputeShaderParameters = true;
			}
		}

		if (mVorticityComputationComputeJob) {

			if (!mHaveSetVorticityComputationComputeShaderParameters) {

				mVorticityComputationComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mVorticityComputationComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				halfDeltaX->setManualValue(mHalfDeltaX);
				shaderParams.setDirty();

				mHaveSetVorticityComputationComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mSubtractPressureGradientComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		if (mVorticityConfinementComputeJob) {

			if (!mHaveSetVorticityConfinementComputeShaderParameters) {

				mVorticityConfinementComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mVorticityConfinementComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* halfDeltaX = shaderParams.findParameter("halfDeltaX");
				Ogre::ShaderParams::Param* vortConfScale = shaderParams.findParameter("vorticityConfinementScale");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				halfDeltaX->setManualValue(mHalfDeltaX);
				vortConfScale->setManualValue(mVorticityConfinementScale);
				tsl->setManualValue(timeSinceLast);
				shaderParams.setDirty();

				mHaveSetVorticityConfinementComputeShaderParameters = true;
			}
			else {
				Ogre::ShaderParams& shaderParams = mVorticityConfinementComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
				shaderParams.setDirty();
			}
		}

		if (mRenderComputeJob)
		{
			if (!mHaveSetRenderComputeShaderParameters) {

				mRenderComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				//Update the compute shader's
				Ogre::ShaderParams& shaderParams = mRenderComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
				Ogre::ShaderParams::Param* maxInk = shaderParams.findParameter("maxInk");
				auto pb = shaderParams.findParameter("pixelBuffer");
				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint));
				maxInk->setManualValue(20.0f);
				shaderParams.setDirty();

				////Update the pass that draws the UAV Buffer into the RTT (we could
				////use auto param viewport_size, but this more flexible)
				Ogre::GpuProgramParametersSharedPtr psParams = mDrawFromUavBufferMat->getTechnique(0)->
					getPass(0)->getFragmentProgramParameters();

				psParams->setNamedConstant("texResolution", res, 1u, 2u);

				// This is unnecessary, but I'm leaving it commented out here as an example of how to manually draw to a texture.
				// Need a staging texture delivered from the graphics system across threads, need to discard that staging texture as soon as done with it.
				//if (this->getInkStagingTexture())
				//{
				//	this->getInkStagingTexture()->startMapRegion();
				//	auto tBox = this->getInkStagingTexture()->mapRegion(
				//		this->getBufferResolutionWidth(),
				//		this->getBufferResolutionHeight(),
				//		this->getBufferResolutionDepth(),
				//		1u,
				//		this->getPixelFormat3D());
				//	for (size_t z = 0; z < this->getBufferResolutionDepth(); ++z)
				//	{
				//		for (size_t y = 0; y < this->getBufferResolutionHeight(); ++y)
				//		{
				//			for (size_t x = 0; x < this->getBufferResolutionWidth(); ++x)
				//			{
				//				float* data = reinterpret_cast<float*>(tBox.at(x, y, z));
				//				//data[0] = 0.0450033244f; //r
				//				//data[1] = 0.1421513113f; //g
				//				//data[2] = 0.4302441212f; //b
				//				//data[3] = 1.0f; //a
				//				data[0] = 0.0f; //r
				//				data[1] = 0.0f; //g
				//				data[2] = 0.0f; //b
				//				data[3] = 1.0f; //a
				//			}
				//		}
				//	}
				//	this->getInkStagingTexture()->stopMapRegion();
				//	this->getInkStagingTexture()->upload(tBox, this->getPrimaryInkTexture(), 0, 0, 0);
				//	this->mGameEntityManager->mLogicSystem->queueSendMessage(
				//		this->mGameEntityManager->mGraphicsSystem,
				//		Mq::REMOVE_STAGING_TEXTURE,
				//		mInkStagingTexture);
				//	mInkStagingTexture = 0;
				//}

				mHaveSetRenderComputeShaderParameters = true;
			}

			auto c = Ogre::ColourValue(0.0f, 0.32f, 0.12f, 1.0f);

			/*for (auto& leaf : mLeaves) {
				if (leaf.second->mLeafVisible) {
					leaf.second->mLeafVisible = false;
					mGameEntityManager->toggleGameEntityVisibility(leaf.second->mLeafEntity, false);
				}
			}*/

			// the input buffer MUST be cleared as often as possible
			for (auto& iter : mInkInputBuffer) {
				iter.colour = Ogre::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				iter.ink = 0.0f;
				iter.velocity = Ogre::Vector3::ZERO;
			}

			// Manual additions (logic system provides keyboard input)
			Ogre::Vector3 manualVel = Ogre::Vector3::ZERO;
			Ogre::Real manualInk = 0.0f;

			auto it = mImpulses.begin();

			while (it != mImpulses.end())
			{
				manualVel += it->vVelocity;
				manualInk += it->rInkAmount;
				it = mImpulses.erase(it);
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
					
					//auto indexVel = mInkInputBuffer[mBufferResolutionWidth / 2 * mBufferResolutionWidth + mBufferResolutionWidth].velocity;

					if (mGraphicsSystem) {
						mGraphicsSystem->setAdditionalDebugText(
							Ogre::String("\nIntersecting: ") + Ogre::StringConverter::toString(aabbIntersectionCount) + Ogre::String(" Leaf AaBbs") +
							Ogre::String("\nSeeing: " + Ogre::StringConverter::toString(leaves.size() * mLeafResolutionX * mLeafResolutionZ) + Ogre::String(" buffer indices")));
							/*Ogre::String("\nMidpoint Velocity: " + Ogre::StringConverter::toString(indexVel.x) + Ogre::String(", ")
																 + Ogre::StringConverter::toString(indexVel.y) + Ogre::String(", ")
																 + Ogre::StringConverter::toString(indexVel.z)));*/
					}

					Ogre::Real rHandSphereSquared = mHand->getBoundingSphere()->getRadius() * mHand->getBoundingSphere()->getRadius();

					Ogre::Vector3 vHandPos = mHand->getBoundingSphere()->getCenter();

					auto intersectors = std::vector<FieldComputeSystem_BufferIndexPosition>();

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

							if (distLen <= rHandSphereSquared) {

								intersectors.push_back(index);

								auto& thisElement = mInkInputBuffer[index.mIndex];

								auto ink = mHand->getState().rInk;

								//if (thisElement.ink > 5.0f)
								//	thisElement.ink = 5.0f;

								thisElement.colour.x = 0.85f;

								thisElement.colour.y = 0.2941176562f;

								thisElement.colour.w = mHand->getState().rInk;

								auto pos = mHand->getState().vPos;

								auto posPrev = mHand->getState().vPosPrev;

								auto dir = pos - posPrev;

								auto vel = mHand->getState().vVel;

								ink += manualInk;
								vel += manualVel;

								thisElement.ink += ink;
								thisElement.velocity = vel;

							}
						}
					}
				}
			}

			auto buffer = getUavBuffer(0);
			auto uavBufferNumElements = buffer->getNumElements();

			float* instanceBuffer = const_cast<float*>(mInstanceBufferStart);

			for (const auto& iter : mInkInputBuffer) {

				*instanceBuffer++ = iter.ink;

				*instanceBuffer++ = iter.colour.x;
				*instanceBuffer++ = iter.colour.y;
				*instanceBuffer++ = iter.colour.z;
				*instanceBuffer++ = iter.colour.w;// iter.colour.a;// (float)sin(mTimeAccumulator);

				*instanceBuffer++ = iter.velocity.x;
				*instanceBuffer++ = iter.velocity.z;
				*instanceBuffer++ = 0;// iter.velocity.z;
			}

			auto tsb = buffer->getTotalSizeBytes();
			auto calc = (size_t)(instanceBuffer - mInstanceBufferStart) * sizeof(float);

			OGRE_ASSERT_LOW(calc <= tsb);

			buffer->upload(mCpuInstanceBuffer, 0u, buffer->getNumElements());
		}

		if (mClearBuffersComputeJob) {

			if (!mHaveSetClearBuffersComputeShaderParameters) {

				mClearBuffersComputeJob->setNumThreadGroups(mThreadGroupsX, mThreadGroupsY, 1);

				Ogre::ShaderParams& shaderParams = mClearBuffersComputeJob->getShaderParams("default");

				Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");

				texResolution->setManualValue(resolution, sizeof(resolution) / sizeof(Ogre::uint32));

				Ogre::ShaderParams::Param* velDis = shaderParams.findParameter("velocityDissipationConstant");

				velDis->setManualValue(mVelocityDissipationConstant);

				Ogre::ShaderParams::Param* inkDis = shaderParams.findParameter("inkDissipationConstant");

				inkDis->setManualValue(mInkDissipationConstant);

				Ogre::ShaderParams::Param* pDis = shaderParams.findParameter("inkDissipationConstant");

				pDis->setManualValue(mPressureDissipationConstant);

				shaderParams.setDirty();

				mHaveSetClearBuffersComputeShaderParameters = true;

			}
			else {
				Ogre::ShaderParams& shaderParams = mClearBuffersComputeJob->getShaderParams("default");
				Ogre::ShaderParams::Param* tsl = shaderParams.findParameter("timeSinceLast");
				tsl->setManualValue(timeSinceLast);
			}
		}

		#pragma region For Awesome Visual Debugging using Arrow.mesh instances

		if (mParent && mParent->getVelocityVisible()) {
			if (!isDownloadingViaTextureTicket()) {
				getTextureTicket3D()->download(getVelocityTexture(), 0, true);
				setDownloadingTextureViaTicket(true);
			}
			else
			{
				if (getTextureTicket3D()->queryIsTransferDone())
				{
					setDownloadingTextureViaTicket(false);

					auto numSlices = getTextureTicket3D()->getNumSlices();

					for (int i = 0; i < numSlices; i++) {
						auto texBox = getTextureTicket3D()->map(i);
						auto pFormat = getVelocityTexture()->getPixelFormat();

						for (int x = 0; x < mBufferResolutionWidth; x++) {
							for (int z = 0; z < mBufferResolutionHeight; z++) {

								auto colourValue = texBox.getColourAt(x, z, 0, pFormat);

								FieldComputeSystem_VelocityMessage velocityMessage = FieldComputeSystem_VelocityMessage({
									CellCoord(x - (mBufferResolutionWidth / 2),
												0,
												z - (mBufferResolutionHeight / 2)),
									HandInfluence(Ogre::Vector3(colourValue.r, colourValue.g, colourValue.b), 0.0f) });

								mGameEntityManager->mGraphicsSystem->queueSendMessage(
									mGameEntityManager->mLogicSystem,
									Mq::FIELD_COMPUTE_SYSTEM_WRITE_VELOCITIES,
									velocityMessage);
							}
						}

						int f = 0;
						// Yay! I can read a texture which was written BY THE GPU!!!
					}
					getTextureTicket3D()->unmap();

					textureTicketFrameCounter = 0;
				}
			}
		}

#pragma endregion

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
		if (mRenderComputeJob)
		{
			mGameEntityManager->mLogicSystem->queueSendMessage(
				mGameEntityManager->mGraphicsSystem,
				Mq::FIELD_COMPUTE_SYSTEM_WRITE_FILE_TESTING,
				NULL);
		}
	}

	void FieldComputeSystem::addManualVelocity(float timeSinceLast, Ogre::Vector3 vVel, Ogre::Real rInk)
	{
		mImpulses.push_back(HandInfluence(vVel, rInk));
	}

	void FieldComputeSystem::reset(void) 
	{
		#pragma region Rest the sphere/hand

		mHand->setVelocity(0.0f, Ogre::Vector3::ZERO);
		mHand->setInk(0.0f, 0.0f);
		mHand->setPosition(0.0f, Ogre::Vector3::ZERO);

		#pragma endregion

		#pragma region Clear the manual (keyboard) inputs

		auto it = mImpulses.begin();

		while (it != mImpulses.end())
		{
			it = mImpulses.erase(it);
		}

		#pragma endregion

#pragma region Ask the graphics system to clear textures for us
		mGameEntityManager->mLogicSystem->queueSendMessage(
			mGameEntityManager->mGraphicsSystem,
			Mq::FIELD_COMPUTE_SYSTEM_REQUEST_RESET,
			NULL);
#pragma endregion

	}

	void FieldComputeSystem::receiveStagingTextureAndReset(Ogre::StagingTexture* stagingTexture) 
	{
#pragma region Clear Textures

#pragma endregion
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