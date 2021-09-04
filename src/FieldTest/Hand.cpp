#include "Hand.h"
#include "GameEntityManager.h"

namespace MyThirdOgre 
{
	Hand::Hand(
        int columnCount, 
        int rowCount,
        float maxInk,
        GameEntityManager* geMgr
    ) :
            mColumnCount(columnCount),
            mRowCount(rowCount),
            mGameEntityManager(geMgr)
	{
        mState =
        {
            mState.bIsVisible = true,
            mState.vPos = Ogre::Vector3(-16.0f, 0.0f, 16.0f),
            mState.vVel = Ogre::Vector3::ZERO,
            mState.qRot = Ogre::Quaternion::IDENTITY,
            mState.rInk = 0.0f,
            mState.rMaxInk = maxInk,
            mState.bActive = true
        };

        mOriginalState = HandState(mState);

        initialise();
	}

	Hand::~Hand() 
	{
        if (mHandMoDef) {
            delete mHandMoDef;
            mHandMoDef = 0;
        }
	}

    void Hand::initialise(void) 
    {
        mHandMoDef = new MovableObjectDefinition();
        mHandMoDef->meshName = "Sphere1000.mesh";
        mHandMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mHandMoDef->moType = MoTypeItem;

        Ogre::String name = "leapHand";

        mHandEntity = mGameEntityManager->addGameEntity(
            name,
            Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mHandMoDef,
            mState.vPos,
            mState.qRot,
            Ogre::Vector3::UNIT_SCALE * 3,
            true,
            0.4f);


        mSphere = new Ogre::Sphere(mState.vPos, 1.5f);
    }

    void Hand::update(float timeSinceLast, Ogre::uint32 currIdx, Ogre::uint32 prevIdx) 
    {
        auto q = mHandEntity->mTransform[prevIdx]->qRot;

        mHandEntity->mTransform[currIdx]->vPos = mState.vPos;
    }

    void Hand::setPosition(float timeSinceLast, Ogre::Vector3 position) 
    {
        mState.vPos = position;
        mSphere->setCenter(mState.vPos);
    }

    void Hand::setVelocity(float timeSinceLast, Ogre::Vector3 velocity) 
    {
        mState.vVel = velocity;
    }

    void Hand::setInk(float timeSinceLast, Ogre::Real ink)
    {
        if (ink < mState.rMaxInk)
            mState.rInk = ink;
        else
            mState.rInk = mState.rMaxInk;
    }
}