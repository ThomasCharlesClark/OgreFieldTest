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
	struct HandInfluence 
	{
		Ogre::Vector3 vVelocity;
		bool bAddingInk;

		HandInfluence() {};

		/*HandInfluence() 
		{
			vVelocity = Ogre::Vector3::ZERO;
			bAddingInk = false;
		};*/

		HandInfluence(Ogre::Vector3 v, bool ai)
		{
			vVelocity = v;
			bAddingInk = ai;
		};
	};

	struct HandState
	{
		bool bIsVisible;
		Ogre::Vector3 vPos;
		Ogre::Vector3 vVel;
		Ogre::Quaternion qRot;
		bool bActive;
		bool bAddingInk;

		HandState() :
			bIsVisible(false),
			vPos(Ogre::Vector3::ZERO),
			vVel(Ogre::Vector3::ZERO),
			qRot(Ogre::Quaternion::IDENTITY),
			bAddingInk(false),
			bActive(false) { }

		HandState(
			bool bV,
			Ogre::Vector3 vP,
			Ogre::Vector3 vV,
			Ogre::Quaternion qR,
			bool a,
			bool aI) {
			bIsVisible = bV;
			vPos = vP;
			vVel = vV;
			qRot = qR;
			bActive = a;
			bAddingInk = aI;
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
		virtual void setInk(float timeSinceLast, bool addingInk);
	};
}
#endif