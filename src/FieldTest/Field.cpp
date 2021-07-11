#include <stdexcept>
#include <sstream>

#include "Field.h"

#include "Ogrestd\vector.h"

Field::Field(Ogre::SceneManager* scnMgr) 
{
	columnCount = 22;
	rowCount = 22;
	sceneManager = scnMgr;

	createGrid();
}

Field::~Field() {
	for (auto i = cells.begin(); i != cells.end(); i++) {
		delete i->second;
	}
}

void Field::createGrid(void) {

	Ogre::ManualObject* gridObject = sceneManager->createManualObject();

	gridObject->begin("BaseWhite", Ogre::OT_LINE_LIST);

	int xPointCounter = 0;
	int zPointCounter = 0;

	// rows go horizontally
	for (int i = 0; i < rowCount; i++) {
		gridObject->position(i, 0.0f, 0.0f);
		gridObject->position(i, 0.0f, columnCount);
		gridObject->line(xPointCounter, i + 1);
		xPointCounter += 2;
	}

	// columns go vertically

	gridObject->end();

	Ogre::SceneNode* sceneNodeLines = sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->
		createChildSceneNode(Ogre::SCENE_DYNAMIC);

	sceneNodeLines->attachObject(gridObject);
	sceneNodeLines->scale(1.0f, 1.0f, 1.0f);
	sceneNodeLines->translate(0.0f, 0.0f, 0.0f, Ogre::SceneNode::TS_LOCAL);
}

Cell* Field::getCell(int x, int y) 
{
	if (y > 0)
		y = -y;

	auto iter = cells.find(std::pair<int, int>(x, y));
	
	if (iter != cells.end())
		return iter->second;
	else {
		return 0;
	}
}

std::vector<Cell*> Field::getNeighbours(Ogre::Vector3 pos)
{
	return std::vector<Cell*>();
}
