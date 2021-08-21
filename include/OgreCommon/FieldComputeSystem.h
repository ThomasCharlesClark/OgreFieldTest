#pragma once
#include "OgreRoot.h"
#include "OgreMaterialManager.h"
#include "OgreMaterial.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsCompute.h"
#include "OgreHlmsComputeJob.h"
#include "GraphicsSystem.h"
#include "OgreTextureGpu.h"
#include "Compositor\OgreCompositorWorkspace.h"
#include "Threading\MessageQueueSystem.h"
#include "OgreStagingTexture.h"
#include "OgreAsyncTextureTicket.h"
#include "GameEntityManager.h"
#include <array>

namespace MyThirdOgre
{
	struct FieldComputeSystem_StagingTextureMessage
	{
		Ogre::StagingTexture* mStagingTexture;

		FieldComputeSystem_StagingTextureMessage(
			Ogre::StagingTexture* sTex
		) :
			mStagingTexture(sTex)
		{

		}
	};

	//struct FieldComputeSystem_TestMessage
	//{
	//	float						mTimeSinceLast;
	//	Ogre::AsyncTextureTicket*	mTextureTicket;
	//	Ogre::TextureGpu*			mUavTextureGpu;
	//	Ogre::HlmsComputeJob*		mComputeJob;

	//	FieldComputeSystem_TestMessage(
	//		float timeSinceLast,
	//		Ogre::AsyncTextureTicket* ticket,
	//		Ogre::TextureGpu* tex,
	//		Ogre::HlmsComputeJob* job) :
	//		mTimeSinceLast(timeSinceLast),
	//		mTextureTicket(ticket),
	//		mUavTextureGpu(tex),
	//		mComputeJob(job) 
	//	{
	//	
	//	}
	//};

	enum FieldComputeSystemTexture
	{
		Velocity,
		LeapMotion
	};

	class MessageQueueSystem;

	struct FieldComputeSystem : GameEntity
	{
		private:
			bool						mDeinitialised;

			float						mWidth;
			float						mHeight;
			float						mDepth;
			bool						mDownloadingTextureViaTicket;
			bool						mHaveSetShaderParamsOnce;

			MovableObjectDefinition*	mPlaneMoDef;
			GameEntity*					mPlaneEntity;

		protected:
			GameEntityManager*					mGameEntityManager;
			Ogre::TextureTypes::TextureTypes	mTextureType;
			Ogre::PixelFormatGpu				mPixelFormat;
			Ogre::MaterialPtr					mDrawFromUavBufferMat;
			Ogre::StagingTexture*				mLeapMotionStagingTexture;
			std::array<Ogre::TextureGpu*, 2>	mTextures;
			Ogre::AsyncTextureTicket*			mTextureTicket;
			Ogre::HlmsComputeJob*				mComputeJob;
			float								mTimeAccumulator;

		public:
			FieldComputeSystem(Ogre::uint32 id, const MovableObjectDefinition* moDefinition,
				Ogre::SceneMemoryMgrTypes type, GameEntityManager* geMgr);
			~FieldComputeSystem();

			virtual void _notifyStagingTextureRemoved(const FieldComputeSystem_StagingTextureMessage* msg);

			virtual void initialise(void);
			virtual void deinitialise(void);

			virtual void update(float timeSinceLast);

			virtual void setComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setMaterial(Ogre::MaterialPtr mat);
			virtual void setLeapMotionStagingTexture(Ogre::StagingTexture* tex);
			virtual void setTextures(std::array<Ogre::TextureGpu*, 2> textures);
			virtual void setAsyncTextureTicket(Ogre::AsyncTextureTicket* tex);
			virtual void setDownloadingTextureViaTicket(bool val) { mDownloadingTextureViaTicket = val; };

			Ogre::TextureTypes::TextureTypes getTextureType(void) { return mTextureType; };
			Ogre::PixelFormatGpu getPixelFormat(void) { return mPixelFormat; };
			float getWidth(void) { return mWidth; };
			float getHeight(void) { return mHeight; };
			float getDepth(void) { return mDepth; };
			bool getDownloadingTextureViaTicket(void) { return mDownloadingTextureViaTicket; };
			Ogre::HlmsComputeJob* getComputeJob(void) { return mComputeJob; };
			Ogre::TextureGpu* getTexture(FieldComputeSystemTexture t) { return mTextures[t]; };
			Ogre::AsyncTextureTicket* getTextureTicket(void) { return mTextureTicket; };

			virtual void writeDebugImages(float timeSinceLast);
	};
}