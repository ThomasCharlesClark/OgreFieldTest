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
		int mColumnCount;
		int mRowCount;
		GameEntityManager* mGameEntityManager;
		GameEntity* mGridEntity;
		MovableObjectDefinition* mGridLineMoDef;
		std::map<std::pair<int, int>, Cell*> mCells;

		Cell* mActiveCell;

	protected:
		virtual void createGrid(void);

		virtual void createCells(void);

	public:
		Field(GameEntityManager* geMgr);
		~Field();

		Cell* getCell(std::pair<int, int> coords);

		virtual void update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex);

		// we want to be updating a "shadow" of all of our cells' transforms 
		// this shadow needs to be processed through a number of different functions 
		// before the end result is actually rendered

		virtual float getPressureDerivativeX(void);
		virtual float getPressureDerivativeY(void);

		virtual void advect(float timeSinceLast, std::map<std::pair<int, int>, CellState> &state);

		virtual void spinLeft();
		virtual void spinRight();

		virtual void traverseActiveCellZNegative();
		virtual void traverseActiveCellXPositive();
		virtual void traverseActiveCellZPositive();
		virtual void traverseActiveCellXNegative();
	};
}