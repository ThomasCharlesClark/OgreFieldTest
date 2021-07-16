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

namespace MyThirdOgre 
{
	class GameEntityManager;

	struct CellState
	{
		Ogre::Vector3 vVel;
		Ogre::Quaternion qRot;
		Ogre::Real rPressure;

		CellState() {};

		CellState(Ogre::Vector3 v, Ogre::Quaternion q, Ogre::Real p) {};
	};

	class Cell
	{

protected:

		int mIndexX;
		int mIndexZ;
		int mRowCount;
		int mColumnCount;
		bool mBoundary;

		CellState					mState;
		CellState					mOriginalState;

		GameEntity					*mArrowEntity;
		MovableObjectDefinition		*mArrowMoDef;

		GameEntity					*mPlaneEntity;
		MovableObjectDefinition		*mPlaneMoDef;

		GameEntity					*mSphereEntity;
		MovableObjectDefinition		*mSphereMoDef;

		Ogre::Sphere				*mSphere;

		Ogre::HlmsPbsDatablock* personalDatablock;

		virtual void createArrow(GameEntityManager* geMgr);

		virtual void createPressureIndicator(GameEntityManager* geMgr);

		virtual void createBoundingSphere(GameEntityManager* geMgr);

	public:
		Cell(int rowIndex,
			int columnIndex,
			int columnCount,
			int rowCount,
			GameEntityManager* geMgr);

		~Cell();

// Possibly nonsense:

		virtual int getXIndex() { return mIndexX; }

		virtual int getYIndex() { return -mIndexZ; }

		Ogre::Vector3 getVelocity();

		void setVelocity(Ogre::Vector3 v, float timeSinceLast);

		virtual void updatePressureAlpha();

		virtual void setPressure(Ogre::Real p);

		virtual Ogre::Real getPressure();

		//Ogre::SceneNode* getNode() { return arrowNode; };


		virtual void restoreOriginalState();

		//virtual void randomiseVelocity();

		//virtual void updateVelocity();

		//virtual void setActive();

		//virtual void unsetActive();

		//virtual void setNeighbourly();

		//virtual void unsetNeighbourly();

		//virtual Ogre::Sphere* getSphere();




// Good stuff:
		bool getIsBoundary() { return mBoundary; };

		virtual void warpForwardInTime(Ogre::Vector3 v, float timeSinceLast);

		virtual void warpBackInTime(float timeSinceLast);

		virtual void undoTimewarp();

		virtual void update(Ogre::Vector3 velocity, Ogre::Real timeSinceLastFrame);

		virtual void setScale();

		virtual void orientArrow();
	};


}