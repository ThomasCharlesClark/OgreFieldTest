#include <stdexcept>
#include <sstream>
#include <vector>

#include "Field.h"

#include "OgreVector3.h"

using namespace MyThirdOgre;

namespace MyThirdOgre 
{
	Field::Field(GameEntityManager* geMgr) :
		mGameEntityManager ( geMgr ),
		mScale( 1 ),
		mColumnCount ( 22 ),
		mRowCount ( 22 ),
		mCells ( std::map<std::pair<int, int>, Cell*> { } )

	{
		createGrid();

		createCells();
	}

	Field::~Field() {
		delete mGridLineMoDef;
		mGridLineMoDef = 0;

		for (auto i = mCells.begin(); i != mCells.end(); i++) {
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

		float adjuster = 0.5f;

		for (int i = 0; i < mColumnCount + 1; i++) {
			gridLineList.points.push_back(Ogre::Vector3((i - adjuster) * mScale, 0, adjuster));
			gridLineList.points.push_back(Ogre::Vector3((i - adjuster) * mScale, 0, -(mColumnCount - adjuster) * mScale));
			gridLineList.lines.push_back({ pointCounter, pointCounter + 1 });
			pointCounter += 2;
		}

		for (int i = 0; i < mRowCount + 1; i++) {
			gridLineList.points.push_back(Ogre::Vector3(-adjuster, 0, -(i - adjuster) * mScale));
			gridLineList.points.push_back(Ogre::Vector3((mRowCount - adjuster) * mScale, 0, -(i - adjuster) * mScale));
			gridLineList.lines.push_back({ pointCounter, pointCounter + 1 });
			pointCounter += 2;
		}

		mGridEntity = mGameEntityManager->addGameEntity(
			Ogre::SceneMemoryMgrTypes::SCENE_STATIC,
			mGridLineMoDef,
			"UnlitBlack",
			gridLineList,
			Ogre::Vector3::ZERO,
			Ogre::Quaternion::IDENTITY,
			Ogre::Vector3::UNIT_SCALE
		);
	}

	void Field::createCells(void) {
		for (int i = 0; i < mColumnCount; i++) {
			for (int j = 0; j < mRowCount; j++) {
				mCells.insert({ 
					{ i, j, }, 
					new Cell(i, j, mColumnCount, mRowCount, mGameEntityManager) 
				});
			}
		}
	}

	Cell* Field::getCell(int x, int y)
	{
		if (y > 0)
			y = -y;

		auto iter = mCells.find(std::pair<int, int>(x, y));

		if (iter != mCells.end())
			return iter->second;
		else {
			return 0;
		}
	}
}