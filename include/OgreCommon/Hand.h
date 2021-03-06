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
		Ogre::Real rInkAmount;

		HandInfluence() {};

		/*HandInfluence() 
		{
			vVelocity = Ogre::Vector3::ZERO;
			bAddingInk = false;
		};*/

		HandInfluence(Ogre::Vector3 v, Ogre::Real ri)
		{
			vVelocity = v;
			rInkAmount = ri;
		};
	};

	struct HandState
	{
		bool bIsVisible;
		Ogre::Vector3 vPos;
		Ogre::Vector3 vPosPrev;
		Ogre::Vector3 vVel;
		Ogre::Quaternion qRot;
		Ogre::Real rInk;
		Ogre::Real rMaxInk;
		bool bActive;

		HandState() :
			bIsVisible(false),
			vPos(Ogre::Vector3::ZERO),
			vPosPrev(Ogre::Vector3::ZERO),
			vVel(Ogre::Vector3::ZERO),
			qRot(Ogre::Quaternion::IDENTITY),
			rInk(0.0f),
			rMaxInk(20.0f),
			bActive(false) { }

		HandState(
			bool bV,
			Ogre::Vector3 vP,
			Ogre::Vector3 vPp,
			Ogre::Vector3 vV,
			Ogre::Quaternion qR,
			Ogre::Real rI,
			Ogre::Real rIM,
			bool a) {
			bIsVisible = bV;
			vPos = vP;
			vPosPrev = vPp;
			vVel = vV;
			qRot = qR;
			rInk = rI;
			rMaxInk = rIM;
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
		Ogre::Sphere				*mOuterSphere;

	public:
		Hand(
			int columnCount,
			int rowCount,
			float maxInk,
			GameEntityManager* geMgr);
		~Hand();


		Ogre::Sphere* getBoundingSphere(void) { return mSphere; };
		Ogre::Sphere* getOuterBoundingSphere(void) { return mOuterSphere; };

		HandState getState(void) { return mState; };

		virtual void initialise(void);
		virtual void update(float timeSinceLast, Ogre::uint32 currIdx, Ogre::uint32 prevIdx);
		virtual void setPosition(float timeSinceLast, Ogre::Vector3 position);
		virtual void setVelocity(float timeSinceLast, Ogre::Vector3 velocity);
		virtual void setInk(float timeSinceLast, Ogre::Real ink);
	};
}
#endif