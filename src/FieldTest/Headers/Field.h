#pragma once
#include <vector>

#include "Cell.h"
#include "OgreSceneNode.h"
#include "OgreManualObject2.h"
#include "GameEntity.h"
#include "GameEntityManager.h"

#include <map>

namespace MyThirdOgre 
{
	class Field
	{
		int mScale;
		int mCellCount;
		int mRowCount;
		int mColumnCount;
		int mLayerCount;
		float mBaseManualVelocityAdjustmentSpeed;
		float mBoostedManualVelocityAdjustmentSpeed;

		float mMaxCellPressure;
		float mKinematicViscosity;
		float mFluidDensity;

		bool mManualVelocityAdjustmentSpeedModifier;
		GameEntityManager* mGameEntityManager;
		GameEntity* mGridEntity;
		MovableObjectDefinition* mGridLineMoDef;
		std::map<CellCoord, Cell*> mCells;

		Cell* mActiveCell;

		bool mIsRunning;
		int mPressureSpreadHalfWidth;

	protected:
		virtual void createGrid(void);

		virtual void createCells(void);

	public:
		Field(GameEntityManager* geMgr, int scale, int columnCount, int rowCount);
		~Field();

		Cell* getCell(CellCoord coords);

		virtual void update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex);

		// we want to be updating a "shadow" of all of our cells' transforms 
		// this shadow needs to be processed through a number of different functions 
		// before the end result is actually rendered

		bool getIsRunning(void) { return mIsRunning; }
		virtual void setIsRunning(bool isRunning);

		virtual void advect(float timeSinceLast, std::map<CellCoord, CellState> &state);
		virtual void diffuse(float timeSinceLast, std::map<CellCoord, CellState>& state);
		virtual void addForces(float timeSinceLast, std::map<CellCoord, CellState>& state);
		virtual void computePressureGradient(float timeSinceLast, std::map<CellCoord, CellState>& state);

		//virtual void jacobi(
		//	const CellCoord& coord,
		//	Ogre::Vector3& xNew,
		//	float alpha,
		//	int rBeta,
		//	const std::map<CellCoord, CellState>& x,
		//	const std::map<CellCoord, CellState>& b);


		virtual void increaseVelocity(float timeSinceLast);
		virtual void decreaseVelocity(float timeSinceLast);

		virtual void increasePressure(float timeSinceLast);
		virtual void decreasePressure(float timeSinceLast);

		virtual void notifyShiftKey(bool shift);

		virtual void clearActiveCell(void);
		virtual void resetState(void);

		virtual void traverseActiveCellZNegative(void);
		virtual void traverseActiveCellXPositive(void);
		virtual void traverseActiveCellZPositive(void);
		virtual void traverseActiveCellXNegative(void);

		virtual void rotateVelocityClockwise(float timeSinceLast);
		virtual void rotateVelocityCounterClockwise(float timeSinceLast);
	};
}