
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
        mCameraMoDef ( 0 ),
        mSpaceKey ( false )
    {
        memset(mInputKeys, 0, sizeof(mInputKeys));
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

        mField->update(timeSinceLast, currIdx, prevIdx);

        if (mInputKeys[0]) // Up Arrow
            mField->traverseActiveCellZNegative();
        if (mInputKeys[1]) // Right Arrow
            mField->traverseActiveCellXPositive();
        if (mInputKeys[2]) // Down Arrow
            mField->traverseActiveCellZPositive();
        if (mInputKeys[3]) // Left Arrow
            mField->traverseActiveCellXNegative();

        if (mCameraController)
            mCameraController->update(timeSinceLast, currIdx, prevIdx, mLogicSystem->getMouseX(), mLogicSystem->getMouseY());

        GameState::update(timeSinceLast);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::keyPressed(const SDL_KeyboardEvent& arg)
    {
        switch (arg.keysym.scancode) {
            case SDL_SCANCODE_SPACE:
                mSpaceKey = true;
                break;
            case SDL_SCANCODE_UP:
                mInputKeys[0] = true;
                break;
            case SDL_SCANCODE_RIGHT:
                mInputKeys[1] = true;
                break;
            case SDL_SCANCODE_DOWN:
                mInputKeys[2] = true;
                break;
            case SDL_SCANCODE_LEFT:
                mInputKeys[3] = true;
                break;
            default:
                break;
        }

        if (mCameraController)
            mCameraController->keyPressed(arg);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::keyReleased(const SDL_KeyboardEvent& arg)
    {
        switch (arg.keysym.scancode) {
            case SDL_SCANCODE_SPACE:
                mSpaceKey = false;
                break;
            case SDL_SCANCODE_UP:
                mInputKeys[0] = false;
                break;
            case SDL_SCANCODE_RIGHT:
                mInputKeys[1] = false;
                break;
            case SDL_SCANCODE_DOWN:
                mInputKeys[2] = false;
                break;
            case SDL_SCANCODE_LEFT:
                mInputKeys[3] = false;
                break;
            default:
                break;
        }

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
