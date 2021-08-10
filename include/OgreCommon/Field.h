#pragma once
#include <vector>

#include "Cell.h"
#include "Hand.h"
#include "OgreSceneNode.h"
#include "OgreManualObject2.h"
#include "GameEntity.h"
#include "GameEntityManager.h"

#include <map>
#include <vector>

namespace MyThirdOgre 
{
	class Hand;
	struct HandInfluence;

	class Field
	{
		float	mScale;
		float	mHalfScale;
		float	mDeltaX;
		float	mHalfReciprocalDeltaX;
		float	mReciprocalDeltaX; // radix?
		int		mCellCount;
		int		mRowCount;
		int		mColumnCount;
		int		mLayerCount;
		float	mBaseManualVelocityAdjustmentSpeed;
		float	mBoostedManualVelocityAdjustmentSpeed;
		float	mBaseManualPressureAdjustmentSpeed;
		float	mBoostedManualPressureAdjustmentSpeed;


		float mMinCellPressure;
		float mMaxCellPressure;
		float mMaxCellVelocitySquared;
		float mViscosity;
		float mFluidDensity;
		float mKinematicViscosity;
		float mVelocityDissipationConstant;
		float mInkDissipationConstant;

		int mJacobiIterationsPressure;
		int mJacobiIterationsDiffusion;

		bool mManualAdjustmentSpeedModifier;
		GameEntityManager* mGameEntityManager;
		GameEntity* mGridEntity;
		MovableObjectDefinition* mGridLineMoDef;
		std::unordered_map<CellCoord, Cell*> mCells;
		std::vector<std::pair<CellCoord, HandInfluence>> mImpulses;

		Cell* mActiveCell;
		Hand* mHand;

		bool mIsRunning;

		int mPressureSpreadHalfWidth;
		int mVelocitySpreadHalfWidth;

		bool mGridVisible;
		bool mVelocityVisible;
		bool mPressureGradientVisible;

		float mVorticityConfinementScale;

	protected:
		virtual void createGrid(void);

		virtual void createCells(void);

	public:
		Field(GameEntityManager* geMgr, float scale, int columnCount, int rowCount);
		~Field();

		virtual void _notifyHand(Hand* hand) { mHand = hand; };

		Cell* getCell(CellCoord coords);

		virtual void update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex);

		// we want to be updating a "shadow" of all of our cells' transforms 
		// this shadow needs to be processed through a number of different functions 
		// before the end result is actually rendered

		bool getIsRunning(void) { return mIsRunning; }
		virtual void toggleIsRunning(void);

		virtual void advect(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void addImpulses(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void jacobiDiffusion(float timeSinceLaste, std::unordered_map<CellCoord, CellState>& state);
		virtual void divergence(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void jacobiPressure(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void subtractPressureGradient(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void vorticityComputation(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void vorticityConfinement(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);
		virtual void boundaryConditions(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state);

		virtual void increaseVelocity(float timeSinceLast);
		virtual void increaseVelocity(float timeSinceLast, Ogre::Vector3 vVel);
		virtual void decreaseVelocity(float timeSinceLast);

		virtual void increasePressure(float timeSinceLast);
		virtual void decreasePressure(float timeSinceLast);

		virtual void addImpulse(float timeSinceLast);

		virtual void notifyShiftKey(bool shift);

		virtual void clearActiveCell(void);
		virtual void resetState(void);

		virtual void traverseActiveCellZNegative(void);
		virtual void traverseActiveCellXPositive(void);
		virtual void traverseActiveCellZPositive(void);
		virtual void traverseActiveCellXNegative(void);

		virtual void rotateVelocityClockwise(float timeSinceLast);
		virtual void rotateVelocityCounterClockwise(float timeSinceLast);

		virtual void togglePressureGradientIndicators(void);
	};
}