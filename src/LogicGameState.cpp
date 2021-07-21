
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
            mField->increaseVelocity(timeSinceLast);
        if (mInputKeys[1]) // Right Arrow
            mField->rotateVelocityClockwise(timeSinceLast);
        if (mInputKeys[2]) // Down Arrow
            mField->decreaseVelocity(timeSinceLast);
        if (mInputKeys[3]) // Left Arrow
            mField->rotateVelocityCounterClockwise(timeSinceLast);

        if (mInputKeys[4]) // Num Pad 4
            mField->traverseActiveCellXNegative();
        if (mInputKeys[5]) // Num Pad 6
            mField->traverseActiveCellXPositive();
        if (mInputKeys[6]) // Num Pad 8
            mField->traverseActiveCellZNegative();
        if (mInputKeys[7]) // Num Pad 2
            mField->traverseActiveCellZPositive();

        mInputKeys[4] = false; // Num Pad 4
        mInputKeys[5] = false; // Num Pad 6
        mInputKeys[6] = false; // Num Pad 8
        mInputKeys[7] = false; // Num Pad 2

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
            case SDL_SCANCODE_LSHIFT:
                mField->notifyShift(true);
                break;
            case SDL_SCANCODE_KP_4:
                mInputKeys[4] = true;
                break;
            case SDL_SCANCODE_KP_6:
                mInputKeys[5] = true;
                break;
            case SDL_SCANCODE_KP_8:
                mInputKeys[6] = true;
                break;
            case SDL_SCANCODE_KP_2:
                mInputKeys[7] = true;
                break;
            case SDL_SCANCODE_ESCAPE:
                mField->clearActiveCell();
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
            case SDL_SCANCODE_LSHIFT:
                mField->notifyShift(false);
                break;
            case SDL_SCANCODE_F5:
                mField->resetState();
                break;
           /* case SDL_SCANCODE_KP_4:
                mInputKeys[4] = false;
                break;
            case SDL_SCANCODE_KP_6:
                mInputKeys[5] = false;
                break;
            case SDL_SCANCODE_KP_8:
                mInputKeys[6] = false;
                break;
            case SDL_SCANCODE_KP_2:
                mInputKeys[7] = false;
                break;*/
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
