
#include "LogicGameState.h"
#include "GameEntityManager.h"
#include "LogicSystem.h"
#include "OgreResourceGroupManager.h"
#include "OgreVector3.h"

using namespace MyThirdOgre;

namespace MyThirdOgre
{
    LogicGameState::LogicGameState() :
        mDisplacement(0), mField(0), mLogicSystem(0), mRotationRadians(0)
    {
    }
    //-----------------------------------------------------------------------------------
    LogicGameState::~LogicGameState()
    {
        delete mField;
        mField = 0;
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::createScene01(void)
    {
        const Ogre::Vector3 origin(-5.0f, 0.0f, 0.0f);
        //const Ogre::Vector3 origin( 0.0f, 0.0f, 0.0f );

        GameEntityManager* geMgr = mLogicSystem->getGameEntityManager();

        mField = new Field(geMgr);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::update(float timeSinceLast)
    {
        const Ogre::Vector3 origin(-5.0f, 0.0f, 0.0f);

        mDisplacement += timeSinceLast * 40.0f;
        mDisplacement = fmodf(mDisplacement, 10.0f);

        mRotationRadians += timeSinceLast * 2.0f;
        mRotationRadians = fmodf(mRotationRadians, 6.28318531f);

        const size_t currIdx = mLogicSystem->getCurrentTransformIdx();
        const size_t prevIdx = (currIdx + NUM_GAME_ENTITY_BUFFERS - 1) % NUM_GAME_ENTITY_BUFFERS;

        //mCubeEntity->mTransform[currIdx]->vPos = origin + Ogre::Vector3::UNIT_X * mDisplacement;

        // This code will read our last position we set and update it to the the new buffer.
        // Graphics will be reading mCubeEntity->mTransform[prevIdx]; as long as we don't
        // write to it, we're ok.
 /*       mCubeEntity->mTransform[currIdx]->vPos = mCubeEntity->mTransform[prevIdx]->vPos +
                                                      Ogre::Vector3::UNIT_X * timeSinceLast;*/

        GameState::update(timeSinceLast);
    }
}
