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
#include <Cell.h>

namespace MyThirdOgre
{
	struct Particle {
		Ogre::Real ink;
		Ogre::Vector4 colour;
		Ogre::Vector3 velocity;
	};

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

	struct FieldComputeSystem_BufferIndexPosition
	{

	public:
		size_t mIndex;
		Ogre::Vector3 mPosition;
		Ogre::Vector3* mFieldCenter;

		FieldComputeSystem_BufferIndexPosition(
			size_t i,
			Ogre::Vector3 p
		) : mIndex(i),
			mPosition(p)
		{

		}
	};

	struct FieldComputeSystem_BoundingHierarchyBox
	{
		Ogre::Vector3 mCenter;
		Ogre::Vector3 mHalfWidths;

		Ogre::AxisAlignedBox mAaBb;

		std::vector<FieldComputeSystem_BoundingHierarchyBox> mChildren;
		std::vector<FieldComputeSystem_BufferIndexPosition> mBufferIndices;
		GameEntity*			mLeafEntity;

		bool mIsLeaf;
		bool mLeafVisible;
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

			mLeafEntity = 0;
			mLeafVisible = false;

			mAaBb = Ogre::AxisAlignedBox(mCenter - mHalfWidths, mCenter + mHalfWidths);
			mChildren = std::vector<FieldComputeSystem_BoundingHierarchyBox>({});
			mBufferIndices = std::vector<FieldComputeSystem_BufferIndexPosition>({});
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
			bool						mHaveSetTestComputeShaderParameters;
			bool						mHaveSetAdvectionCopyComputeShaderParameters;
			bool						mHaveSetBoundaryConditionsComputeShaderParameters;
			bool						mHaveSetClearBuffersComputeShaderParameters;
			bool						mHaveSetClearBuffersComputeTwoShaderParameters;
			bool						mHaveSetVelocityAdvectionComputeShaderParameters;
			bool						mHaveSetJacobiDiffusionComputeShaderParameters;
			bool						mHaveSetInkAdvectionComputeShaderParameters;
			bool						mHaveSetAddImpulsesComputeShaderParameters;
			bool						mHaveSetDivergenceComputeShaderParameters;
			bool						mHaveSetJacobiPressureComputeShaderParameters;
			bool						mHaveSetSubtractPressureGradientComputeShaderParameters;
			bool						mHaveSetVorticityComputationComputeShaderParameters;
			bool						mHaveSetVorticityConfinementComputeShaderParameters;

			MovableObjectDefinition*	mPlaneMoDef;
			MovableObjectDefinition*	mDebugPlaneMoDef;
			GameEntity*					mPlaneEntity;
			GraphicsSystem*				mGraphicsSystem;

		protected:
			GameEntityManager*					mGameEntityManager;
			Ogre::TextureTypes::TextureTypes	mTextureType2D;
			Ogre::TextureTypes::TextureTypes	mTextureType3D;
			Ogre::PixelFormatGpu				mPixelFormat2D;
			Ogre::PixelFormatGpu				mPixelFormat3D;
			Ogre::MaterialPtr					mDrawFromUavBufferMat;
			Ogre::StagingTexture*				mVelocityStagingTexture;
			Ogre::StagingTexture*				mInkStagingTexture;
			Ogre::TextureGpu*					mRenderTargetTexture;
			Ogre::TextureGpu*					mVelocityTexture;
			Ogre::TextureGpu*					mSecondaryVelocityTexture;
			Ogre::TextureGpu*					mPressureTexture;
			Ogre::TextureGpu*					mPressureGradientTexture;
			Ogre::TextureGpu*					mDivergenceTexture;
			Ogre::TextureGpu*					mInkTexture;
			Ogre::TextureGpu*					mSecondaryInkTexture;
			Ogre::UavBufferPackedVec*			mUavBuffers;
			Ogre::AsyncTextureTicket*			mTextureTicket2D;
			Ogre::AsyncTextureTicket*			mTextureTicket3D;
			Ogre::HlmsComputeJob*				mTestComputeJob;
			Ogre::HlmsComputeJob*				mAdvectionCopyComputeJob;
			Ogre::HlmsComputeJob*				mBoundaryConditionsComputeJob;
			Ogre::HlmsComputeJob*				mClearBuffersComputeJob;
			Ogre::HlmsComputeJob*				mClearBuffersTwoComputeJob;
			Ogre::HlmsComputeJob*				mVelocityAdvectionComputeJob;
			Ogre::HlmsComputeJob*				mJacobiDiffusionComputeJob;
			Ogre::HlmsComputeJob*				mInkAdvectionComputeJob;
			Ogre::HlmsComputeJob*				mAddImpulsesComputeJob;
			Ogre::HlmsComputeJob*				mDivergenceComputeJob;
			Ogre::HlmsComputeJob*				mJacobiPressureComputeJob;
			Ogre::HlmsComputeJob*				mSubtractPressureGradientComputeJob;
			Ogre::HlmsComputeJob*				mVorticityComputationComputeJob;
			Ogre::HlmsComputeJob*				mVorticityConfinementComputeJob;
			float								mTimeAccumulator;
			Hand*								mHand;

			std::vector<FieldComputeSystem_BoundingHierarchyBox>	mFieldBoundingHierarchy;
			std::map<CellCoord, FieldComputeSystem_BoundingHierarchyBox*> mLeaves;
			bool													mDebugFieldBoundingHierarchy;

			std::vector<Particle>				mInkInputBuffer;

			float* mCpuInstanceBuffer;
			float* RESTRICT_ALIAS mInstanceBuffer;
			const float* mInstanceBufferStart;

			Ogre::uint32 resolution[2];

		public:
			FieldComputeSystem(Ogre::uint32 id, const MovableObjectDefinition* moDefinition,
				Ogre::SceneMemoryMgrTypes type, GameEntityManager* geMgr);
			~FieldComputeSystem();

			virtual void _notifyGraphicsSystem(GraphicsSystem* gs);
			virtual void _notifyStagingTextureRemoved(const FieldComputeSystem_StagingTextureMessage* msg);

			virtual void initialise(void);
			virtual void deinitialise(void);

			virtual void update(float timeSinceLast);

			virtual void setTestComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setAdvectionCopyComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setBoundaryConditionsComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setClearBuffersComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setClearBuffersTwoComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setVelocityAdvectionComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setInkAdvectionComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setAddImpulsesComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setJacobiDiffusionComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setDivergenceComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setJacobiPressureComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setSubtractPressureGradientComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setVorticityComputationComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setVorticityConfinementComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setMaterial(Ogre::MaterialPtr mat);
			virtual void setVelocityStagingTexture(Ogre::StagingTexture* tex);
			virtual void setInkStagingTexture(Ogre::StagingTexture* tex);
			virtual void setRenderTargetTexture(Ogre::TextureGpu* texture);
			virtual void setVelocityTexture(Ogre::TextureGpu* texture);
			virtual void setSecondaryVelocityTexture(Ogre::TextureGpu* texture);
			virtual void setPressureTexture(Ogre::TextureGpu* texture);
			virtual void setPressureGradientTexture(Ogre::TextureGpu* texture);
			virtual void setDivergenceTexture(Ogre::TextureGpu* texture);
			virtual void setInkTexture(Ogre::TextureGpu* texture);
			virtual void setSecondaryInkTexture(Ogre::TextureGpu* texture);
			virtual void setAsyncTextureTicket2D(Ogre::AsyncTextureTicket* tex);
			virtual void setAsyncTextureTicket3D(Ogre::AsyncTextureTicket* tex);
			virtual void setDownloadingTextureViaTicket(bool val) { mDownloadingTextureViaTicket = val; };

			virtual void addUavBuffer(Ogre::UavBufferPacked* b);

			Ogre::TextureTypes::TextureTypes getTextureType2D(void) { return mTextureType2D; };
			Ogre::TextureTypes::TextureTypes getTextureType3D(void) { return mTextureType3D; };
			Ogre::PixelFormatGpu getPixelFormat2D(void) { return mPixelFormat2D; };
			Ogre::PixelFormatGpu getPixelFormat3D(void) { return mPixelFormat3D; };
			Ogre::StagingTexture* getVelocityStagingTexture(void) { return mVelocityStagingTexture; };
			Ogre::StagingTexture* getInkStagingTexture(void) { return mInkStagingTexture; };
			float getBufferResolutionWidth(void) { return mBufferResolutionWidth; };
			float getBufferResolutionHeight(void) { return mBufferResolutionHeight; };
			float getBufferResolutionDepth(void) { return mBufferResolutionDepth; };
			float getDepth(void) { return mBufferResolutionDepth; };
			GameEntity* getPlane(void) { return mPlaneEntity; };
			bool getDownloadingTextureViaTicket(void) { return mDownloadingTextureViaTicket; };
			Ogre::HlmsComputeJob* getTestComputeJob(void) { return mTestComputeJob; };
			Ogre::HlmsComputeJob* getAdvectionCopyComputeJob(void) { return mAdvectionCopyComputeJob; };
			Ogre::HlmsComputeJob* getBoundaryConditionsComputeJob(void) { return mBoundaryConditionsComputeJob; };
			Ogre::HlmsComputeJob* getClearBuffersComputeJob(void) { return mClearBuffersComputeJob; };
			Ogre::HlmsComputeJob* getClearBuffersTwoComputeJob(void) { return mClearBuffersTwoComputeJob; };
			Ogre::HlmsComputeJob* getAdvectionComputeJob(void) { return mVelocityAdvectionComputeJob; };
			Ogre::HlmsComputeJob* getInkAdvectionComputeJob(void) { return mInkAdvectionComputeJob; };
			Ogre::HlmsComputeJob* getJacobiDiffusionComputeJob(void) { return mJacobiDiffusionComputeJob; };
			Ogre::HlmsComputeJob* getAddImpulsesComputeJob(void) { return mAddImpulsesComputeJob; };
			Ogre::HlmsComputeJob* getDivergenceComputeJob(void) { return mDivergenceComputeJob; };
			Ogre::HlmsComputeJob* getJacobiPressureComputeJob(void) { return mJacobiPressureComputeJob; };
			Ogre::HlmsComputeJob* getSubtractPressureGradientComputeJob(void) { return mSubtractPressureGradientComputeJob; };
			Ogre::HlmsComputeJob* getVorticityComputationComputeJob(void) { return mVorticityComputationComputeJob; };
			Ogre::HlmsComputeJob* getVorticityConfinementComputeJob(void) { return mVorticityConfinementComputeJob; };
			Ogre::TextureGpu* getRenderTargetTexture(void) { return mRenderTargetTexture; };
			Ogre::TextureGpu* getPrimaryVelocityTexture(void) { return mVelocityTexture; };
			Ogre::TextureGpu* getSecondaryVelocityTexture(void) { return mSecondaryVelocityTexture; };
			Ogre::TextureGpu* getPressureTexture(void) { return mPressureTexture; };
			Ogre::TextureGpu* getPressureGradientTexture(void) { return mPressureGradientTexture; };
			Ogre::TextureGpu* getDivergenceTexture(void) { return mDivergenceTexture; };
			Ogre::TextureGpu* getPrimaryInkTexture(void) { return mInkTexture; };
			Ogre::TextureGpu* getSecondaryInkTexture(void) { return mSecondaryInkTexture; };
			Ogre::UavBufferPackedVec* getUavBuffers(void) { return mUavBuffers; };
			Ogre::UavBufferPacked* getUavBuffer(int idx) { return mUavBuffers->at(idx); };
			Ogre::AsyncTextureTicket* getTextureTicket(void) { return mTextureTicket2D; };

			void _notifyHand(Hand* hand) { mHand = hand; };

			virtual void writeDebugImages(float timeSinceLast);

			virtual void createBoundingHierarchy(void);

			virtual void subdivideBoundingHierarchy(
				float xRes,
				float zRes,
				FieldComputeSystem_BoundingHierarchyBox& box,
				int& leafIndexX,
				int& leafIndexZ,
				float& depthCount);

			virtual void buildBoundingDivisionIntersections(const size_t index, FieldComputeSystem_BoundingHierarchyBox& box);

			virtual void traverseBoundingHierarchy(const FieldComputeSystem_BoundingHierarchyBox& level, 
				const std::vector<FieldComputeSystem_BoundingHierarchyBox>* leaves,
				int& aabbIntersectionCount);
	};
}