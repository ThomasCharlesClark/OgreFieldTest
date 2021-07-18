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
		mLayerCount(1),
		mCells(std::map<CellCoord, Cell*> { }),
		mActiveCell(0),
		mBaseManualVelocityAdjustmentSpeed(1.0f),
		mBoostedManualVelocityAdjustmentSpeed(5.0f),
		mManualVelocityAdjustmentSpeedModifier(false),
		mMaxCellPressure(1.0f)
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
		assert(mLayerCount == 1);
		for (int i = 0; i < mColumnCount; i++) {
			for (int j = 0; j < mRowCount; j++) {
				for (int k = 0; k < mLayerCount; k++) {
					auto c = new Cell(i, j, k, mColumnCount, mRowCount, mMaxCellPressure, mGameEntityManager);
					mCells.insert({
						c->getCellCoords(),
						c
					});
				}
			}
		}
	}

	Cell* Field::getCell(CellCoord coords)
	{
		auto iter = mCells.find(coords);

		if (iter != mCells.end())
			return iter->second;
		else {
			return 0;
		}
	}

	void Field::advect(float timeSinceLast, std::map<CellCoord, CellState> &state)
	{
		for (auto& cell : state) {
			auto cVel = cell.second.vVel == Ogre::Vector3::ZERO ? Ogre::Vector3(0, 0, 1) : cell.second.vVel;
			auto vPosBackInTime = cell.second.vPos - (cVel * (timeSinceLast * 10));
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

			auto a = state.find({ ax, 0, -az });
			auto b = state.find({ bx, 0, -bz });
			auto c = state.find({ cx, 0, -cz });
			auto d = state.find({ dx, 0, -dz });

			//if (a == state.end())
			//	a = state.find({ cell.first.mIndexX, 0, cell.first.mIndexZ - 1 });

			//if (b == state.end())
			//	b = state.find({ cell.first.mIndexX + 1, 0, cell.first.mIndexZ });

			//if (c == state.end())
			//	c = state.find({ cell.first.mIndexX, 0, cell.first.mIndexZ + 1 });

			//if (d == state.end())
			//	d = state.find({ cell.first.mIndexX - 1, 0, cell.first.mIndexZ });

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

				//auto nextPos = cell.second.vPos + (vNewVel * timeSinceLast * 100);

				//if (nextPos.x < 1)
				//	vNewVel = -vNewVel.reflect(Ogre::Vector3::UNIT_X);
				//if (nextPos.x > mColumnCount - 1)
				//	vNewVel = -vNewVel.reflect(Ogre::Vector3(-1, 0, 0));

				//if (nextPos.z > -1)
				//	vNewVel = -vNewVel.reflect(Ogre::Vector3(0, 0, -1));
				//if (nextPos.z < -mRowCount + 1)
				//	vNewVel = -vNewVel.reflect(Ogre::Vector3(0, 0, 1));

				if (vNewVel != Ogre::Vector3::ZERO)
					cell.second.vVel = vNewVel;
			}
		}
	}

	void Field::update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex)
	{
		std::map<CellCoord, CellState> state;

		for (auto c : mCells)
			if (!c.second->getIsBoundary()) {
				state.insert({ c.second->getCellCoords() , c.second->getState() });
			}

		advect(timeSinceLast, state);

		for (auto advectedCell : state) {
			mCells.at(advectedCell.first)->setVelocity(advectedCell.second.vVel);
		}

		for (auto cell : mCells) {
			if (!cell.second->getIsBoundary())
				cell.second->updateTransforms(timeSinceLast, currentTransformIndex, previousTransformIndex);
		}
	}

	float Field::getPressureDerivativeX(void) {
		float result = 0.0f;

		return result;
	}

	float Field::getPressureDerivativeY(void) {
		float result = 0.0f;

		return result;
	}

	void Field::notifyShift(bool shift) {
		mManualVelocityAdjustmentSpeedModifier = shift;
	}

	void Field::increaseVelocityX(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary())
				mActiveCell->setVelocity(mActiveCell->getVelocity() +
					Ogre::Vector3(
						timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed),
						0.0f,
						0.0f)
				);
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary())
					c.second->setVelocity(c.second->getVelocity() +
						Ogre::Vector3(
							timeSinceLast
							* mBaseManualVelocityAdjustmentSpeed
							* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed),
							0.0f,
							0.0f)
					);
			}
		}
	}

	void Field::decreaseVelocityX(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary())
				mActiveCell->setVelocity(mActiveCell->getVelocity() -
					Ogre::Vector3(
						timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed),
						0.0f,
						0.0f)
				);
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary())
					c.second->setVelocity(c.second->getVelocity() -
						Ogre::Vector3(
							timeSinceLast
							* mBaseManualVelocityAdjustmentSpeed
							* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed),
							0.0f,
							0.0f)
					);
			}
		}
	}

	void Field::increaseVelocityZ(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary())
				if (mActiveCell->getVelocity() == Ogre::Vector3::ZERO) {
					mActiveCell->setVelocity(Ogre::Vector3(0, 0, 0.1f));
				}
				mActiveCell->setVelocity(mActiveCell->getVelocity() +
					Ogre::Vector3(
						0.0f,
						0.0f,
						timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed))
				);
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary())
					c.second->setVelocity(c.second->getVelocity() +
						Ogre::Vector3(
							0.0f,
							0.0f,
							timeSinceLast
							* mBaseManualVelocityAdjustmentSpeed
							* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed))
					);
			}
		}
	}

	void Field::decreaseVelocityZ(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary())
				mActiveCell->setVelocity(mActiveCell->getVelocity() -
					Ogre::Vector3(
						0.0f,
						0.0f,
						timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed))
				);
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary())
					c.second->setVelocity(c.second->getVelocity() -
						Ogre::Vector3(
							0.0f,
							0.0f,
							timeSinceLast
							* mBaseManualVelocityAdjustmentSpeed
							* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed))
					);
			}
		}
	}

	void Field::clearActiveCell(void) {
		if (mActiveCell) {
			mActiveCell->unsetActive();
			mActiveCell = 0;
		}
	}

	void Field::traverseActiveCellZNegative(void) {
		if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 0, 1 })->second;
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexZ += 1;
			if (coords.mIndexZ > mRowCount - 2)
				coords.mIndexZ = 1;
			mActiveCell = mCells.find(coords)->second;
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellXPositive(void) {
		if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 0, 1 })->second;
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexX += 1;
			if (coords.mIndexX > mColumnCount - 2)
				coords.mIndexX = 1;
			mActiveCell = mCells.find(coords)->second;
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellZPositive(void) {
		if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 0, 1 })->second;
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexZ -= 1;
			if (coords.mIndexZ < 1)
				coords.mIndexZ = mRowCount - 2;
			mActiveCell = mCells.find(coords)->second;
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellXNegative(void) {
		if (!mActiveCell) {
			mActiveCell = mCells.find({ 1, 0, 1 })->second;
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexX -= 1;
			if (coords.mIndexX < 1)
				coords.mIndexX = mColumnCount - 2;
			mActiveCell = mCells.find(coords)->second;
			mActiveCell->setActive();
		}
	}

	void Field::resetState(void) {
		for (auto& c : mCells) {
			if (!c.second->getIsBoundary())
				c.second->resetState();
		}
	}
}