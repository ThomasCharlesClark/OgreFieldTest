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

	class Cell
	{

protected:

		int xIndex;
		int zIndex;
		bool boundary;

		GameEntity* mArrowEntity;
		MovableObjectDefinition* mArrowMoDef;

		GameEntity* mPlaneEntity;
		MovableObjectDefinition* mPlaneMoDef;




		//Ogre::ManualObject* arrowObject;
		//Ogre::SceneNode* arrowNode;

		Ogre::Vector3 initialVelocity;
		Ogre::Vector3 velocity;

		Ogre::Real originalPressure;
		Ogre::Real pressure;

		Ogre::Vector2 gridReference;

		Ogre::Quaternion originalOrientation;

		Ogre::Item* planeItem;

		Ogre::HlmsPbsDatablock* personalDatablock;

		Ogre::Vector3 originalPosition;

		Ogre::Sphere* mSphere;

		virtual void createArrow(GameEntityManager* geMgr);

		virtual void createPlane(GameEntityManager* geMgr);

		virtual void createBoundingSphere(void);

	public:
		Cell(int rowIndex,
			int columnIndex,
			int columnCount,
			int rowCount,
			GameEntityManager* geMgr);

		~Cell();

// Possibly nonsense:

		virtual int getXIndex() { return xIndex; }

		virtual int getYIndex() { return -zIndex; }

		Ogre::Vector3 getVelocity();

		void setVelocity(Ogre::Vector3 v, float timeSinceLast);

		virtual void updatePressureAlpha();

		virtual void setPressure(Ogre::Real p);

		virtual Ogre::Real getPressure();

		//Ogre::SceneNode* getNode() { return arrowNode; };


		virtual void restoreOriginalState();

		virtual void randomiseVelocity();

		virtual void updateVelocity();

		virtual void setActive();

		virtual void unsetActive();

		virtual void setNeighbourly();

		virtual void unsetNeighbourly();

		virtual Ogre::Sphere* getSphere();




// Good stuff:
		bool getIsBoundary() { return boundary; };

		virtual void warpForwardInTime(Ogre::Vector3 v, float timeSinceLast);

		virtual void warpBackInTime(float timeSinceLast);

		virtual void undoTimewarp();

		virtual void update(Ogre::Vector3 velocity, Ogre::Real timeSinceLastFrame);

		virtual void setScale();

		virtual void orientArrow();
	};


}