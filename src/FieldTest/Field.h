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

	protected:
		virtual void createGrid(void);
		virtual void createCells(void);

	public:
		Field(GameEntityManager* geMgr);
		~Field();

		std::map<std::pair<int, int>, Cell*> mCells;

		Cell* getCell(int x, int y);
	};
}