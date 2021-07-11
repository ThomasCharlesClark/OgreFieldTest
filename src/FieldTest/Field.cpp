#include <stdexcept>
#include <sstream>
#include <vector>

#include "Field.h"

#include "OgreVector3.h"

using namespace MyThirdOgre;

namespace MyThirdOgre 
{
	Field::Field(GameEntityManager* geMgr)
	{
		mColumnCount = 22;
		mRowCount = 22;
		mGameEntityManager = geMgr;

		createGrid();
	}

	Field::~Field() {
		delete mGridLineMoDef;
		mGridLineMoDef = 0;

		for (auto i = cells.begin(); i != cells.end(); i++) {
			delete i->second;
		}
	}

	void Field::createGrid(void) {

		mGridLineMoDef = new MovableObjectDefinition();
		mGridLineMoDef->meshName = "";
		mGridLineMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
		mGridLineMoDef->moType = MoTypeStaticManualLineList;

		ManualObjectLineListDefinition gridLineList;

		gridLineList.points = std::vector<Ogre::Vector3>{};
		gridLineList.lines = std::vector<std::pair<int, int>>{};

		int pointCounter = 0;

		for (int i = 0; i < mColumnCount + 1; i++) {
			gridLineList.points.push_back(Ogre::Vector3(i, 0, 0));
			gridLineList.points.push_back(Ogre::Vector3(i, 0, -mColumnCount));
			gridLineList.lines.push_back({ pointCounter, pointCounter + 1 });
			pointCounter += 2;
		}

		for (int i = 0; i < mRowCount + 1; i++) {
			gridLineList.points.push_back(Ogre::Vector3(0, 0, -i));
			gridLineList.points.push_back(Ogre::Vector3(mRowCount, 0, -i));
			gridLineList.lines.push_back({ pointCounter, pointCounter + 1 });
			pointCounter += 2;
		}

		mGridEntity = mGameEntityManager->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_STATIC,
			mGridLineMoDef,
			Ogre::Vector3::ZERO,
			Ogre::Quaternion::IDENTITY,
			Ogre::Vector3::UNIT_SCALE,
			"UnlitBlack",
			gridLineList
		);

		//Ogre::ManualObject* gridObject = mGameEntityManager->createManualObject();

		//gridObject->begin("BaseWhite", Ogre::OT_LINE_LIST);

		//int xPointCounter = 0;
		//int zPointCounter = 0;

		//// rows go horizontally
		//for (int i = 0; i < mRowCount; i++) {
		//	gridObject->position(i, 0.0f, 0.0f);
		//	gridObject->position(i, 0.0f, mColumnCount);
		//	gridObject->line(xPointCounter, i + 1);
		//	xPointCounter += 2;
		//}

		//// columns go vertically

		//gridObject->end();

		//Ogre::SceneNode* sceneNodeLines = mGameEntityManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->
		//	createChildSceneNode(Ogre::SCENE_DYNAMIC);

		//sceneNodeLines->attachObject(gridObject);
		//sceneNodeLines->scale(1.0f, 1.0f, 1.0f);
		//sceneNodeLines->translate(0.0f, 0.0f, 0.0f, Ogre::SceneNode::TS_LOCAL);
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
}