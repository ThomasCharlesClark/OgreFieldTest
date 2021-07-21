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

		bool mManualVelocityAdjustmentSpeedModifier;
		GameEntityManager* mGameEntityManager;
		GameEntity* mGridEntity;
		MovableObjectDefinition* mGridLineMoDef;
		std::map<CellCoord, Cell*> mCells;

		Cell* mActiveCell;

	protected:
		virtual void createGrid(void);

		virtual void createCells(void);

	public:
		Field(GameEntityManager* geMgr);
		~Field();

		Cell* getCell(CellCoord coords);

		virtual void update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex);

		// we want to be updating a "shadow" of all of our cells' transforms 
		// this shadow needs to be processed through a number of different functions 
		// before the end result is actually rendered

		virtual float getPressureDerivativeX(void);
		virtual float getPressureDerivativeY(void);

		virtual void advect(float timeSinceLast, std::map<CellCoord, CellState> &state);

		virtual void increaseVelocity(float timeSinceLast);
		virtual void decreaseVelocity(float timeSinceLast);

		virtual void notifyShift(bool shift);

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