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
		int cellCount;
		int mColumnCount;
		int mRowCount;
		GameEntityManager* mGameEntityManager;

		MyThirdOgre::GameEntity* mGridEntity;
		MovableObjectDefinition* mGridLineMoDef;

	protected:
		virtual void createGrid(void);

	public:
		Field(GameEntityManager* geMgr);
		~Field();

		std::map<std::pair<int, int>, Cell*> cells;

		Cell* getCell(int x, int y);
	};
}