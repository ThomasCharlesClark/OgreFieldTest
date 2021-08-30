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
#include <Hand.h>
#include "OgreAxisAlignedBox.h"

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

	struct FieldComputeSystem_BoundingHierarchyBox
	{
		Ogre::Vector3 mCenter;
		Ogre::Vector3 mHalfWidths;

		Ogre::AxisAlignedBox mAaBb;

		std::vector<FieldComputeSystem_BoundingHierarchyBox> mChildren;
		std::vector<size_t> mBufferIndices;

		bool mIsLeaf;
		int mLeafIndexX;
		int mLeafIndexZ;

	public:
		FieldComputeSystem_BoundingHierarchyBox(
			Ogre::Vector3 c,
			Ogre::Vector3 hw
		) : mCenter(c),
			mHalfWidths(hw)
		{
			mIsLeaf = false;
			mLeafIndexX = -1;
			mLeafIndexZ = -1;

			mAaBb = Ogre::AxisAlignedBox(mCenter - mHalfWidths, mCenter + mHalfWidths);
			mChildren = std::vector<FieldComputeSystem_BoundingHierarchyBox>({});
			mBufferIndices = std::vector<size_t>({});
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

	class MessageQueueSystem;

	struct FieldComputeSystem : GameEntity
	{
		private:
			bool						mDeinitialised;

			float						mBufferResolutionWidth;
			float						mBufferResolutionHeight;
			float						mFieldWidth;
			float						mFieldHeight;
			float						mLeafWidth;
			float						mLeafHeight;
			float						mLeafResolutionX;
			float						mLeafResolutionZ;
			int							mLeafCountX;
			int							mLeafCountZ;
			float						mBufferResolutionDepth;
			bool						mDownloadingTextureViaTicket;
			bool						mHaveSetShaderParamsOnce;

			MovableObjectDefinition*	mPlaneMoDef;
			MovableObjectDefinition*	mDebugPlaneMoDef;
			GameEntity*					mPlaneEntity;

		protected:
			GameEntityManager*					mGameEntityManager;
			Ogre::TextureTypes::TextureTypes	mTextureType;
			Ogre::PixelFormatGpu				mPixelFormat;
			Ogre::MaterialPtr					mDrawFromUavBufferMat;
			Ogre::StagingTexture*				mLeapMotionStagingTexture;
			Ogre::TextureGpu*					mRenderTargetTexture;
			Ogre::UavBufferPackedVec*			mUavBuffers;
			Ogre::AsyncTextureTicket*			mTextureTicket;
			Ogre::HlmsComputeJob*				mComputeJob;
			float								mTimeAccumulator;
			Hand*								mHand;

			std::vector<FieldComputeSystem_BoundingHierarchyBox>	mFieldBoundingHierarchy;
			bool													mDebugFieldBoundingHierarchy;

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
			virtual void setTexture(Ogre::TextureGpu* texture);
			virtual void setAsyncTextureTicket(Ogre::AsyncTextureTicket* tex);
			virtual void setDownloadingTextureViaTicket(bool val) { mDownloadingTextureViaTicket = val; };

			virtual void addUavBuffer(Ogre::UavBufferPacked* b);

			Ogre::TextureTypes::TextureTypes getTextureType(void) { return mTextureType; };
			Ogre::PixelFormatGpu getPixelFormat(void) { return mPixelFormat; };
			float getWidth(void) { return mBufferResolutionWidth; };
			float getHeight(void) { return mBufferResolutionHeight; };
			float getDepth(void) { return mBufferResolutionDepth; };
			GameEntity* getPlane(void) { return mPlaneEntity; };
			bool getDownloadingTextureViaTicket(void) { return mDownloadingTextureViaTicket; };
			Ogre::HlmsComputeJob* getComputeJob(void) { return mComputeJob; };
			Ogre::TextureGpu* getRenderTargetTexture(void) { return mRenderTargetTexture; };
			Ogre::UavBufferPackedVec* getUavBuffers(void) { return mUavBuffers; };
			Ogre::UavBufferPacked* getUavBuffer(int idx) { return mUavBuffers->at(idx); };
			Ogre::AsyncTextureTicket* getTextureTicket(void) { return mTextureTicket; };

			void _notifyHand(Hand* hand) { mHand = hand; };

			virtual void writeDebugImages(float timeSinceLast);

			virtual void createBoundingHierarchy(void);

			virtual void subdivideBoundingHierarchy(
				float xRes,
				float zRes,
				FieldComputeSystem_BoundingHierarchyBox& box,
				int& leafIndexX,
				int& leafIndexZ,
				std::vector<FieldComputeSystem_BoundingHierarchyBox*>& leaves,
				int& depthCount);

			virtual void buildBoundingDivisionIntersections(const size_t index, FieldComputeSystem_BoundingHierarchyBox& box);

			virtual void traverseBoundingHierarchy(const FieldComputeSystem_BoundingHierarchyBox& level, 
				const std::vector<size_t>* indicesList);
	};
}