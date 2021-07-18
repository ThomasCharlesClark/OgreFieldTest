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

		virtual void increaseVelocityX(float timeSinceLast);
		virtual void decreaseVelocityX(float timeSinceLast);
		virtual void increaseVelocityZ(float timeSinceLast);
		virtual void decreaseVelocityZ(float timeSinceLast);

		void notifyShift(bool shift);

		virtual void traverseActiveCellZNegative();
		virtual void traverseActiveCellXPositive();
		virtual void traverseActiveCellZPositive();
		virtual void traverseActiveCellXNegative();
	};
}