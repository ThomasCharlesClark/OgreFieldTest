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
		mHalfScale((float)mScale/2),
		mColumnCount(columnCount),
		mRowCount(rowCount),
		mGridLineMoDef(0),
		mGridEntity(0),
		mLayerCount(1),
		mCells(std::unordered_map<CellCoord, Cell*> { }),
		mActiveCell(0),
		mBaseManualVelocityAdjustmentSpeed(0.5f),
		mBoostedManualVelocityAdjustmentSpeed(1.5f),
		mBaseManualPressureAdjustmentSpeed(0.01f),
		mBoostedManualPressureAdjustmentSpeed(0.05f),
		mManualAdjustmentSpeedModifier(false),
		mMinCellPressure(0.0f),		// 0 atmosphere...?
		mMaxCellPressure(1.0f),		// 5 atmosphere...?
		mMaxCellVelocitySquared(1000.0f),		// total guess
		mViscosity(0.05f),				// total guess
		mFluidDensity(99.7f),		// water density = 997kg/m^3
		mKinematicViscosity(mViscosity / mFluidDensity),
		mDissipationConstant(1.0f),//.9991f),
		mIsRunning(true),
		mPressureSpreadHalfWidth(2),
		mVelocitySpreadHalfWidth(2),
		mPressureGradientVisible(true),
		mJacobiIterationsPressure(20),
		mJacobiIterationsDiffusion(20),
		mImpulses(std::vector<std::pair<CellCoord, Ogre::Vector3>> { })
	{
		createGrid();

		createCells();

		mActiveCell = mCells[{ (mColumnCount - 1) / 2, 0, (mRowCount - 1) / 2 }];

		mActiveCell->setActive();
	}

	Field::~Field() {
		if (mGridLineMoDef) {
			delete mGridLineMoDef;
			mGridLineMoDef = 0;
		}

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
			"mainGrid",
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
					auto c = new Cell(i, j, k, mColumnCount, mRowCount, mMaxCellPressure, mMaxCellVelocitySquared, mPressureGradientVisible, mGameEntityManager);
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
		return mCells[coords];
	}

	void Field::toggleIsRunning(void) 
	{
		mIsRunning = !mIsRunning;
	}

	void Field::update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex)
	{
		if (mIsRunning) {

			std::unordered_map<CellCoord, CellState> state;

			for (const auto& c : mCells) {
				//if (!c.second->getIsBoundary()) {
					state.insert({ c.second->getCellCoords() , c.second->getState() });
				//}
			}

			advect(timeSinceLast, state);

			addImpulses(timeSinceLast, state);

			assert(state.size() == mCells.size());

			if (mViscosity > 0)
				jacobiDiffusion(timeSinceLast, state);

			divergence(timeSinceLast, state);

			jacobiPressure(timeSinceLast, state);

			boundaryConditions(timeSinceLast, state);

			subtractPressureGradient(timeSinceLast, state);

			for (const auto& c : state) {
				auto cell = mCells[c.first];
				cell->setVelocity(c.second.vVel);
				cell->setPressure(c.second.rPressure);
				cell->setPressureGradient(c.second.vPressureGradient);
			}

			for (const auto& cell : mCells) {
				cell.second->updateTransforms(timeSinceLast, currentTransformIndex, previousTransformIndex);
			}
		}
	}

	void Field::advect(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto cell : state) {

			if (!cell.second.bIsBoundary) {

				auto cVel = cell.second.vVel;

				auto vPos = cell.second.vPos - (cVel * timeSinceLast);

				//if (false) {

				//	auto aCoord = cell.first + CellCoord(-1, 0, 0);
				//	auto bCoord = cell.first + CellCoord(0, 0, 1);
				//	auto cCoord = cell.first + CellCoord(1, 0, 0);
				//	auto dCoord = cell.first + CellCoord(0, 0, -1);

				//	// this type of indexing performs an insert if not found.
				//	auto a = state[aCoord];
				//	auto b = state[bCoord];
				//	auto c = state[cCoord];
				//	auto d = state[dCoord];

				//	Ogre::Vector3 vNewVel = velocityBiLerpCrossForm(

				//		a.vPos,
				//		b.vPos,
				//		c.vPos,
				//		d.vPos,

				//		a.vVel,
				//		b.vVel,
				//		c.vVel,
				//		d.vVel,

				//		vPos.x,
				//		vPos.z
				//	);

				//	if (vNewVel != Ogre::Vector3::ZERO)
				//		int f = 0;

				//	cell.second.vVel = mDissipationConstant * vNewVel;
				//}
				//else {

					int ax, az,
						bx, bz,
						cx, cz,
						dx, dz;

					ax = floor(vPos.x);
					az = ceil(vPos.z);

					bx = floor(vPos.x);
					bz = floor(vPos.z);

					cx = ceil(vPos.x);
					cz = floor(vPos.z);

					dx = ceil(vPos.x);
					dz = ceil(vPos.z);

					if (ax > mColumnCount - 1)
						ax = mColumnCount - 1;
					if (ax < 0)
						ax = 0;
					if (az > mRowCount - 1)
						az = mRowCount - 1;
					if (az < 0)
						az = 0;

					if (bx > mColumnCount - 1)
						bx = mColumnCount - 1;
					if (bx < 0)
						bx = 0;
					if (bz > mRowCount - 1)
						bz = mRowCount - 1;
					if (bz < 0)
						bz = 0;

					if (cx > mColumnCount - 1)
						cx = mColumnCount - 1;
					if (cx < 0)
						cx = 0;
					if (cz > mRowCount - 1)
						cz = mRowCount - 1;
					if (cz < 0)
						cz = 0;

					if (dx > mColumnCount - 1)
						dx = mColumnCount - 1;
					if (dx < 0)
						dx = 0;
					if (dz > mRowCount - 1)
						dz = mRowCount - 1;
					if (dz < 0)
						dz = 0;

					// this type of indexing performs an insert if not found.
					auto a = state[{ ax, 0, abs(az) }];
					auto b = state[{ bx, 0, abs(bz) }];
					auto c = state[{ cx, 0, abs(cz) }];
					auto d = state[{ dx, 0, abs(dz) }];

					Ogre::Vector3 vNewVel = velocityBiLerpCrossForm(

						a.vPos,
						b.vPos,
						c.vPos,
						d.vPos,

						a.vVel,
						b.vVel,
						c.vVel,
						d.vVel,

						vPos.x,
						vPos.z
					);

					if (vNewVel != Ogre::Vector3::ZERO)
						int f = 0;

					cell.second.vVel = mDissipationConstant * vNewVel;
				//}
			}
		}
	}

	void Field::addImpulses(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		auto it = mImpulses.begin();

		while (it != mImpulses.end()) 
		{
			state[it->first].vVel += it->second;
			it = mImpulses.erase(it);
		}
	}

	void Field::jacobiDiffusion(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (!cell.second.bIsBoundary) {
				for (auto i = 0; i < mJacobiIterationsDiffusion; i++) {

					auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
					auto bCoord = cell.first + CellCoord(1, 0, 0); // right
					auto cCoord = cell.first + CellCoord(0, 0, 1); // top
					auto dCoord = cell.first + CellCoord(0, 0, -1); // bottom

					float alpha = mScale * mScale / (mViscosity * timeSinceLast);
					float rBeta = 1 / (4 + alpha);
					auto beta = cell.second.vVel;

					auto a = state[aCoord];
					auto b = state[bCoord];
					auto c = state[cCoord];
					auto d = state[dCoord];

					const_cast<CellState&>(cell.second).vVel = (a.vVel + b.vVel + c.vVel + d.vVel + alpha * beta) * rBeta; 

					if (cell.second.vVel != Ogre::Vector3::ZERO)
						int f = 0;
				}
			}
		}
	}

	void Field::divergence(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (!cell.second.bIsBoundary) {
				auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
				auto bCoord = cell.first + CellCoord(1, 0, 0); // right
				auto cCoord = cell.first + CellCoord(0, 0, 1); // top
				auto dCoord = cell.first + CellCoord(0, 0, -1); // bottom

				auto a = state[aCoord];
				auto b = state[bCoord];
				auto c = state[cCoord];
				auto d = state[dCoord];

				//if (isinf(a.vVel.x))
				//	a.vVel.x = 0;
				//if (isinf(b.vVel.x))
				//	b.vVel.x = 0;
				//if (isinf(c.vVel.z))
				//	c.vVel.z = 0;
				//if (isinf(d.vVel.z))
				//	d.vVel.z = 0;

				const_cast<CellState&>(cell.second).rDivergence = ((a.vVel.x - b.vVel.x) + (c.vVel.z - d.vVel.z)) * mHalfScale;

				if (cell.second.rDivergence != 0)
					int f = 0;
			}
		}
	}

	void Field::jacobiPressure(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (!cell.second.bIsBoundary) {

				if (cell.second.rPressure != 0)
					int f = 0;

				const_cast<CellState&>(cell.second).rPressure = 0;

				for (auto i = 0; i < mJacobiIterationsPressure; i++) {

					auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
					auto bCoord = cell.first + CellCoord(1, 0, 0); // right
					auto cCoord = cell.first + CellCoord(0, 0, 1); // top
					auto dCoord = cell.first + CellCoord(0, 0, -1); // bottom

					float alpha = -mScale * mScale;
					float rBeta = 0.25f;
					auto beta = cell.second.rDivergence;

					if (beta != 0)
						int f = 0;

					auto a = state[aCoord];
					auto b = state[bCoord];
					auto c = state[cCoord];
					auto d = state[dCoord];

					const_cast<CellState&>(cell.second).rPressure = (a.rPressure + b.rPressure + c.rPressure + d.rPressure + alpha * beta) * rBeta;

					if (cell.second.rPressure != 0)
						int f = 0;
				}
			}
		}
	}

	void Field::subtractPressureGradient(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (!cell.second.bIsBoundary) {

				auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
				auto bCoord = cell.first + CellCoord(1, 0, 0); // right
				auto cCoord = cell.first + CellCoord(0, 0, 1); // top
				auto dCoord = cell.first + CellCoord(0, 0, -1); // bottom

				auto a = state[aCoord];
				auto b = state[bCoord];
				auto c = state[cCoord];
				auto d = state[dCoord];

				Ogre::Vector3 gradient = Ogre::Vector3(
					(a.rPressure - b.rPressure),
					0.0f,
					(c.rPressure - d.rPressure)) * mHalfScale;

				if (gradient != Ogre::Vector3::ZERO)
					int f = 0;

				const_cast<CellState&>(cell.second).vVel -= gradient;
				const_cast<CellState&>(cell.second).vPressureGradient = gradient;
			}
		}
	}

	void Field::boundaryConditions(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (cell.second.bIsBoundary) {

				// edges
				CellState neighbourState = CellState();

				if (cell.first.mIndexX == 0) {
					neighbourState = state[cell.first + CellCoord(1, 0, 0)];
				}
				else if (cell.first.mIndexX == mColumnCount - 1) {
					neighbourState = state[cell.first + CellCoord(-1, 0, 0)];
				}
				else if (cell.first.mIndexZ == 0) {
					neighbourState = state[cell.first + CellCoord(0, 0, 1)];
				}
				else if (cell.first.mIndexZ == mRowCount - 1) {
					neighbourState = state[cell.first + CellCoord(0, 0, -1)];
				}

				//corners
				if (cell.first.mIndexX == 0 && cell.first.mIndexZ == 0) {
					neighbourState = state[cell.first + CellCoord(1, 0, 1)];
				}
				else if (cell.first.mIndexX == 0 && cell.first.mIndexZ == mRowCount) {
					neighbourState = state[cell.first + CellCoord(1, 0, -1)];
				}
				else if (cell.first.mIndexX == mColumnCount && cell.first.mIndexZ == 0) {
					neighbourState = state[cell.first + CellCoord(-1, 0, 1)];
				} 
				else if (cell.first.mIndexX == mColumnCount && cell.first.mIndexZ == mRowCount) {
					neighbourState = state[cell.first + CellCoord(-1, 0, -1)];
				}

				const_cast<CellState&>(cell.second).vVel = -1 * neighbourState.vVel;
				const_cast<CellState&>(cell.second).rPressure = neighbourState.rPressure;
			}
		}
	}

	void Field::addImpulse(float timeSinceLast) {
		if (mActiveCell) {
			mImpulses.push_back({ mActiveCell->getCellCoords(), Ogre::Vector3(50.0f, 0.0f, 50.0f) });
		}
	}

	void Field::notifyShiftKey(bool shift) {
		mManualAdjustmentSpeedModifier = shift;
	}

	void Field::rotateVelocityClockwise(float timeSinceLast) {
		if (mIsRunning) {
			float rotationRadians = Ogre::Math::DegreesToRadians(
				50
				* timeSinceLast
				* mBaseManualVelocityAdjustmentSpeed
				* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

			auto q = Ogre::Quaternion(Ogre::Math::Cos(rotationRadians), 0.0f, -Ogre::Math::Sin(rotationRadians), 0.0f);

			q.normalise();

			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {

					mActiveCell->setVelocity(q * mActiveCell->getVelocity());

					for (int i = -mVelocitySpreadHalfWidth; i < mVelocitySpreadHalfWidth + 1; i++) {
						for (int j = -mVelocitySpreadHalfWidth; j < mVelocitySpreadHalfWidth + 1; j++) {
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
	}

	void Field::rotateVelocityCounterClockwise(float timeSinceLast) {
		if (mIsRunning) {
			float rotationRadians = Ogre::Math::DegreesToRadians(
				50
				* timeSinceLast
				* mBaseManualVelocityAdjustmentSpeed
				* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

			auto q = Ogre::Quaternion(Ogre::Math::Cos(rotationRadians), 0.0f, Ogre::Math::Sin(rotationRadians), 0.0f);

			q.normalise();

			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {
					mActiveCell->setVelocity(q * mActiveCell->getVelocity());

					for (int i = -mVelocitySpreadHalfWidth; i < mVelocitySpreadHalfWidth + 1; i++) {
						for (int j = -mVelocitySpreadHalfWidth; j < mVelocitySpreadHalfWidth + 1; j++) {
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
	}

	void Field::increaseVelocity(float timeSinceLast) {
		if (mIsRunning) {
			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {

					auto vImpulse = Ogre::Vector3(2.0f, 0.0f, -2.0f);

					auto vNew = vImpulse * (1 + timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					for (int i = -mVelocitySpreadHalfWidth; i < mVelocitySpreadHalfWidth + 1; i++) {
						for (int j = -mVelocitySpreadHalfWidth; j < mVelocitySpreadHalfWidth + 1; j++) {
							if (i != 0 && j != 0) { // don't impulse self 
								auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
								if (cNeighbour != mCells.end()) {
									auto cNeighbourImpulse = vNew * (1 / (mActiveCell->getState().vPos - cNeighbour->second->getState().vPos).length());
									if (!cNeighbour->second->getIsBoundary()) {
										mImpulses.push_back({ cNeighbour->first, cNeighbourImpulse });
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// This gets called from the parent LogicSystem, when the LeapSystem sends it velocity messages
	void Field::increaseVelocity(float timeSinceLast, Ogre::Vector3 vVel) {
		if (mIsRunning) {
			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {

					auto vImpulse = vVel;

					auto vNew = vImpulse * (1 + timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					for (int i = -mVelocitySpreadHalfWidth; i < mVelocitySpreadHalfWidth + 1; i++) {
						for (int j = -mVelocitySpreadHalfWidth; j < mVelocitySpreadHalfWidth + 1; j++) {
							if (i != 0 && j != 0) { // don't impulse self 
								auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
								if (cNeighbour != mCells.end()) {
									auto cNeighbourImpulse = vNew * (1 / (mActiveCell->getState().vPos - cNeighbour->second->getState().vPos).length());
									if (!cNeighbour->second->getIsBoundary()) {
										mImpulses.push_back({ cNeighbour->first, cNeighbourImpulse });
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void Field::decreaseVelocity(float timeSinceLast) {
		if (mIsRunning) {
			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {

					auto vImpulse = Ogre::Vector3(2.0f, 0.0f, 2.0f);

					auto vNew = vImpulse * (1 - timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					for (int i = -mVelocitySpreadHalfWidth; i < mVelocitySpreadHalfWidth + 1; i++) {
						for (int j = -mVelocitySpreadHalfWidth; j < mVelocitySpreadHalfWidth + 1; j++) {
							if (i != 0 && j != 0) { // don't impulse self 
								auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
								if (cNeighbour != mCells.end()) {
									auto cNeighbourImpulse = vNew * (1 / (mActiveCell->getState().vPos - cNeighbour->second->getState().vPos).length());
									if (!cNeighbour->second->getIsBoundary()) {
										mImpulses.push_back({ cNeighbour->first, cNeighbourImpulse });
									}
								}
							}
						}
					}
				}
			}
			/*else {
				for (auto& c : mCells) {
					if (!c.second->getIsBoundary()) {

						auto vCurr = c.second->getVelocity();

						auto vNew = vCurr * (1 - timeSinceLast
							* mBaseManualVelocityAdjustmentSpeed
							* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

						c.second->setVelocity(vNew);
					}
				}
			}*/
		}
	}

	void Field::increasePressure(float timeSinceLast) {
		if (mActiveCell) {
			if (!mActiveCell->getIsBoundary()) {

				if (mActiveCell->getPressure() < mMaxCellPressure) {

					//if (mActiveCell->getPressure() == 0) {
					//	mActiveCell->setPressure(0.001f);
					//}

					auto rCurrentPressure = mActiveCell->getPressure();

					if (rCurrentPressure == 0.0f)
						rCurrentPressure = 0.01f;

					auto rNewPressure = rCurrentPressure * (1 + timeSinceLast
						* mBaseManualPressureAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualPressureAdjustmentSpeed));

					if (rNewPressure > mMaxCellPressure)
						rNewPressure = mMaxCellPressure;

					for (int i = -mPressureSpreadHalfWidth; i < mPressureSpreadHalfWidth + 1; i++) {
						for (int j = -mPressureSpreadHalfWidth; j < mPressureSpreadHalfWidth + 1; j++) {
							auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
							if (cNeighbour != mCells.end()) {
								if (!cNeighbour->second->getIsBoundary()) {
									cNeighbour->second->setPressure(rNewPressure * (1 / (mActiveCell->getState().vPos - cNeighbour->second->getState().vPos).length()));
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
					* mBaseManualPressureAdjustmentSpeed
					* (1 - mManualAdjustmentSpeedModifier * mBoostedManualPressureAdjustmentSpeed));

				if (rNewPressure < 0)
					rNewPressure = 0;

				for (int i = -mPressureSpreadHalfWidth; i < mPressureSpreadHalfWidth + 1; i++) {
					for (int j = -mPressureSpreadHalfWidth; j < mPressureSpreadHalfWidth + 1; j++) {
						auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
						if (cNeighbour != mCells.end()) {
							if (!cNeighbour->second->getIsBoundary()) {
								cNeighbour->second->setPressure(rNewPressure * (1 / (mActiveCell->getState().vPos - cNeighbour->second->getState().vPos).length()));
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
			mActiveCell = mCells[{ (mColumnCount - 1) / 2, 0, (mRowCount - 1) / 2 }];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexZ += 1;
			if (coords.mIndexZ > mRowCount - 2)
				coords.mIndexZ = 1;
			mActiveCell = mCells[coords];
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellXPositive(void) {
		if (!mActiveCell) {
			mActiveCell = mCells[{ (mColumnCount - 1) / 2, 0, (mRowCount - 1) / 2 }];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexX += 1;
			if (coords.mIndexX > mColumnCount - 2)
				coords.mIndexX = 1;
			mActiveCell = mCells[coords];
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellZPositive(void) {
		if (!mActiveCell) {
			mActiveCell = mCells[{ (mColumnCount - 1) / 2, 0, (mRowCount - 1) / 2 }];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexZ -= 1;
			if (coords.mIndexZ < 1)
				coords.mIndexZ = mRowCount - 2;
			mActiveCell = mCells[coords];
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellXNegative(void) {
		if (!mActiveCell) {
			mActiveCell = mCells[{ (mColumnCount - 1) / 2, 0, (mRowCount - 1) / 2 }];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexX -= 1;
			if (coords.mIndexX < 1)
				coords.mIndexX = mColumnCount - 2;
			mActiveCell = mCells[coords];
			mActiveCell->setActive();
		}
	}

	void Field::resetState(void) {
		for (const auto& c : mCells)
			c.second->resetState();
	}

	void Field::togglePressureGradientIndicators(void) {

		mPressureGradientVisible = !mPressureGradientVisible;

		for (auto& c : mCells) {
			mGameEntityManager->toggleGameEntityVisibility(c.second->getPressureGradientArrowGameEntity(), mPressureGradientVisible);
		}
	}
}