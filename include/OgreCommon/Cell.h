#pragma once

#include "OgreSceneManager.h"
#include "OgreManualObject.h"
#include "OgreSceneNode.h"
#include "OgreVector2.h"
#include "OgrePrerequisites.h"
#include "OgreQuaternion.h"
#include "OgreItem.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreEntity.h"
#include "OgreSphere.h"

#include "GameEntity.h"
#include "GameEntityManager.h"

// Velocity IS and SHOULD BE separte from ORIENTATION.
// Velocity could be ridiculously high, it can bounce around, be reflected, etc. 
// So what... relationship do those statements bear to what's going on in my simulation?

// At the moment, velocity seems to be tightly coupled to orientation. The idea that it is some separate quantity has been lost.

// Right. That has disconnected velocity from orientation once more.
// But... now... here's the question. Is Velocity really separate from Orientation?

// Ye... no... velocity is a distance moved over a time elapsed.

// Therefore each cell has a velocity X component and a velocity Y component. 

// Our "orientation" is simply an attmept to visualise the "facing" for velocity.
// Therefore orientation MUST be directly and irrefutably connected to velocity: we want to "Point" in the direction of our velocity.

// We do NOT want to "velocity" in the direction of our orientation. Quite the opposite.

// Therefore all operations should occur on our velocity, and our orientation should be updated to match.

namespace MyThirdOgre {
	struct CellCoord
	{
		int mIndexX;
		int mIndexY;
		int mIndexZ;

		CellCoord() {};

		CellCoord(
			int x,
			int y,
			int z) {
			mIndexX = x;
			mIndexY = y;
			mIndexZ = z;
		};
	};

	// tyvm; https://stackoverflow.com/questions/4421706/what-are-the-basic-rules-and-idioms-for-operator-overloading#answer-4421719
	// ty even more: https://stackoverflow.com/questions/3882467/defining-operator-for-a-struct#answer-16090720
	inline bool operator==(const CellCoord& lhs, const CellCoord& rhs) {
		return	lhs.mIndexX == rhs.mIndexX &&
			lhs.mIndexY == rhs.mIndexY &&
			lhs.mIndexZ == rhs.mIndexZ;
	}
	inline bool operator!=(const CellCoord& lhs, const CellCoord& rhs) { return !operator==(lhs, rhs); }
	inline bool operator< (const CellCoord& lhs, const CellCoord& rhs) {
		return std::tie(lhs.mIndexX, lhs.mIndexY, lhs.mIndexZ) < std::tie(rhs.mIndexX, rhs.mIndexY, rhs.mIndexZ);
	}
	inline bool operator> (const CellCoord& lhs, const CellCoord& rhs) { return  operator< (rhs, lhs); }
	inline bool operator<=(const CellCoord& lhs, const CellCoord& rhs) { return !operator> (lhs, rhs); }
	inline bool operator>=(const CellCoord& lhs, const CellCoord& rhs) { return !operator< (lhs, rhs); }

	inline CellCoord operator-(const CellCoord& lhs, const CellCoord& rhs) {
		return CellCoord(
			lhs.mIndexX - rhs.mIndexX,
			lhs.mIndexY - rhs.mIndexY,
			lhs.mIndexZ - rhs.mIndexZ);
	}

	inline CellCoord operator+(const CellCoord& lhs, const CellCoord& rhs) {
		return CellCoord(
			lhs.mIndexX + rhs.mIndexX,
			lhs.mIndexY + rhs.mIndexY,
			lhs.mIndexZ + rhs.mIndexZ);
	}
}

namespace std {
	template <> struct hash<MyThirdOgre::CellCoord>
	{
		size_t operator()(const MyThirdOgre::CellCoord& x) const
		{
			return ((hash<int>()(x.mIndexX) << 32) + ((hash<int>()(x.mIndexY) << 32)) + (hash<int>()(x.mIndexZ)));
		}
	};
}

namespace MyThirdOgre 
{
	class GameEntityManager;

	struct CellState
	{
		bool bIsBoundary;
		Ogre::Vector3 vPos;
		Ogre::Vector3 vVel;
		Ogre::Vector3 vPressureGradient;
		Ogre::Real rDivergence;
		Ogre::Quaternion qRot;
		Ogre::Real rPressure;
		bool bActive;
		Ogre::Real rVorticity;
		Ogre::Real rInk;
		Ogre::Real rMaxInk;
		Ogre::Vector3 vInkColour;

		CellState() : 
			bIsBoundary(false),
			vPos(Ogre::Vector3::ZERO),
			vVel(Ogre::Vector3::ZERO),
			vPressureGradient(Ogre::Vector3::ZERO),
			rDivergence(0.0f),
			qRot(Ogre::Quaternion::IDENTITY),
			rPressure(0.0f),
			bActive(false),
			rVorticity(0.0f),
			rInk(0.0f),
			rMaxInk(20.0f),
			vInkColour(Ogre::Vector3(0.0f, 0.0f, 0.0f)) { };

		CellState(
			bool b,
			Ogre::Vector3 vP, 
			Ogre::Vector3 vV,
			Ogre::Real rP,
			Ogre::Vector3 vPG,
			Ogre::Real rDiv,
			Ogre::Quaternion qR, 
			bool a,
			Ogre::Real rVort,
			Ogre::Real rI,
			Ogre::Real rIM,
			Ogre::Vector3 vIn) {
			bIsBoundary = b;
			vPos = vP;
			vVel = vV;
			rPressure = rP;
			vPressureGradient = vPG;
			rDivergence = rDiv,
			qRot = qR;
			bActive = a;
			rVorticity = rVort;
			rInk = rI;
			rMaxInk = rIM;
			vInkColour = vIn;
		};
	};

	class Cell
	{

protected:

		float	mScale;
		int mRowCount;
		int mColumnCount;
		bool mBoundary;
		bool mVelocityArrowVisible;
		bool mPressureGradientArrowVisible;
		bool mTileVisible;

		CellState					mState;
		CellState					mOriginalState;

		CellCoord					mCellCoords;

		GameEntity					*mVelocityArrowEntity;
		MovableObjectDefinition		*mVelocityArrowMoDef;

		GameEntity					*mPressureGradientArrowEntity;
		MovableObjectDefinition		*mPressureGradientArrowMoDef;

		GameEntity					*mPlaneEntity;
		MovableObjectDefinition		*mPlaneMoDef;

		GameEntity					*mSphereEntity;
		MovableObjectDefinition		*mSphereMoDef;

		GameEntityManager			*mGameEntityManager;

		Ogre::Sphere				*mSphere;

		Ogre::HlmsPbsDatablock* personalDatablock;

		float mMaxPressure;
		float mMaxVelocitySquared;

		virtual void createVelocityArrow(void);

		virtual void createPressureGradientArrow(void);

		virtual void createPressureIndicator(void);

		virtual void createBoundingSphere(void);

		virtual void createBoundingSphereDisplay(void);

		virtual void updatePlaneEntity(void);

	public:
		Cell(float scale,
			 int rowIndex,
			 int columnIndex,
			 int layerIndex,
			 int columnCount,
			 int rowCount,
			 float maxPressure,
			 float maxVelocitySquared,
			 bool velocityArrowVisible,
		  	 bool pressureGradientArrowVisible,
			 bool tileVisible,
			 float maxInk,
			 GameEntityManager* geMgr);

		~Cell();

		GameEntity* getPressureGradientArrowGameEntity(void) { return mPressureGradientArrowEntity; }
		GameEntity* getVelocityArrowGameEntity(void) { return mVelocityArrowEntity; }

		CellCoord getCellCoords(void) { return mCellCoords; }
		CellState getState(void) { return mState; };
		void setState(CellState state) { mState = state; };
		Ogre::Vector3 getVelocity(void) { return mState.vVel; };
		Ogre::Vector3 getPosition(void) { return mState.vPos; };
		bool getIsBoundary(void) { return mBoundary; };
		bool getIsActive(void) { return mState.bActive; };
		Ogre::Sphere* getBoundingSphere(void) { return mSphere; };

		virtual void resetState(void);

		void setVelocity(Ogre::Vector3 v);
		void setInkColour(Ogre::Vector3 v);
		void setPressureGradient(Ogre::Vector3 v);

		virtual Ogre::Real getPressure();

		virtual void setPressure(Ogre::Real p);

		virtual void setActive();

		virtual void unsetActive();

		virtual void updateTransforms(float timeSinceLastFrame, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex);
	};
}