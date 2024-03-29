
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
        mFieldScale( 1.0f ),
#if OGRE_DEBUG_MODE
        mFieldColumnCount ( 23 ),
        mFieldRowCount ( 23 ),
#else
        mFieldColumnCount(43),
        mFieldRowCount(43),
#endif
        mField( 0 ), 
        mHand( 0 ),
        mMaxInk ( 20.0f ),
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

        if (mField) {
            delete mField;
            mField = 0;
        }

        if (mHand) {
            delete mHand;
            mHand = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::createScene01(void)
    {
        const Ogre::Vector3 origin(-5.0f, 0.0f, 0.0f);
        //const Ogre::Vector3 origin( 0.0f, 0.0f, 0.0f );

        GameEntityManager* geMgr = mLogicSystem->getGameEntityManager();

        mCameraMoDef = new MovableObjectDefinition();
        mCameraMoDef->moType = MoTypeCamera;
        mCameraEntity = geMgr->addGameEntity(
            "mainCamera",
            Ogre::SCENE_DYNAMIC, 
            mCameraMoDef,
//#if OGRE_DEBUG_MODE
//            Ogre::Vector3(0, 15, 35),
//            Ogre::Quaternion(0.983195186, -0.182557389, 0.0f, 0.0f),
//#else
            Ogre::Vector3(0, 35, 70),
            Ogre::Quaternion(0.983195186, -0.182557389, 0.0f, 0.0f),
//#endif
            Ogre::Vector3::UNIT_SCALE);

        float width = mLogicSystem->getWindowWidth(),
            height = mLogicSystem->getWindowHeight();

        mCameraController = new CameraControllerMultiThreading(mCameraEntity, width, height);

        mField = new Field(geMgr, mFieldScale, mFieldColumnCount, mFieldRowCount, mMaxInk);

#if OGRE_DEBUG_MODE
        mHand = new Hand(mFieldColumnCount, mFieldRowCount, mMaxInk, geMgr); //0; //noop
#else
        mHand = new Hand(mFieldColumnCount, mFieldRowCount, mMaxInk, geMgr);
#endif

        if (mHand)
            mField->_notifyHand(mHand);
    }
    //-----------------------------------------------------------------------------------
    void LogicGameState::update(float timeSinceLast)
    {
        const size_t currIdx = mLogicSystem->getCurrentTransformIdx();
        const size_t prevIdx = (currIdx + NUM_GAME_ENTITY_BUFFERS - 1) % NUM_GAME_ENTITY_BUFFERS;

        mField->update(timeSinceLast, currIdx, prevIdx);

        if (mField->getUseComputeSystem() == false)
        {
            if (mInputKeys[0]) // Up Arrow
                mField->increaseVelocity(timeSinceLast);
            if (mInputKeys[1]) // Right Arrow
                mField->rotateVelocityClockwise(timeSinceLast);
            if (mInputKeys[2]) // Down Arrow
                mField->decreaseVelocity(timeSinceLast);
            if (mInputKeys[3]) // Left Arrow
                mField->rotateVelocityCounterClockwise(timeSinceLast);

            if (mInputKeys[8]) // SDL_SCANCODE_KP_PLUS
                mField->addImpulse(timeSinceLast);
            if (mInputKeys[9]) // SDL_SCANCODE_KP_MINUS
                mField->decreasePressure(timeSinceLast);
        }
        else 
        {
            if (mField->getComputeSystem()) {
                if (mInputKeys[0]) // Up Arrow
                    mField->getComputeSystem()->addManualInput(timeSinceLast, Ogre::Vector3(0, 0, -1.0f), 0.0f);
                if (mInputKeys[1]) // Right Arrow
                    mField->getComputeSystem()->addManualInput(timeSinceLast, Ogre::Vector3(1.0f, 0, 0), 0.0f);
                if (mInputKeys[2]) // Down Arrow
                    mField->getComputeSystem()->addManualInput(timeSinceLast, Ogre::Vector3(0, 0, 1.0f), 0.0f);
                if (mInputKeys[3]) // Left Arrow
                    mField->getComputeSystem()->addManualInput(timeSinceLast, Ogre::Vector3(-1.0f, 0, 0), 0.0f);

                if (mInputKeys[8]) // SDL_SCANCODE_KP_PLUS
                    mField->getComputeSystem()->addManualInput(timeSinceLast, Ogre::Vector3(0, 0, 0), 50.0f);
                if (mInputKeys[9]) // SDL_SCANCODE_KP_MINUS
                    mField->getComputeSystem()->addManualInput(timeSinceLast, Ogre::Vector3(0, 0, 0), -50.0f);
            }
        }

        if (mInputKeys[4]) // Num Pad 4
            mField->traverseActiveCellXNegative();
        if (mInputKeys[5]) // Num Pad 6
            mField->traverseActiveCellXPositive();
        if (mInputKeys[6]) // Num Pad 8
            mField->traverseActiveCellZNegative();
        if (mInputKeys[7]) // Num Pad 2
            mField->traverseActiveCellZPositive();

        if (mInputKeys[10]) // SDL_SCANCODE_5
            mField->togglePressureGradientIndicators();

        if (mInputKeys[11]) // SDL_SCANCODE_6
            mField->toggleVelocityIndicators();

        if (mInputKeys[12]) // SDL_SCANCODE_P
            if (mField->getComputeSystem())
                mField->getComputeSystem()->writeDebugImages(timeSinceLast);

        mInputKeys[4] = false; // Num Pad 4
        mInputKeys[5] = false; // Num Pad 6
        mInputKeys[6] = false; // Num Pad 8
        mInputKeys[7] = false; // Num Pad 2
        mInputKeys[8] = false; // Num Pad Plus
        mInputKeys[9] = false; // Num Pad Minus
        mInputKeys[10] = false; // Number Row 5
        mInputKeys[11] = false; // Number Row 6
        mInputKeys[12] = false; // P key

        if (mCameraController)
            mCameraController->update(timeSinceLast, currIdx, prevIdx, mLogicSystem->getMouseX(), mLogicSystem->getMouseY());

        if (mHand)
            mHand->update(timeSinceLast, currIdx, prevIdx);

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
                mField->notifyShiftKey(true);
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
            case SDL_SCANCODE_KP_PLUS:
                mInputKeys[8] = true;
                break;
            case SDL_SCANCODE_KP_MINUS:
                mInputKeys[9] = true;
                break;
            case SDL_SCANCODE_5:
                mInputKeys[10] = true;
                break;
            case SDL_SCANCODE_6:
                mInputKeys[11] = true;
                break;
            case SDL_SCANCODE_RETURN:
                mField->toggleIsRunning();
                break;
            case SDL_SCANCODE_ESCAPE:
                mField->clearActiveCell();
                break;
            case SDL_SCANCODE_P:
                mInputKeys[12] = true;
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
                mField->notifyShiftKey(false);
                break;
            case SDL_SCANCODE_F5:
                if (mField->getUseComputeSystem() == false)
                    mField->resetState();
                else
                    mField->getComputeSystem()->reset();
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
