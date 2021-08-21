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

namespace MyThirdOgre 
{
	FieldComputeSystem::FieldComputeSystem(Ogre::uint32 id, const MovableObjectDefinition* moDefinition,
		Ogre::SceneMemoryMgrTypes type, GameEntityManager* geMgr) : GameEntity(id, moDefinition, type)
	{
		mTextureType = Ogre::TextureTypes::Type2D;
		mPixelFormat = Ogre::PixelFormatGpu::PFG_RGBA32_FLOAT;

		mPlaneEntity = 0;

		mPlaneMoDef = new MovableObjectDefinition();
		mPlaneMoDef->moType = MoTypePrefabPlane;
		mPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

		mWidth = 1024.0f;
		mHeight = 1024.0f;
		mDepth = 1.0f;
		mDrawFromUavBufferMat = Ogre::MaterialPtr();

		mDeinitialised = false;

		mGameEntityManager = geMgr;

		mDownloadingTextureViaTicket = false;

		mHaveSetShaderParamsOnce = false;
	}

	FieldComputeSystem::~FieldComputeSystem() 
	{
		if (mPlaneMoDef) {
			delete mPlaneMoDef;
			mPlaneMoDef = 0;
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
			Ogre::SceneManager::PrefabType::PT_PLANE,
			Ogre::BLANKSTRING,
			Ogre::Vector3::ZERO,
			qRot,
			Ogre::Vector3::UNIT_SCALE,
			true,
			1.0f,
			true,
			Ogre::Vector3::ZERO,
			mTextures[FieldComputeSystemTexture::Velocity]);


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

			if (mLeapMotionStagingTexture) 
			{
				//mStagingTexture->upload
			}

			if (mTextures[FieldComputeSystemTexture::Velocity]) 
			{
				auto listeners = mTextures[FieldComputeSystemTexture::Velocity]->getListeners();

				if (listeners.size()) {
					//mTextures[FieldComputeSystemTexture::Velocity]->removeListener(mComputeJob);
					//mTextures[FieldComputeSystemTexture::Velocity]->removeListener(static_cast<Ogre::HlmsUnlitDatablock*>(static_cast<Ogre::v1::Entity*>(mPlaneEntity->mMovableObject)->getSubEntity(0)->getDatablock()));
				}
			}

			if (mTextures[FieldComputeSystemTexture::LeapMotion])
			{
				auto listeners = mTextures[FieldComputeSystemTexture::LeapMotion]->getListeners();

				if (listeners.size()) {
					//mTextures[FieldComputeSystemTexture::LeapMotion]->removeListener(mComputeJob);
					//mTextures[FieldComputeSystemTexture::LeapMotion]->removeListener(static_cast<Ogre::HlmsUnlitDatablock*>(static_cast<Ogre::v1::Entity*>(mPlaneEntity->mMovableObject)->getSubEntity(0)->getDatablock()));
				}
			}

			mDeinitialised = true;
		}
	}

	void FieldComputeSystem::setComputeJob(Ogre::HlmsComputeJob* job)
	{
		mComputeJob = job;
	}

	void FieldComputeSystem::setMaterial(Ogre::MaterialPtr mat)
	{
		mDrawFromUavBufferMat = mat;
	}

	void FieldComputeSystem::setTextures(std::array<Ogre::TextureGpu*, 2> textures)
	{
		//StagingTexture
		//AsyncTextureTicket
		mTextures = textures;

		//mVelocityTexture = textures[0];
		//mLeapMotionInputTexture = textures[1];
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

		if (mComputeJob && !mHaveSetShaderParamsOnce)
		{
			Ogre::uint32 res[2];
			res[0] = mWidth;
			res[1] = mHeight;

			//Update the compute shader's
			Ogre::ShaderParams& shaderParams = mComputeJob->getShaderParams("default");
			Ogre::ShaderParams::Param* texResolution = shaderParams.findParameter("texResolution");
			auto pb = shaderParams.findParameter("pixelBuffer");
			texResolution->setManualValue(res, sizeof(res) / sizeof(Ogre::uint32));
			shaderParams.setDirty();

			mComputeJob->setNumThreadGroups((res[0] + mComputeJob->getThreadsPerGroupX() - 1u) /
				mComputeJob->getThreadsPerGroupX(),
				(res[1] + mComputeJob->getThreadsPerGroupY() - 1u) /
				mComputeJob->getThreadsPerGroupY(), 1u);

			////Update the pass that draws the UAV Buffer into the RTT (we could
			////use auto param viewport_size, but this more flexible)
			Ogre::GpuProgramParametersSharedPtr psParams = mDrawFromUavBufferMat->getTechnique(0)->
				getPass(0)->getFragmentProgramParameters();

			psParams->setNamedConstant("texResolution", res, 1u, 2u);

			mHaveSetShaderParamsOnce = true;

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
		}
	}

	void FieldComputeSystem::writeDebugImages(float timeSinceLast) 
	{
		if (mComputeJob) 
		{
			/*mTextureTicket->download(mUavTextureGpu, 0, false); 
			mDownloadingTextureViaTicket = true;*/

			mGameEntityManager->mLogicSystem->queueSendMessage(
				mGameEntityManager->mGraphicsSystem,
				Mq::FIELD_COMPUTE_SYSTEM_WRITE_FILE_TESTING,
				NULL);
				//FieldComputeSystem_TestMessage(mTimeAccumulator, mTextureTicket, mVelocityTexture, mComputeJob));
		}
	}
}