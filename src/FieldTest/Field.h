#pragma once
#include <vector>
#include "Cell.h"
#include "OgreSceneNode.h"
#include "OgreManualObject2.h"

class Field
{
	int cellCount;
	int columnCount;
	int rowCount;
	Ogre::SceneManager* sceneManager;

	Ogre::ManualObject* gridObject;
	Ogre::SceneNode* gridSceneNode;

protected:
	virtual void createGrid(void);

public: 
	Field(Ogre::SceneManager* scnMgr);
	~Field();

	std::map<std::pair<int,int>, Cell*> cells;

	Cell* getCell(int x, int y);

	std::vector<Cell*> getNeighbours(Ogre::Vector3 pos);
};

