#pragma once
#include <vector>

#include "Cell.h"
#include "OgreSceneNode.h"
#include "OgreManualObject2.h"
#include "GameEntity.h"
#include "GameEntityManager.h"

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

		Cell* getCell(int x, int y);

		virtual void update(float timeSinceLast);

		// we want to be updating a "shadow" of all of our cells' transforms 
		// this shadow needs to be processed through a number of different functions 
		// before the end result is actually rendered

	};
}