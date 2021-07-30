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
		mGridLineMoDef(0),
		mGridEntity(0),
		mLayerCount(1),
		mCells(std::unordered_map<CellCoord, Cell*> { }),
		mActiveCell(0),
		mBaseManualVelocityAdjustmentSpeed(2.0f),
		mBoostedManualVelocityAdjustmentSpeed(10.0f),
		mBaseManualPressureAdjustmentSpeed(10.0f),
		mBoostedManualPressureAdjustmentSpeed(50.0f),
		mManualAdjustmentSpeedModifier(false),
		mMinCellPressure(0.0f),		// 0 atmosphere...?
		mMaxCellPressure(5.0f),		// 5 atmosphere...?
		mViscosity(1.40f), // total guess
		mFluidDensity(0.9970f),		// water density = 997kg/m^3
		mKinematicViscosity(mViscosity / mFluidDensity),
		mDiffusionConstant(0.991f),
		mIsRunning(true),
		mPressureSpreadHalfWidth(2),
		mVelocitySpreadHalfWidth(2),
		mPressureGradientVisible(true),
		mJacobiIterationsPressure(10),
		mJacobiIterationsDiffusion(20)
	{
		createGrid();

		createCells();
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
					auto c = new Cell(i, j, k, mColumnCount, mRowCount, mMaxCellPressure, mPressureGradientVisible, mGameEntityManager);
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

	void Field::toggleIsRunning(void) 
	{
		mIsRunning = !mIsRunning;
	}

	void Field::update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex)
	{
		if (mIsRunning) {

			std::unordered_map<CellCoord, CellState> state;

			for (auto& c : mCells) {
				//if (!c.second->getIsBoundary()) {
					state.insert({ c.second->getCellCoords() , *c.second->getState() });
				//}
			}

			advect(timeSinceLast, state);

			//jacobiPressure(state);

			jacobiDiffusion(timeSinceLast, state);

			//addForces(timeSinceLast, state);

			//computeVelocityGradient(timeSinceLast, state); 

			computePressureGradient(timeSinceLast, state);

			for (auto& c : state) {
				if (!mCells.at(c.first)->getIsBoundary()) {

					auto cell = mCells.at(c.first);

					auto vNew = c.second.vVel; // nope, velocity is still zero because it began at zero.

					auto pressureGradientTerm = -(1 / mFluidDensity) * c.second.vPressureGradient;

					//vNew += c.second.vPressureGradient;// pressureGradientTerm;
					vNew -= pressureGradientTerm;

					if (vNew != Ogre::Vector3::ZERO)
						cell->setVelocity(vNew);
					if (c.second.rPressure != 0)
						cell->setPressure(c.second.rPressure); // pressure is being advected properly
					cell->setPressureGradient(c.second.vPressureGradient); // the pressure gradient is being calculated properly
				}
			}
		}

		for (auto& cell : mCells) {
			if (cell.second->getIsBoundary()) {

			//	//// equal and opposite reaction: the walls of a container push back against the contents.
			//	//// outer edges:
			//	//if (cell.first.mIndexX == 0) { // x == 0 edge
			//	//	cell.second->setVelocity(-mCells.at(cell.first + CellCoord(1, 0, 0))->getVelocity());
			//	//}
			//	//else if (cell.first.mIndexX == mRowCount - 1) { // opposite x edge
			//	//	cell.second->setVelocity(-mCells.at(cell.first + CellCoord(-1, 0, 0))->getVelocity());
			//	//}
			//	//else if (cell.first.mIndexZ == 0) {
			//	//	cell.second->setVelocity(-mCells.at(cell.first + CellCoord(0, 0, 1))->getVelocity());
			//	//}
			//	//else if (cell.first.mIndexZ == mColumnCount - 1) { // opposite z edge
			//	//	cell.second->setVelocity(-mCells.at(cell.first + CellCoord(0, 0, -1))->getVelocity());
			//	//}

			//	//// corners:  // x == 0, z == 0
			//	//if (cell.first.mIndexX == 0 && cell.first.mIndexZ == 0) {
			//	//	mState.qRot = Ogre::Quaternion(0.3827, 0.0, 0.9239, 0.0);
			//	//}           // x == max, z == 0
			//	//else if (cell.first.mIndexX == mRowCount - 1 && cell.first.mIndexZ == 0) {
			//	//	mState.qRot = Ogre::Quaternion(-0.3827, 0.0, 0.9239, 0.0);
			//	//}           // x == 0, z == max
			//	//else if (cell.first.mIndexX == 0 && cell.first.mIndexZ == mColumnCount - 1) {
			//	//	mState.qRot = Ogre::Quaternion(-0.9239, 0.0, -0.3827, 0.0);
			//	//}           // x == max, z == max
			//	//else if (cell.first.mIndexX == mRowCount - 1 && cell.first.mIndexZ == mColumnCount - 1) {
			//	//	mState.qRot = Ogre::Quaternion(0.9239, 0.0, -0.3827, 0.0);
			//	//}
			}
			else
				cell.second->updateTransforms(timeSinceLast, currentTransformIndex, previousTransformIndex);
		}
	}
	
	void Field::jacobiPressure(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto& cell : state) {
			if (!cell.second.bIsBoundary) {
				for (auto i = 0; i < mJacobiIterationsPressure; i++) {

					auto aCoord = cell.first + CellCoord(-1, 0, 0);
					auto bCoord = cell.first + CellCoord(0, 0, 1);
					auto cCoord = cell.first + CellCoord(1, 0, 0);
					auto dCoord = cell.first + CellCoord(0, 0, -1);

					float alpha = -1;
					auto beta = cell.second.vVelocityGradient;

					auto a = state.at(aCoord);
					auto b = state.at(bCoord);
					auto c = state.at(cCoord);
					auto d = state.at(dCoord);

					cell.second.vPressureGradient = (a.rPressure + b.rPressure + c.rPressure + d.rPressure + (alpha * beta)) / 4;
				}
			}
		}
	}

	void Field::jacobiDiffusion(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto& cell : state) {
			if (!cell.second.bIsBoundary) {
				for (auto i = 0; i < mJacobiIterationsDiffusion; i++) {

					auto aCoord = cell.first + CellCoord(-1, 0, 0);
					auto bCoord = cell.first + CellCoord(0, 0, 1);
					auto cCoord = cell.first + CellCoord(1, 0, 0);
					auto dCoord = cell.first + CellCoord(0, 0, -1);

					float alpha = (1 / mViscosity * timeSinceLast);
					float rBeta = (1 / (4 + 1 / mViscosity * timeSinceLast));
					auto beta = cell.second.vVelocityGradient;

					auto a = state.at(aCoord);
					auto b = state.at(bCoord);
					auto c = state.at(cCoord);
					auto d = state.at(dCoord);

					cell.second.vVel = (a.vVel + b.vVel + c.vVel + d.vVel + alpha * cell.second.vVel) * rBeta;
				}
			}
		}
	}

	void Field::advect(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto& cell : state) {

			if (!cell.second.bIsBoundary) {

				auto cVel = cell.second.vVel;

				auto vPos = cell.second.vPos - (cVel * timeSinceLast);

				if (true) { //cVel == Ogre::Vector3::ZERO) {

					auto aCoord = cell.first + CellCoord(-1, 0, 0);
					auto bCoord = cell.first + CellCoord(0, 0, 1);
					auto cCoord = cell.first + CellCoord(1, 0, 0);
					auto dCoord = cell.first + CellCoord(0, 0, -1);

					auto a = state.at(aCoord);
					auto b = state.at(bCoord);
					auto c = state.at(cCoord);
					auto d = state.at(dCoord);

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

					Ogre::Real rNewPressure = pressureBiLerpCrossForm(
						a.vPos,
						b.vPos,
						c.vPos,
						d.vPos,
						a.rPressure,
						b.rPressure,
						c.rPressure,
						d.rPressure,
						vPos.x,
						vPos.z
					);

					cell.second.rPressure = rNewPressure;
					cell.second.vVel = vNewVel;
					//cell.second.vViscosity = (a.vVel + b.vVel + c.vVel + d.vVel) - (4 * cell.second.vVel);
				}

				else {

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

					auto a = state.at({ ax, 0, abs(az) });
					auto b = state.at({ bx, 0, abs(bz) });
					auto c = state.at({ cx, 0, abs(cz) });
					auto d = state.at({ dx, 0, abs(dz) });

					Ogre::Vector3 vNewVel = velocityBiLerp(

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

					Ogre::Real rNewPressure = pressureBiLerp(

						a.vPos,
						b.vPos,
						c.vPos,
						d.vPos,

						a.rPressure,
						b.rPressure,
						c.rPressure,
						d.rPressure,

						vPos.x,
						vPos.z
					);

					cell.second.rPressure = rNewPressure;
					cell.second.vVel = vNewVel;
					//cell.second.vViscosity = (a.vVel + b.vVel + c.vVel + d.vVel) - (4 * cell.second.vVel);
				}

				

				//auto a = state.find({ ax, 0, abs(az) });
				//auto b = state.find({ bx, 0, abs(bz) });
				//auto c = state.find({ cx, 0, abs(cz) });
				//auto d = state.find({ dx, 0, abs(dz) });

				//if (a != state.end() &&
				//	b != state.end() &&
				//	c != state.end() &&
				//	d != state.end())
				//{
				//	assert(a != b);
				//	assert(a != c);
				//	assert(a != d);

				//	assert(b != c);
				//	assert(b != d);

				//	assert(c != d);

				//	Ogre::Vector3 vNewVel = velocityBiLerp(

				//		a->second.vPos,
				//		b->second.vPos,
				//		c->second.vPos,
				//		d->second.vPos,

				//		a->second.vVel,
				//		b->second.vVel,
				//		c->second.vVel,
				//		d->second.vVel,

				//		vPos.x,
				//		vPos.z
				//	);

				//	cell.second.vVel = vNewVel;
				//}
			}
		}
	}

	void Field::diffuse(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
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

	void Field::addForces(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{

	}

	void Field::computeVelocityGradient(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto& c : state) {
			if (!c.second.bIsBoundary) {

				auto left = c.first + CellCoord(-1, 0, 0);
				auto right = c.first + CellCoord(1, 0, 0);
				auto top = c.first + CellCoord(0, 0, 1);
				auto bottom = c.first + CellCoord(0, 0, -1);

				auto l = state.at(left);
				auto r = state.at(right);
				auto t = state.at(top);
				auto b = state.at(bottom);

				Ogre::Vector3 gradient = Ogre::Vector3(
					((l.vVel - r.vVel) / 2).length(),
					0.0f,
					((t.vVel - b.vVel) / 2).length());

				c.second.vVelocityGradient = gradient;
			}
		}
	}

	void Field::computePressureGradient(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto& c : state) {
			if (!c.second.bIsBoundary) {

				auto left = c.first + CellCoord(-1, 0, 0);
				auto right = c.first + CellCoord(1, 0, 0);
				auto top = c.first + CellCoord(0, 0, 1);
				auto bottom = c.first + CellCoord(0, 0, -1);

				auto l = state.at(left);
				auto r = state.at(right);
				auto t = state.at(top);
				auto b = state.at(bottom);

				Ogre::Vector3 gradient = Ogre::Vector3(
					(l.rPressure - r.rPressure) / 2, 
					0.0f, 
					(t.rPressure - b.rPressure) / 2);

				c.second.vPressureGradient = gradient;
			}
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

					if (mActiveCell->getVelocity() == Ogre::Vector3::ZERO) {
						mActiveCell->setVelocity(Ogre::Vector3(0, 0, -0.1f));
					}

					auto vCurr = mActiveCell->getVelocity();

					auto vNew = vCurr * (1 + timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

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

						auto vNew = vCurr * (1 + timeSinceLast
							* mBaseManualVelocityAdjustmentSpeed
							* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

						c.second->setVelocity(vNew);
					}
				}
			}
		}
	}

	void Field::decreaseVelocity(float timeSinceLast) {
		if (mIsRunning) {
			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {

					if (mActiveCell->getVelocity() == Ogre::Vector3::ZERO) {
						mActiveCell->setVelocity(Ogre::Vector3(0, 0, 0.1f));
					}

					auto vCurr = mActiveCell->getVelocity();

					auto vNew = vCurr * (1 - timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

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
							* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

						c.second->setVelocity(vNew);
					}
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
						* mBaseManualPressureAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualPressureAdjustmentSpeed));

					if (rNewPressure > mMaxCellPressure)
						rNewPressure = mMaxCellPressure;

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
					* mBaseManualPressureAdjustmentSpeed
					* (1 - mManualAdjustmentSpeedModifier * mBoostedManualPressureAdjustmentSpeed));

				if (rNewPressure < 0)
					rNewPressure = 0;

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

	void Field::togglePressureGradientIndicators(void) {

		mPressureGradientVisible = !mPressureGradientVisible;

		for (auto& c : mCells) {
			mGameEntityManager->toggleGameEntityVisibility(c.second->getPressureGradientArrowGameEntity(), mPressureGradientVisible);
		}
	}
}