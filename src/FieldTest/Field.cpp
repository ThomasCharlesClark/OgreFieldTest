#include <stdexcept>
#include <sstream>
#include <vector>

#include "Cell.h"
#include "Field.h"

#include "MyMath.h"

#include "OgreVector3.h"

#include <map>

using namespace MyThirdOgre;

namespace MyThirdOgre
{
	Field::Field(
		GameEntityManager* geMgr, 
		float scale, 
		int columnCount, 
		int rowCount, 
		float maxInk) :
		mGameEntityManager(geMgr),
		mScale(scale),
		mHalfScale(0.5 * scale),
		mDeltaX(1.0f),
		mHalfDeltaX(0.5f),
		/*mColumnCount(columnCount),
		mRowCount(rowCount),*/
		mColumnCount(23),
		mRowCount(23),
		mGridLineMoDef(0),
		mGridEntity(0),
		mLayerCount(1),
		mCells(std::unordered_map<CellCoord, Cell*> { }),
		mActiveCell(0),
		mBaseManualVelocityAdjustmentSpeed(20.0f),
		mBoostedManualVelocityAdjustmentSpeed(80.0f),
		mBaseManualPressureAdjustmentSpeed(0.01f),
		mBoostedManualPressureAdjustmentSpeed(0.05f),
		mManualAdjustmentSpeedModifier(false),
		mMinCellPressure(0.0f),		// 0 atmosphere...?
		mMaxCellPressure(1.0f),		// this is BS
		mMaxCellVelocitySquared(1000.0f),	// total guess
		mViscosity(0.0f),					// total guess
		mFluidDensity(0.0f),				// water density = 997kg/m^3
		mKinematicViscosity(mViscosity / mFluidDensity),
		mVelocityDissipationConstant(1.0f),
		mInkDissipationConstant(0.991f),
		mIsRunning(false),
		mPressureSpreadHalfWidth(4),
		mVelocitySpreadHalfWidth(2),
#if OGRE_DEBUG_MODE
		mGridVisible(false),
		mVelocityVisible(true),
		mPressureGradientVisible(true),
		mJacobiIterationsPressure(20),
		mJacobiIterationsDiffusion(20),
#else
		mGridVisible(false),
		mVelocityVisible(false),
		mPressureGradientVisible(false),
		mJacobiIterationsPressure(80),
		mJacobiIterationsDiffusion(40),
#endif
		mImpulses(std::vector<std::pair<CellCoord, HandInfluence>> { }),
		mHand(0),
		mVorticityConfinementScale(0.065f),
		mMaxInk(maxInk),
		mUseComputeSystem( true ),
		mFieldComputeSystem( 0 )
	{
		mReciprocalDeltaX = (float)1 / mDeltaX;
		mHalfReciprocalDeltaX = 0.5f / mReciprocalDeltaX;

		if (mGridVisible)
			createGrid();

		//if (!mUseComputeSystem)
			createCells();

		//mActiveCell = mCells[{0, 0, mRowCount / 2 - 1}];

		//mActiveCell = mCells[{0, 0, 0}];

		if (mActiveCell)
			mActiveCell->setActive();

		if (mUseComputeSystem)
			createFieldComputeSystem();
	}

	Field::~Field() {
		if (mGridLineMoDef) {
			delete mGridLineMoDef;
			mGridLineMoDef = 0;
		}

		if (mUseComputeSystem) {
			delete mFieldComputeSystemMoDef;
			mFieldComputeSystem = 0;

			delete mFieldComputeSystem;
			mFieldComputeSystem = 0;
		}

		for (auto i = mCells.begin(); i != mCells.end(); i++) {
			delete i->second;
		}
	}

	void Field::_notifyHand(Hand* hand) 
	{
		mHand = hand;

		if (mFieldComputeSystem) {
			mFieldComputeSystem->_notifyHand(mHand);
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

		float adjuster = (mColumnCount / 2) + 0.5f;

		for (int i = 0; i < mColumnCount + 1; i++) {
			gridLineList.points.push_back(Ogre::Vector3((i - adjuster), 0, adjuster) * mScale);
			gridLineList.points.push_back(Ogre::Vector3((i - adjuster), 0, -(mColumnCount - adjuster)) * mScale);
			gridLineList.lines.push_back({ pointCounter, pointCounter + 1 });
			pointCounter += 2;
		}

		for (int i = 0; i < mRowCount + 1; i++) {
			gridLineList.points.push_back(Ogre::Vector3(-adjuster, 0, -(i - adjuster)) * mScale);
			gridLineList.points.push_back(Ogre::Vector3((mRowCount - adjuster), 0, -(i - adjuster)) * mScale);
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
					auto c = new Cell(
						mScale,
						i, 
						j, 
						k,
						mColumnCount,
						mRowCount, 
						mMaxCellPressure, 
						mMaxCellVelocitySquared, 
						mVelocityVisible,
						mPressureGradientVisible,
						mMaxInk,
						mGameEntityManager);
					mCells.insert({
						c->getCellCoords(),
						c
					});
				}
			}
		}
	}

	void Field::createFieldComputeSystem(void) 
	{
		mFieldComputeSystemMoDef = new MovableObjectDefinition();

		mFieldComputeSystemMoDef->moType = MoTypeFieldComputeSystem;
		mFieldComputeSystemMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;

		mFieldComputeSystem = mGameEntityManager->addFieldComputeSystem(
			"mainGridComputeSystem",
			Ogre::SceneMemoryMgrTypes::SCENE_STATIC,
			mFieldComputeSystemMoDef,
			"TestCompute",
			Ogre::Vector3(0, -1, 0),
			Ogre::Quaternion::IDENTITY,
			Ogre::Vector3::UNIT_SCALE, // What does it mean to even scale an abstract concept... 
									  // but it ISN'T an abstract concept: this is a concrete THING which exists at a POSITION and DOES STUFF.
									  // might be sensible to try to avoid the notion of scaling it just yet, though.
			this
		);
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
		if (mFieldComputeSystem && !mFieldComputeSystem->getParent()) {
			mFieldComputeSystem->_notifyField(this);
		}

		if (mIsRunning) 
		{

			// Noooooooooooooooo don't do that here
			// Let the Graphics thread do all the updating of the field compute system
			// otherwise BAD THINGS
			/*if (mFieldComputeSystem) {
				mFieldComputeSystem->update(timeSinceLast);
			}*/
			
			std::unordered_map<CellCoord, CellState> state;

			for (const auto& c : mCells) {
				state.insert({ c.second->getCellCoords() , c.second->getState() });

				if (mHand) {
					if (!c.second->getIsBoundary()) {
						if (c.second->getBoundingSphere()->intersects(*mHand->getBoundingSphere())) {
							mImpulses.push_back({ c.first, HandInfluence(mHand->getState().vVel, mHand->getState().rInk) });
						}
					}
				}
			}

			addImpulses(timeSinceLast, state);

			advect(timeSinceLast, state);

			if (mViscosity > 0)
				jacobiDiffusion(timeSinceLast, state);

			divergence(timeSinceLast, state);

			jacobiPressure(timeSinceLast, state);

			boundaryConditions(timeSinceLast, state);

			subtractPressureGradient(timeSinceLast, state);

			if (mVorticityConfinementScale) {
				vorticityComputation(timeSinceLast, state);
				vorticityConfinement(timeSinceLast, state);
			}

			assert(state.size() == mCells.size());

			for (const auto& c : state) {
				auto cell = mCells[c.first];

				cell->setState(c.second);
			}

			for (const auto& cell : mCells) {
				cell.second->updateTransforms(timeSinceLast, currentTransformIndex, previousTransformIndex);
			}
		}
		else {
			// we can now be "not running" as in "not running on the CPU", but still trying to transmit info from GPU to CPU cells.
			for (const auto& cell : mCells) {
				if (cell.second) {
					cell.second->updateTransforms(timeSinceLast, currentTransformIndex, previousTransformIndex);
				}
				else {
					int f = 0;
				}
			}
		}
	}

	void Field::advect(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {

			if (!cell.second.bIsBoundary) {

				auto cVel = cell.second.vVel;

				auto vPos = cell.second.vPos -(cVel * mReciprocalDeltaX * timeSinceLast);

				CellState a, b, c, d;

				CellCoord cellCoord = cell.first;

				int ax, az,
					bx, bz,
					cx, cz,
					dx, dz;

				if (vPos.x == static_cast<int>(vPos.x) && vPos.z == static_cast<int>(vPos.z))
				{
					// we're bang on a corner: take a linear interpolation approach
					auto aCoord = cellCoord + CellCoord(-1, 0, 0); // left
					auto bCoord = cellCoord + CellCoord(0, 0, -1); // top 
					auto cCoord = cellCoord + CellCoord(1, 0, 0);  // right
					auto dCoord = cellCoord + CellCoord(0, 0, 1);  // bottom

					a = state[aCoord];
					b = state[bCoord];
					c = state[cCoord];
					d = state[dCoord];

					auto vAC = Ogre::Math::lerp(a.vVel, c.vVel, 0.5f);
					auto vBD = Ogre::Math::lerp(b.vVel, d.vVel, 0.5f);

					Ogre::Vector3 vNewVel = Ogre::Math::lerp(vAC, vBD, 0.5f);

					const_cast<CellState&>(cell.second).vVel = mVelocityDissipationConstant * vNewVel;

					auto iAC = Ogre::Math::lerp(a.rInk, c.rInk, 0.5f);
					auto iBD = Ogre::Math::lerp(b.rInk, d.rInk, 0.5f);

					auto rNewInk = Ogre::Math::lerp(iAC, iBD, 0.5f);

					const_cast<CellState&>(cell.second).rInk = mInkDissipationConstant * rNewInk;
				}
				else 
				{
					ax = floor(vPos.x); // floor moves left along the number line
					az = ceil(vPos.z);	// ceil  moves right along the number line

					bx = floor(vPos.x);
					bz = floor(vPos.z);

					cx = ceil(vPos.x);
					cz = floor(vPos.z);

					dx = ceil(vPos.x);
					dz = ceil(vPos.z);

					ax = std::clamp(ax, -mColumnCount / 2, mColumnCount / 2);
					az = std::clamp(az, -mRowCount / 2, mRowCount / 2);

					bx = std::clamp(bx, -mColumnCount / 2, mColumnCount / 2);
					bz = std::clamp(bz, -mRowCount / 2, mRowCount / 2);

					cx = std::clamp(cx, -mColumnCount / 2, mColumnCount / 2);
					cz = std::clamp(cz, -mRowCount / 2, mRowCount / 2);

					dx = std::clamp(dx, -mColumnCount / 2, mColumnCount / 2);
					dz = std::clamp(dz, -mRowCount / 2, mRowCount / 2);

					a = state[{ax, 0, az}];
					b = state[{bx, 0, bz}];
					c = state[{cx, 0, cz}];
					d = state[{dx, 0, dz}];

					// this... should! have the four corners around our "back-in-time" point.

					Ogre::Vector3 vNewVel = vectorBiLerp(
						a.vPos,
						b.vPos,
						c.vPos,
						d.vPos,
						a.vVel,
						b.vVel,
						c.vVel,
						d.vVel,
						vPos.x,
						vPos.z,
						false
					);

					const_cast<CellState&>(cell.second).vVel = mVelocityDissipationConstant * vNewVel;

					Ogre::Real rNewInk = scalarBiLerp(
						a.vPos,
						b.vPos,
						c.vPos,
						d.vPos,
						a.rInk,
						b.rInk,
						c.rInk,
						d.rInk,
						vPos.x,
						vPos.z,
						false
					);

					const_cast<CellState&>(cell.second).rInk = mInkDissipationConstant * rNewInk;
				}
			}
		}
	}

	void Field::addImpulses(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		auto it = mImpulses.begin();

		while (it != mImpulses.end()) 
		{
			state[it->first].vVel += it->second.vVelocity;
			state[it->first].rInk += it->second.rInkAmount;
			
			it = mImpulses.erase(it);
		}
	}

	void Field::vorticityComputation(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (!cell.second.bIsBoundary) {
				auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
				auto bCoord = cell.first + CellCoord(1, 0, 0); // right
				auto cCoord = cell.first + CellCoord(0, 0, -1); // top
				auto dCoord = cell.first + CellCoord(0, 0, 1); // bottom

				auto a = state[aCoord];
				auto b = state[bCoord];
				auto c = state[cCoord];
				auto d = state[dCoord];

				const_cast<CellState&>(cell.second).rVorticity = ((a.vVel.z - b.vVel.z) - (c.vVel.x - d.vVel.x)) * mHalfDeltaX;
			}
		}
	}

	void Field::vorticityConfinement(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (!cell.second.bIsBoundary) {
				auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
				auto bCoord = cell.first + CellCoord(1, 0, 0); // right
				auto cCoord = cell.first + CellCoord(0, 0, -1); // top
				auto dCoord = cell.first + CellCoord(0, 0, 1); // bottom

				auto a = state[aCoord];
				auto b = state[bCoord];
				auto c = state[cCoord];
				auto d = state[dCoord];

				Ogre::Vector3 force = Ogre::Vector3
					(abs(c.rVorticity) - abs(d.rVorticity),
					0.0f,
					abs(b.rVorticity) - abs(a.rVorticity)) 
					* mHalfDeltaX;

				force.normalise();

				force *= mVorticityConfinementScale * cell.second.rVorticity * Ogre::Vector3(1, 0, -1);

				const_cast<CellState&>(cell.second).vVel += force * timeSinceLast;
			}
		}
	}

	void Field::jacobiDiffusion(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto i = 0; i < mJacobiIterationsDiffusion; i++) {
			for (const auto& cell : state) {
				if (!cell.second.bIsBoundary) {
					auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
					auto bCoord = cell.first + CellCoord(1, 0, 0); // right
					auto cCoord = cell.first + CellCoord(0, 0, -1); // top
					auto dCoord = cell.first + CellCoord(0, 0, 1); // bottom

					float alpha = mHalfDeltaX * mHalfDeltaX / (mViscosity * timeSinceLast);
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
				auto cCoord = cell.first + CellCoord(0, 0, -1); // top
				auto dCoord = cell.first + CellCoord(0, 0, 1); // bottom

				auto a = state[aCoord];
				auto b = state[bCoord];
				auto c = state[cCoord];
				auto d = state[dCoord];

				const_cast<CellState&>(cell.second).rDivergence = ((a.vVel.x - b.vVel.x) + (c.vVel.z - d.vVel.z)) * mHalfDeltaX;

				if (cell.second.rDivergence != 0)
					int f = 0;
			}
		}
	}

	void Field::jacobiPressure(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (auto i = 0; i < mJacobiIterationsPressure; i++) {

			for (const auto& cell : state) {
				if (!cell.second.bIsBoundary) {

					if (cell.second.rPressure != 0)
						int f = 0;

					const_cast<CellState&>(cell.second).rPressure = 0;

					auto aCoord = cell.first + CellCoord(-1, 0, 0); // left
					auto bCoord = cell.first + CellCoord(1, 0, 0); // right
					auto cCoord = cell.first + CellCoord(0, 0, -1); // top
					auto dCoord = cell.first + CellCoord(0, 0, 1); // bottom

					float alpha = -mHalfDeltaX * mHalfDeltaX;
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
				auto cCoord = cell.first + CellCoord(0, 0, -1); // top
				auto dCoord = cell.first + CellCoord(0, 0, 1); // bottom

				auto a = state[aCoord];
				auto b = state[bCoord];
				auto c = state[cCoord];
				auto d = state[dCoord];

				Ogre::Vector3 gradient = Ogre::Vector3(
					(a.rPressure - b.rPressure),
					0.0f,
					(c.rPressure - d.rPressure)) * mHalfDeltaX;

				if (gradient != Ogre::Vector3::ZERO)
					int f = 0;
				
				if (mFluidDensity) {
					const_cast<CellState&>(cell.second).vVel -= (1 / mFluidDensity) * gradient;
				}
				else {
					const_cast<CellState&>(cell.second).vVel -= gradient;
				}
				const_cast<CellState&>(cell.second).vPressureGradient = gradient;
			}
		}
	}

	void Field::boundaryConditions(float timeSinceLast, std::unordered_map<CellCoord, CellState>& state)
	{
		for (const auto& cell : state) {
			if (cell.second.bIsBoundary) {

				// edges
				CellCoord adjustment = CellCoord();

				if (cell.first.mIndexX == -(mColumnCount / 2)) {
					adjustment = CellCoord(1, 0, 0);
				}
				else if (cell.first.mIndexX == mColumnCount / 2) {
					adjustment = CellCoord(-1, 0, 0);
				}
				else if (cell.first.mIndexZ == -(mRowCount / 2)) {
					adjustment = CellCoord(0, 0, 1);
				}
				else if (cell.first.mIndexZ == mRowCount / 2) {
					adjustment = CellCoord(0, 0, -1);
				}

				//corners
				if (cell.first.mIndexX == -(mColumnCount / 2) && cell.first.mIndexZ == -(mRowCount / 2)) {
					adjustment = CellCoord(1, 0, 1);
				}
				else if (cell.first.mIndexX == -(mColumnCount / 2) && cell.first.mIndexZ == mRowCount / 2) {
					adjustment = CellCoord(1, 0, -1);
				}
				else if (cell.first.mIndexX == mColumnCount / 2 && cell.first.mIndexZ == -(mRowCount / 2)) {
					adjustment = CellCoord(-1, 0, 1);
				} 
				else if (cell.first.mIndexX == mColumnCount / 2 && cell.first.mIndexZ == mRowCount / 2) {
					adjustment = CellCoord(-1, 0, -1);
				}

				CellState neighbourState = state[cell.first + adjustment];

				const_cast<CellState&>(cell.second).rInk = neighbourState.rInk;
				const_cast<CellState&>(cell.second).vVel = -1 * neighbourState.vVel;
				const_cast<CellState&>(cell.second).rPressure = neighbourState.rPressure;
			}
		}
	}

	void Field::addImpulse(float timeSinceLast) {
		if (mActiveCell) {
			mImpulses.push_back({ mActiveCell->getCellCoords(), HandInfluence(Ogre::Vector3(0.0f, 0.0f, -15.0f), 10.0f) });
		}
	}

	void Field::notifyShiftKey(bool shift) {
		mManualAdjustmentSpeedModifier = shift;
	}

	void Field::rotateVelocityClockwise(float timeSinceLast) {
		if (mIsRunning) {
			float rotationRadians = Ogre::Math::DegreesToRadians(
				15
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
				15
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

	// This gets called from the parent LogicSystem as a result of SDL input messages
	void Field::increaseVelocity(float timeSinceLast) {
		if (mIsRunning) {
			if (mActiveCell) {
				if (!mActiveCell->getIsBoundary()) {

					auto vImpulse = Ogre::Vector3(0.0f, 0.0f, -5.0f);

					auto vNew = vImpulse * (1 + timeSinceLast
						* mBaseManualVelocityAdjustmentSpeed
						* (1 + mManualAdjustmentSpeedModifier * mBoostedManualVelocityAdjustmentSpeed));

					//mImpulses.push_back({ mActiveCell->getCellCoords(), HandInfluence(vNew, 1.0f) });

					for (int i = -mVelocitySpreadHalfWidth; i < mVelocitySpreadHalfWidth + 1; i++) {
						for (int j = -mVelocitySpreadHalfWidth; j < mVelocitySpreadHalfWidth + 1; j++) {
							//if (i != 0 && j != 0) { // don't impulse self... but... why not?
								auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
								if (cNeighbour != mCells.end()) {
									if (i == 0 && j == 0) {
										mImpulses.push_back({ cNeighbour->first, HandInfluence(vNew, mMaxInk) });
									}
									else {
										auto qRot = (mActiveCell->getState().vPos).getRotationTo(cNeighbour->second->getState().vPos);
										auto cNeighbourImpulse = qRot * vNew;
										//auto cNeighbourImpulse =  -(mActiveCell->getState().vPos - cNeighbour->second->getState().vPos);
										if (!cNeighbour->second->getIsBoundary()) {
											mImpulses.push_back({ cNeighbour->first, HandInfluence(cNeighbourImpulse,  mMaxInk - cNeighbourImpulse.length()) }); // fun to be had with coefficients of ink
											//mImpulses.push_back({ cNeighbour->first, HandInfluence(cNeighbourImpulse, 1 / cNeighbourImpulse.squaredLength()) });
										}
									}
								}
							//}
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
							auto cNeighbour = mCells.find(mActiveCell->getCellCoords() + CellCoord(i, 0, j));
							if (cNeighbour != mCells.end()) {
								if (i == 0 && j == 0) {
									mImpulses.push_back({ cNeighbour->first, HandInfluence(vNew, mMaxInk * 2) });
								}
								else {
									//auto cNeighbourImpulse = vNew * (1 / (mActiveCell->getState().vPos - cNeighbour->second->getState().vPos).length());
									auto cNeighbourImpulse = mActiveCell->getState().vPos - cNeighbour->second->getState().vPos;
									if (!cNeighbour->second->getIsBoundary()) {
										mImpulses.push_back({ cNeighbour->first, HandInfluence(cNeighbourImpulse, mMaxInk - cNeighbourImpulse.length()) });
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
										mImpulses.push_back({ cNeighbour->first, HandInfluence(cNeighbourImpulse, true) });
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
			mActiveCell = mCells[{0, 0, 0}];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexZ -= 1;
			if (coords.mIndexZ == -(mRowCount / 2) - 1)
				coords.mIndexZ = (mRowCount / 2);
			mActiveCell = mCells[coords];
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellZPositive(void) {
		if (!mActiveCell) {
			mActiveCell = mCells[{0, 0, 0}];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexZ += 1;
			if (coords.mIndexZ == (mRowCount / 2) + 1)
				coords.mIndexZ = -(mRowCount / 2);
			mActiveCell = mCells[coords];
			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellXPositive(void) {
		if (!mActiveCell) {
			mActiveCell = mCells[{0, 0, 0}];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexX += 1;
			if (coords.mIndexX == (mColumnCount / 2) + 1)
				coords.mIndexX = -(mColumnCount / 2);
			mActiveCell = mCells[coords];
 			mActiveCell->setActive();
		}
	}

	void Field::traverseActiveCellXNegative(void) {
		if (!mActiveCell) {
			mActiveCell = mCells[{0, 0, 0}];
			mActiveCell->setActive();
		}
		else {
			mActiveCell->unsetActive();
			auto coords = mActiveCell->getCellCoords();
			coords.mIndexX -= 1;
			if (coords.mIndexX == -(mColumnCount / 2) - 1)
				coords.mIndexX = (mColumnCount / 2);
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

	void Field::toggleVelocityIndicators(void) {
		mVelocityVisible = !mVelocityVisible;
		for (auto& c : mCells) {
			mGameEntityManager->toggleGameEntityVisibility(c.second->getVelocityArrowGameEntity(), mVelocityVisible);
		}
	}
}