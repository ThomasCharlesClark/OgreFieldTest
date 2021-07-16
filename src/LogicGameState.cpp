
#include "LogicGameState.h"
#include "GameEntityManager.h"
#include "LogicSystem.h"
#include "OgreResourceGroupManager.h"
#include "OgreVector3.h"
#include "CameraController.h"

using namespace MyThirdOgre;

namespace MyThirdOgre
{
    LogicGameState::LogicGameState() :
        mField( 0 ), 
        mLogicSystem( 0 ), 
        mCameraController( 0 ),
        mCameraMoDef ( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    LogicGameState::~LogicGameState()
    {
        if (mCameraMoDef) {
            delete mCameraMoDef;
            mCameraMoDef = 0;
        }

        if (mCameraController) {
            delete mCameraController;
            mCameraController = 0;
        }

        delete mField;
        mField = 0;
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::createScene01(void)
    {
        const Ogre::Vector3 origin(-5.0f, 0.0f, 0.0f);
        //const Ogre::Vector3 origin( 0.0f, 0.0f, 0.0f );

        GameEntityManager* geMgr = mLogicSystem->getGameEntityManager();

        mCameraMoDef = new MovableObjectDefinition();
        mCameraMoDef->moType = MoTypeCamera;
        mCameraEntity = geMgr->addGameEntity(Ogre::SCENE_DYNAMIC, mCameraMoDef,
            Ogre::Vector3(11, 10, 15),
            Ogre::Quaternion(0.983195186, -0.182557389, 0.0f, 0.0f),
            Ogre::Vector3::UNIT_SCALE);

        float width = mLogicSystem->getWindowWidth(),
            height = mLogicSystem->getWindowHeight();

        mCameraController = new CameraControllerMultiThreading(mCameraEntity, width, height);

        mField = new Field(geMgr);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::update(float timeSinceLast)
    {
        const size_t currIdx = mLogicSystem->getCurrentTransformIdx();
        const size_t prevIdx = (currIdx + NUM_GAME_ENTITY_BUFFERS - 1) % NUM_GAME_ENTITY_BUFFERS;



        //mCubeEntity->mTransform[currIdx]->vPos = origin + Ogre::Vector3::UNIT_X * mDisplacement;

        // This code will read our last position we set and update it to the the new buffer.
        // Graphics will be reading mCubeEntity->mTransform[prevIdx]; as long as we don't
        // write to it, we're ok.
        /*mCubeEntity->mTransform[currIdx]->vPos = mCubeEntity->mTransform[prevIdx]->vPos +
                                                      Ogre::Vector3::UNIT_X * timeSinceLast;*/

        if (mCameraController)
            mCameraController->update(timeSinceLast, currIdx, prevIdx, mLogicSystem->getMouseX(), mLogicSystem->getMouseY());

        GameState::update(timeSinceLast);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::keyPressed(const SDL_KeyboardEvent& arg)
    {
        if (mCameraController)
            mCameraController->keyPressed(arg);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::keyReleased(const SDL_KeyboardEvent& arg)
    {
        if (mCameraController)
            mCameraController->keyReleased(arg);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::mouseMoved(const SDL_Event& arg) 
    {
        if (mCameraController)
            mCameraController->mouseMoved(arg);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::mousePressed(const SDL_MouseButtonEvent& arg, Ogre::uint8 buttonId)
    {

    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::mouseReleased(const SDL_MouseButtonEvent& arg, Ogre::uint8 buttonId)
    {

    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::mouseWheelChanged(const SDL_MouseWheelEvent& arg)
    {

    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::textEditing(const SDL_TextEditingEvent& arg)
    {

    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::textInput(const SDL_TextInputEvent& arg)
    {

    }
}
