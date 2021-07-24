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
	Field::Field(GameEntityManager* geMgr, int scale, int columnCount, int rowCount) :
		mGameEntityManager(geMgr),
		mScale(scale),
		mColumnCount(columnCount),
		mRowCount(rowCount),
		mLayerCount(1),
		mCells(std::map<CellCoord, Cell*> { }),
		mActiveCell(0),
		mBaseManualVelocityAdjustmentSpeed(2.0f),
		mBoostedManualVelocityAdjustmentSpeed(10.0f),
		mManualVelocityAdjustmentSpeedModifier(false),
		mMaxCellPressure(5.0f),		// 1 atmosphere...?
		mKinematicViscosity(0.40f), // total guess
		mFluidDensity(9.97f),		// water density = 997kg/m^3
		mIsRunning(true),
		mPressureSpreadHalfWidth(2)
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

	void Field::setIsRunning(bool isRunning) 
	{
		mIsRunning = isRunning;
	}

	void Field::update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex)
	{
		if (mIsRunning) {

			std::map<CellCoord, CellState> state;

			for (auto& c : mCells) {
				//if (!c.second->getIsBoundary()) {
					state.insert({ c.second->getCellCoords() , *c.second->getState() });
				//}
			}

			advect(timeSinceLast, state);

			//diffuse(timeSinceLast, state);

			//addForces(timeSinceLast, state);

			//computePressure(timeSinceLast, state);

			computePressureGradient(timeSinceLast, state);

			for (auto& c : state) {
				if (!mCells.at(c.first)->getIsBoundary()) {

					auto pressureTerm = - (1 / mFluidDensity) * c.second.vPressureGradient;

					//mCells.at(c.first)->setVelocity(c.second.vVel + pressureTerm);
					mCells.at(c.first)->setPressureGradient(c.second.vPressureGradient);
				}
			}

			for (auto& cell : mCells) {
				if (!cell.second->getIsBoundary())
					cell.second->updateTransforms(timeSinceLast, currentTransformIndex, previousTransformIndex);
			}
		}
	}

	void Field::advect(float timeSinceLast, std::map<CellCoord, CellState>& state)
	{
		for (auto& cell : state) {

			if (!cell.second.bIsBoundary) {

				auto cVel = cell.second.vVel;

				auto vPos = cell.second.vPos - (timeSinceLast * cVel);

				int ax = 0, az = 0,
					bx = 0, bz = 0,
					cx = 0, cz = 0,
					dx = 0, dz = 0;

				ax = floor(vPos.x);
				az = ceil(vPos.z);

				bx = floor(vPos.x);
				bz = floor(vPos.z);

				cx = ceil(vPos.x);
				cz = floor(vPos.z);

				dx = ceil(vPos.x);
				dz = ceil(vPos.z);

				if (az == bz)
					bz--;

				if (ax == dx)
					dx++;

				if (dz == cz)
					cz--;

				if (bx == cx)
					cx++;

				//int ax = 0, az = 0,
				//	bx = 0, bz = 0,
				//	cx = 0, cz = 0,
				//	dx = 0, dz = 0;

				//ax = floor(vPos.x);
				//az = ceil(vPos.z);

				//bx = floor(vPos.x);
				//bz = floor(vPos.z);

				//cx = ceil(vPos.x);
				//cz = floor(vPos.z);

				//dx = ceil(vPos.x);
				//dz = ceil(vPos.z);

				auto a = state.find({ ax, 0, abs(az) });
				auto b = state.find({ bx, 0, abs(bz) });
				auto c = state.find({ cx, 0, abs(cz) });
				auto d = state.find({ dx, 0, abs(dz) });

				if (a != state.end() &&
					b != state.end() &&
					c != state.end() &&
					d != state.end())
				{
					assert(a != b);
					assert(a != c);
					assert(a != d);

					assert(b != c);
					assert(b != d);

					assert(c != d);

					Ogre::Vector3 vNewVel = velocityBiLerp(

						a->second.vPos,
						b->second.vPos,
						c->second.vPos,
						d->second.vPos,

						a->second.vVel,
						b->second.vVel,
						c->second.vVel,
						d->second.vVel,

						vPos.x,
						vPos.z
					);

					//if (!cell.second.bActive)
					cell.second.vVel = vNewVel;
				}
			}
		}
	}

	void Field::diffuse(float timeSinceLast, std::map<CellCoord, CellState>& state) 
	{
		//int iterations = 20;

		//for (auto& cell : state) {
		//	if (!cell.second.bIsBoundary) {
		//		// iterate jacobi function over cell rBeta times
		//		for (int i = 0; i < iterations; i++) {
		//			Ogre::Vector3 output;
		//			jacobi(cell.first, output, mKinematicViscosity, 2, state, state);
		//			cell.second.vVel = output;
		//		}
		//	}
		//}
	}

	void Field::addForces(float timeSinceLast, std::map<CellCoord, CellState>& state)
	{

	}

	void Field::computePressureGradient(float timeSinceLast, std::map<CellCoord, CellState>& state)
	{
		for (auto& c : state) {
			if (!c.second.bIsBoundary) {

				auto left = c.first + CellCoord(-1, 0, 0);
				auto right = c.first + CellCoord(1, 0, 0);
				auto top = c.first + CellCoord(0, 0, 1);
				auto bottom = c.first + CellCoord(0, 0, -1);

				auto l = state.find(left);// .vVel;
				auto r = state.find(right);// .vVel;
				auto t = state.find(top);// .vVel;
				auto b = state.find(bottom);// .vVel;

				//if (l != state.end() && r != state.end() && t != state.end() && b != state.end()) {
					Ogre::Vector3 gradient = Ogre::Vector3(
						(l->second.rPressure - r->second.rPressure) / 2, 
						0.0f, 
						(t->second.rPressure - b->second.rPressure) / 2);

					c.second.vPressureGradient = gradient;
					//c.second.rPressure = c.second.vPressureGradient.length();
				//}
			}
		}
	}

	void Field::notifyShiftKey(bool shift) {
		mManualVelocityAdjustmentSpeedModifier = shift;
	}

	void Field::rotateVelocityClockwise(float timeSinceLast) {
		float rotationRadians = Ogre::Math::DegreesToRadians(
			50 
			* timeSinceLast
			* mBaseManualVelocityAdjustmentSpeed
			* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

		auto q = Ogre::Quaternion(Ogre::Math::Cos(rotationRadians), 0.0f, -Ogre::Math::Sin(rotationRadians), 0.0f);

		q.normalise();

		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {
			
				mActiveCell->setVelocity(q * mActiveCell->getVelocity());

				for (int i = -2; i < 3; i++) {
					for (int j = -2; j < 3; j++) {
						auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
						if (cNeighbour != mCells.end()) {
							if (!cNeighbour->second->getIsBoundary()) {

								//float rotationRadians = Ogre::Math::DegreesToRadians(
								//	50 * (1 / (mActiveCell->getState()->vPos - cNeighbour->second->getState()->vPos).length())
								//	* timeSinceLast
								//	* mBaseManualVelocityAdjustmentSpeed
								//	* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

								//auto q = Ogre::Quaternion(Ogre::Math::Cos(rotationRadians), 0.0f, -Ogre::Math::Sin(rotationRadians), 0.0f);

								//q.normalise();

								cNeighbour->second->setVelocity(q * cNeighbour->second->getVelocity());
							}
						}
					}
				}
			}
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary())
					c.second->setVelocity(q * c.second->getVelocity());
			}
		}
	}

	void Field::rotateVelocityCounterClockwise(float timeSinceLast) {
		float rotationRadians = Ogre::Math::DegreesToRadians(
			50
			* timeSinceLast
			* mBaseManualVelocityAdjustmentSpeed
			* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

		auto q = Ogre::Quaternion(Ogre::Math::Cos(rotationRadians), 0.0f, Ogre::Math::Sin(rotationRadians), 0.0f);

		q.normalise();

		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {
				mActiveCell->setVelocity(q * mActiveCell->getVelocity());

				for (int i = -2; i < 3; i++) {
					for (int j = -2; j < 3; j++) {
						auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
						if (cNeighbour != mCells.end()) {
							if (!cNeighbour->second->getIsBoundary()) {

								//float rotationRadians = Ogre::Math::DegreesToRadians(
								//	50
								//	* timeSinceLast
								//	* mBaseManualVelocityAdjustmentSpeed
								//	* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

								//auto q = Ogre::Quaternion(Ogre::Math::Cos(rotationRadians), 0.0f, Ogre::Math::Sin(rotationRadians), 0.0f);

								q.normalise();

								cNeighbour->second->setVelocity(q * cNeighbour->second->getVelocity());
							}
						}
					}
				}
			}
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary())
					c.second->setVelocity(q * c.second->getVelocity());
			}
		}
	}

	void Field::increaseVelocity(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {

				if (mActiveCell->getVelocity() == Ogre::Vector3::ZERO) {
					mActiveCell->setVelocity(Ogre::Vector3(0, 0, -0.1f));
				}

				auto vCurr = mActiveCell->getVelocity();

				auto vNew = vCurr * (1 + timeSinceLast
					* mBaseManualVelocityAdjustmentSpeed
					* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));


				for (int i = -2; i < 3; i++) {
					for (int j = -2; j < 3; j++) {
						auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
						if (cNeighbour != mCells.end()) {
							if (!cNeighbour->second->getIsBoundary()) {
								cNeighbour->second->setVelocity(vNew * ( 1 / (mActiveCell->getState()->vPos - cNeighbour->second->getState()->vPos).length() ));
							}
						}
					}
				}


				mActiveCell->setVelocity(vNew);
			}
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary()) {

					auto vCurr = c.second->getVelocity();

					auto vNew = vCurr * (1 + timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					c.second->setVelocity(vNew);
				}
			}
		}
	}

	void Field::decreaseVelocity(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {
				if (mActiveCell->getVelocity() == Ogre::Vector3::ZERO) {
					mActiveCell->setVelocity(Ogre::Vector3(0, 0, 0.1f));
				}

				auto vCurr = mActiveCell->getVelocity();

				auto vNew = vCurr * (1 - timeSinceLast
					* mBaseManualVelocityAdjustmentSpeed
					* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

				for (int i = -2; i < 3; i++) {
					for (int j = -2; j < 3; j++) {
						auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
						if (cNeighbour != mCells.end()) {
							if (!cNeighbour->second->getIsBoundary()) {
								cNeighbour->second->setVelocity(vNew * (1 / (mActiveCell->getState()->vPos - cNeighbour->second->getState()->vPos).length()));
							}
						}
					}
				}

				mActiveCell->setVelocity(vNew);
			}
		}
		else {
			for (auto& c : mCells) {
				if (!c.second->getIsBoundary()) {

					auto vCurr = c.second->getVelocity();

					auto vNew = vCurr * (1 - timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					c.second->setVelocity(vNew);
				}
			}
		}
	}

	void Field::increasePressure(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {

				if (mActiveCell->getPressure() < mMaxCellPressure) {

					if (mActiveCell->getPressure() == 0) {
						mActiveCell->setPressure(0.001f);
					}

					auto rCurrentPressure = mActiveCell->getPressure();

					auto rNewPressure = rCurrentPressure * (1 + timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					for (int i = -mPressureSpreadHalfWidth; i < mPressureSpreadHalfWidth + 1; i++) {
						for (int j = -mPressureSpreadHalfWidth; j < mPressureSpreadHalfWidth + 1; j++) {
							auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
							if (cNeighbour != mCells.end()) {
								if (!cNeighbour->second->getIsBoundary()) {
									cNeighbour->second->setPressure(rNewPressure * (1 / (mActiveCell->getState()->vPos - cNeighbour->second->getState()->vPos).length()));
								}
							}
						}
					}


					mActiveCell->setPressure(rNewPressure);
				}
			}
		}
	}

	void Field::decreasePressure(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {
				if (mActiveCell->getPressure() == 0) {
					return;
				}

				auto rCurrentPressure = mActiveCell->getPressure();

				auto rNewPressure = rCurrentPressure * (1.0 - timeSinceLast
					* mBaseManualVelocityAdjustmentSpeed
					* (1 - mManualVelocityAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

				for (int i = -mPressureSpreadHalfWidth; i < mPressureSpreadHalfWidth + 1; i++) {
					for (int j = -mPressureSpreadHalfWidth; j < mPressureSpreadHalfWidth + 1; j++) {
						auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
						if (cNeighbour != mCells.end()) {
							if (!cNeighbour->second->getIsBoundary()) {
								cNeighbour->second->setPressure(rNewPressure * (1 / (mActiveCell->getState()->vPos - cNeighbour->second->getState()->vPos).length()));
							}
						}
					}
				}

				mActiveCell->setPressure(rNewPressure);
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