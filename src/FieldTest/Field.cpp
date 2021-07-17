#include <stdexcept>
#include <sstream>
#include <vector>

#include "Headers/Cell.h"
#include "Headers/Field.h"

#include "Headers/MyMath.h"

#include "OgreVector3.h"

#include <map>

using namespace MyThirdOgre;

namespace MyThirdOgre
{
	Field::Field(GameEntityManager* geMgr) :
		mGameEntityManager(geMgr),
		mScale(1),
		mColumnCount(22),
		mRowCount(22),
		mCells(std::map<std::pair<int, int>, Cell*> { }),
		mActiveCell(0)

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
					{ i, j },
					new Cell(i, j, mColumnCount, mRowCount, mGameEntityManager)
				});
			}
		}
	}

	Cell* Field::getCell(std::pair<int, int> coords)
	{
		auto iter = mCells.find(coords);

		if (iter != mCells.end())
			return iter->second;
		else {
			return 0;
		}
	}

	void Field::advect(float timeSinceLast, std::map<std::pair<int, int>, CellState> &state)
	{
		for (auto& cell : state) {
			auto vPosBackInTime = cell.second.vPos + (-cell.second.vVel * (timeSinceLast * 10));
			int ax, az, 
				bx, bz,
				cx, cz,
				dx, dz;

			ax = floor(vPosBackInTime.x);
			az = ceil(vPosBackInTime.z);

			bx = floor(vPosBackInTime.x);
			bz = floor(vPosBackInTime.z);

			cx = ceil(vPosBackInTime.x);
			cz = floor(vPosBackInTime.z);

			dx = ceil(vPosBackInTime.x);
			dz = ceil(vPosBackInTime.z);

			auto a = state.find({ ax, az });
			auto b = state.find({ bx, bz });
			auto c = state.find({ cx, cz });
			auto d = state.find({ dx, dz });

			if (a != state.end() &&
				b != state.end() &&
				c != state.end() &&
				d != state.end())
			{
				Ogre::Vector3 vNewVel = velocityBiLerp(
					a->second.vPos,
					b->second.vPos,
					c->second.vPos,
					d->second.vPos,
					a->second.vVel,
					b->second.vVel,
					c->second.vVel,
					d->second.vVel,
					vPosBackInTime.x, 
					vPosBackInTime.z
				);

				assert(cell.second.vVel != vNewVel); // just checking - velocities shouldn't normally remain the same following quad bilerp

				cell.second.vVel = vNewVel;
			}
		}
	}

	void Field::update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex)
	{
		std::map<std::pair<int, int>, CellState> state;

		for (auto c : mCells)
			if (!c.second->getIsBoundary())
				state.insert({ { c.second->getXIndex(), c.second->getZIndex() }, c.second->getState() });

		advect(timeSinceLast, state);

		for (auto advectedCell : state) {
			mCells.at(advectedCell.first)->setVelocity(advectedCell.second.vVel);
		}

		for (auto cell : mCells) {
			if (!cell.second->getIsBoundary())
				cell.second->update(timeSinceLast, currentTransformIndex, previousTransformIndex);
		}

		if (mActiveCell)
			mActiveCell->setActive();
	}

	float Field::getPressureDerivativeX(void) {
		float result = 0.0f;

		return result;
	}

	float Field::getPressureDerivativeY(void) {
		float result = 0.0f;

		return result;
	}

	void Field::spinLeft(void) {
		for (auto& c : mCells) {
			if (!c.second->getIsBoundary())
				c.second->setVelocity(Ogre::Quaternion(Ogre::Radian(0.5f), Ogre::Vector3(0, 1, 0)) * c.second->getVelocity());
		}
	}

	void Field::spinRight(void) {
		for (auto& c : mCells) {
			if (!c.second->getIsBoundary())
				c.second->setVelocity(Ogre::Quaternion(Ogre::Radian(-0.5f), Ogre::Vector3(0, 1, 0)) * c.second->getVelocity());
		}
	}

	void Field::traverseActiveCellZNegative(void) {
		/*if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 1 })->second;
		}*/
	}

	void Field::traverseActiveCellXPositive(void) {
		/*if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 1 })->second;
		}*/
	}

	void Field::traverseActiveCellZPositive(void) {
		/*if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 1 })->second;
		}*/
	}

	void Field::traverseActiveCellXNegative(void) {
		/*if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 1 })->second;
		}*/
	}
}