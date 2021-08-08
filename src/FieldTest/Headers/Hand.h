#ifndef _HAND_H
#define _HAND_H

#include "OgreManualObject.h"
#include "OgrePrerequisites.h"
#include "OgreQuaternion.h"
#include "OgreEntity.h"

#include "GameEntity.h"
#include "GameEntityManager.h"

namespace MyThirdOgre
{
	struct HandState
	{
		bool bIsVisible;
		Ogre::Vector3 vPos;
		Ogre::Vector3 vVel;
		Ogre::Quaternion qRot;
		bool bActive;

		HandState() :
			bIsVisible(false),
			vPos(Ogre::Vector3::ZERO),
			vVel(Ogre::Vector3::ZERO),
			qRot(Ogre::Quaternion::IDENTITY),
			bActive(false) { }

		HandState(
			bool bV,
			Ogre::Vector3 vP,
			Ogre::Vector3 vV,
			Ogre::Quaternion qR,
			bool a) {
			bIsVisible = bV;
			vPos = vP;
			vVel = vV;
			qRot = qR;
			bActive = a;
		}
	};

	class GameEntityManager;
	
	class Hand
	{

	protected: 
		HandState					mState;
		HandState					mOriginalState;

		GameEntity*					mHandEntity;
		MovableObjectDefinition*	mHandMoDef;

		int							mColumnCount;
		int							mRowCount;
		GameEntityManager*			mGameEntityManager;

		// bounding sphere
		Ogre::Sphere				*mSphere;

	public:
		Hand(
			int columnCount,
			int rowCount,
			GameEntityManager* geMgr);
		~Hand();


		Ogre::Sphere* getBoundingSphere(void) { return mSphere; };

		HandState getState(void) { return mState; };

		virtual void initialise(void);
		virtual void update(float timeSinceLast, Ogre::uint32 currIdx, Ogre::uint32 prevIdx);
		virtual void setPosition(float timeSinceLast, Ogre::Vector3 position);
		virtual void setVelocity(float timeSinceLast, Ogre::Vector3 velocity);
	};
}
#endif