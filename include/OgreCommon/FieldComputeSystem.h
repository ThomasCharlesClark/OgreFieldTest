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
#include <Field.h>

namespace MyThirdOgre
{
	struct FieldComputeSystem_VelocityMessage
	{
		std::pair<CellCoord, HandInfluence> velocity;

		FieldComputeSystem_VelocityMessage(std::pair<CellCoord, HandInfluence> v)
		{
			velocity = v;
		}
	};

	struct Particle {
		Ogre::Real ink;
		Ogre::Vector4 colour;
		Ogre::Vector3 velocity;
		Ogre::Real inkLifetime; // what is the objective here? we want ink to start disappearing itself after a given time. Say 5s.
								// whenever ink is added, give it a lifetime of 5s. in advection, reduce inkLifetime by timeSinceLast.
								// if inkLifetime == 0, set ink to 0?
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

			mAaBb = Ogre::AxisAlignedBox(mCenter - mHalfWidths - Ogre::Vector3(0.0f, 1.0f, 0.0f), mCenter + mHalfWidths + Ogre::Vector3(0.0f, 1.0f, 0.0f));
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

	class Field;

	class MessageQueueSystem;

	struct FieldComputeSystem : GameEntity
	{
		private:
			bool						mDeinitialised;

			float						mBufferResolutionWidth;
			float						mBufferResolutionHeight;
			float						mBufferResolutionDepth;
			float						mFieldWidth;
			float						mFieldHeight;
			float						mLeafWidth;
			float						mLeafHeight;
			float						mLeafResolutionX;
			float						mLeafResolutionZ;
			float						mVelocityDissipationConstant;
			float						mInkDissipationConstant;
			float						mPressureDissipationConstant;
			float						mViscosity;
			float						mVorticityConfinementScale;
			float						mDeltaX;
			float						mHalfDeltaX;
			int							mColumnCount;
			int							mRowCount;
			int							mThreadGroupsX;
			int							mThreadGroupsY;
			int							mLeafCountX;
			int							mLeafCountZ;
			bool						mVelocityVisible;
			bool						mDownloadingTextureViaTicket;
			bool						mHaveSetRenderComputeShaderParameters;
			bool						mHaveSetBoundaryConditionsComputeShaderParameters;
			bool						mHaveSetClearBuffersComputeShaderParameters;
			bool						mHaveSetVelocityAdvectionComputeShaderParameters;
			bool						mHaveSetJacobiDiffusionComputeShaderParameters;
			bool						mHaveSetAddImpulsesComputeShaderParameters;
			bool						mHaveSetDivergenceComputeShaderParameters;
			bool						mHaveSetJacobiPressureComputeShaderParameters;
			bool						mHaveSetSubtractPressureGradientComputeShaderParameters;
			bool						mHaveSetVorticityComputationComputeShaderParameters;
			bool						mHaveSetVorticityConfinementComputeShaderParameters;

			int							textureTicketFrameCounter;

			MovableObjectDefinition*	mPlaneMoDef;
			MovableObjectDefinition*	mDebugPlaneMoDef;
			GameEntity*					mPlaneEntity;
			GraphicsSystem*				mGraphicsSystem;
			Field*						mParent;

		protected:
			GameEntityManager*					mGameEntityManager;
			Ogre::TextureTypes::TextureTypes	mTextureType2D;
			Ogre::TextureTypes::TextureTypes	mTextureType3D;
			Ogre::PixelFormatGpu				mPixelFormat2D;
			Ogre::PixelFormatGpu				mPixelFormat3D;
			Ogre::PixelFormatGpu				mPixelFormatFloat3D;
			Ogre::MaterialPtr					mDrawFromUavBufferMat;
			Ogre::StagingTexture*				mVelocityStagingTexture;
			Ogre::StagingTexture*				mInkStagingTexture;
			Ogre::StagingTexture*				mReceivedStagingTexture;
			Ogre::TextureGpu*					mRenderTargetTexture;
			Ogre::TextureGpu*					mVelocityTexture;
			Ogre::TextureGpu*					mPressureTexture;
			Ogre::TextureGpu*					mPressureGradientTexture;
			Ogre::TextureGpu*					mDivergenceTexture;
			Ogre::TextureGpu*					mInkTexture;
			Ogre::UavBufferPackedVec*			mUavBuffers;
			Ogre::AsyncTextureTicket*			mTextureTicket2D;
			Ogre::AsyncTextureTicket*			mTextureTicket3D;
			Ogre::HlmsComputeJob*				mRenderComputeJob;
			Ogre::HlmsComputeJob*				mBoundaryConditionsComputeJob;
			Ogre::HlmsComputeJob*				mClearBuffersComputeJob;
			Ogre::HlmsComputeJob*				mVelocityAdvectionComputeJob;
			Ogre::HlmsComputeJob*				mJacobiDiffusionComputeJob;
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
			std::vector<HandInfluence>			mImpulses;

			float* mCpuInstanceBuffer;
			float* RESTRICT_ALIAS mInstanceBuffer;
			const float* mInstanceBufferStart;

			Ogre::uint32 resolution[2];

		public:
			FieldComputeSystem(Ogre::uint32 id, 
				const MovableObjectDefinition* moDefinition,
				Ogre::SceneMemoryMgrTypes type,
				GameEntityManager* geMgr,
				const int columnCount,
				const int rowCount,
				const bool velocityVisible
				);
			~FieldComputeSystem();

			virtual void _notifyGraphicsSystem(GraphicsSystem* gs);
			virtual void _notifyStagingTextureRemoved(const FieldComputeSystem_StagingTextureMessage* msg);
			virtual void _notifyField(Field* f);

			virtual void initialise(void);
			virtual void deinitialise(void);

			virtual void update(float timeSinceLast);

			virtual void setRenderComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setBoundaryConditionsComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setClearBuffersComputeJob(Ogre::HlmsComputeJob* job);
			virtual void setVelocityAdvectionComputeJob(Ogre::HlmsComputeJob* job);
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
			virtual void setPressureTexture(Ogre::TextureGpu* texture);
			virtual void setPressureGradientTexture(Ogre::TextureGpu* texture);
			virtual void setDivergenceTexture(Ogre::TextureGpu* texture);
			virtual void setInkTexture(Ogre::TextureGpu* texture);
			virtual void setAsyncTextureTicket2D(Ogre::AsyncTextureTicket* tex);
			virtual void setAsyncTextureTicket3D(Ogre::AsyncTextureTicket* tex);
			virtual void setDownloadingTextureViaTicket(bool val) { mDownloadingTextureViaTicket = val; };

			virtual void addUavBuffer(Ogre::UavBufferPacked* b);

			virtual void receiveStagingTextureAndReset(Ogre::StagingTexture* texture);

			virtual void reset(void);

			Ogre::TextureTypes::TextureTypes getTextureType2D(void) { return mTextureType2D; };
			Ogre::TextureTypes::TextureTypes getTextureType3D(void) { return mTextureType3D; };
			Ogre::PixelFormatGpu getPixelFormat2D(void) { return mPixelFormat2D; };
			Ogre::PixelFormatGpu getPixelFormat3D(void) { return mPixelFormat3D; };
			Ogre::PixelFormatGpu getPixelFormatFloat3D(void) { return mPixelFormatFloat3D; };
			Ogre::StagingTexture* getVelocityStagingTexture(void) { return mVelocityStagingTexture; };
			Ogre::StagingTexture* getInkStagingTexture(void) { return mInkStagingTexture; };
			std::vector<Particle> getInkInputBuffer(void) { return mInkInputBuffer; };
			float getBufferResolutionWidth(void) { return mBufferResolutionWidth; };
			float getBufferResolutionHeight(void) { return mBufferResolutionHeight; };
			float getBufferResolutionDepth(void) { return mBufferResolutionDepth; };
			float getDepth(void) { return mBufferResolutionDepth; };
			GameEntity* getPlane(void) { return mPlaneEntity; };
			bool getDownloadingTextureViaTicket(void) { return mDownloadingTextureViaTicket; };
			Ogre::HlmsComputeJob* getRenderComputeJob(void) { return mRenderComputeJob; };
			Ogre::HlmsComputeJob* getBoundaryConditionsComputeJob(void) { return mBoundaryConditionsComputeJob; };
			Ogre::HlmsComputeJob* getClearBuffersComputeJob(void) { return mClearBuffersComputeJob; };
			Ogre::HlmsComputeJob* getAdvectionComputeJob(void) { return mVelocityAdvectionComputeJob; };
			Ogre::HlmsComputeJob* getJacobiDiffusionComputeJob(void) { return mJacobiDiffusionComputeJob; };
			Ogre::HlmsComputeJob* getAddImpulsesComputeJob(void) { return mAddImpulsesComputeJob; };
			Ogre::HlmsComputeJob* getDivergenceComputeJob(void) { return mDivergenceComputeJob; };
			Ogre::HlmsComputeJob* getJacobiPressureComputeJob(void) { return mJacobiPressureComputeJob; };
			Ogre::HlmsComputeJob* getSubtractPressureGradientComputeJob(void) { return mSubtractPressureGradientComputeJob; };
			Ogre::HlmsComputeJob* getVorticityComputationComputeJob(void) { return mVorticityComputationComputeJob; };
			Ogre::HlmsComputeJob* getVorticityConfinementComputeJob(void) { return mVorticityConfinementComputeJob; };
			Ogre::TextureGpu* getRenderTargetTexture(void) { return mRenderTargetTexture; };
			Ogre::TextureGpu* getVelocityTexture(void) { return mVelocityTexture; };
			Ogre::TextureGpu* getPressureTexture(void) { return mPressureTexture; };
			Ogre::TextureGpu* getPressureGradientTexture(void) { return mPressureGradientTexture; };
			Ogre::TextureGpu* getDivergenceTexture(void) { return mDivergenceTexture; };
			Ogre::TextureGpu* getInkTexture(void) { return mInkTexture; };
			Ogre::UavBufferPackedVec* getUavBuffers(void) { return mUavBuffers; };
			Ogre::UavBufferPacked* getUavBuffer(int idx) { return mUavBuffers->at(idx); };
			Ogre::AsyncTextureTicket* getTextureTicket2D(void) { return mTextureTicket2D; };
			Ogre::AsyncTextureTicket* getTextureTicket3D(void) { return mTextureTicket3D; };
			Field* getParent(void) { return mParent; };

			bool isDownloadingViaTextureTicket(void) { return mDownloadingTextureViaTicket; };

			void _notifyHand(Hand* hand) { mHand = hand; };

			virtual void writeDebugImages(float timeSinceLast);

			virtual void addManualInput(float timeSinceLast, Ogre::Vector3 v, Ogre::Real rInk = 0.0f);

			virtual void createBoundingHierarchy(void);

			virtual void subdivideBoundingHierarchy(
				float xRes,
				float zRes,
				FieldComputeSystem_BoundingHierarchyBox& box,
				int& leafIndexX,
				int& leafIndexZ,
				float& depthCount);

			virtual void traverseBoundingHierarchy(const FieldComputeSystem_BoundingHierarchyBox& level, 
				const std::vector<FieldComputeSystem_BoundingHierarchyBox>* leaves,
				int& aabbIntersectionCount);
	};
}