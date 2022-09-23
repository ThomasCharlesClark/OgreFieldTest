#include "Leap/LeapSystem.h"
#include "Leap/LeapSystemState.h"
#include "OgrePrerequisites.h"
#include "OgreVector3.h"
//
//#include <Windows.h>
//#include <iostream>
//#include <sstream>
//
//void DBOut(const WCHAR* s)
//{
//    std::wostringstream os_;
//    os_ << s << std::endl;
//    OutputDebugStringW(os_.str().c_str());
//}

namespace MyThirdOgre
{
    LeapSystem::LeapSystem(GameState* gameState) :
        BaseSystem(gameState),
        mConnectionHandle(0),
        mPreviousTrackingMessage(LEAP_CONNECTION_MESSAGE()),
        mRunning(false),
        mVelocityScalingFactor(0.1f),
        mPositionScalingFactor(5.0f)
    {

    }

    //-----------------------------------------------------------------------------------
    LeapSystem::~LeapSystem(void) {

    }

    //-----------------------------------------------------------------------------------
    void LeapSystem::initialize(void) {
        createLeapConnection();
    }

    //-----------------------------------------------------------------------------------
    void LeapSystem::deinitialize(void) {
        destroyLeapConnection();
    }

    //-----------------------------------------------------------------------------------
    void LeapSystem::createLeapConnection(void) {
        eLeapRS result = LeapCreateConnection(NULL, &mConnectionHandle);
        if (result == eLeapRS_Success) {
            result = LeapOpenConnection(mConnectionHandle);
            if (result == eLeapRS_Success) {
                mRunning = true;
                pollConnection(0.0f);
            }
        }
    }

    //-----------------------------------------------------------------------------------
    void LeapSystem::destroyLeapConnection(void) {
        if (mConnectionHandle) {
            mRunning = false;
            LeapDestroyConnection(mConnectionHandle);
            mConnectionHandle = 0;
        }
    }

    //-----------------------------------------------------------------------------------
    void LeapSystem::pollConnection(float timeSinceLast) {
        if (mRunning) {
            eLeapRS result;
            LEAP_CONNECTION_MESSAGE msg;

            result = LeapPollConnection(mConnectionHandle, 0, &msg);
            //Handle message

            switch (msg.type) {
            case eLeapEventType_Tracking: 
                {
                    if (msg.tracking_event->nHands >= 1)
                    {

                        Ogre::Vector3 vVel = Ogre::Vector3(msg.tracking_event->pHands[0].palm.velocity.v);
                     
                        /*Ogre::Vector3 vVel = Ogre::Vector3(msg.tracking_event->pHands[0].palm.velocity.x,
                            0,
                            0);*/

                        Ogre::Vector3 vPos = Ogre::Vector3(msg.tracking_event->pHands[0].palm.position.v);

                        vPos /= mPositionScalingFactor;

                        vPos.y -= 50;

                        //vVel.normalise();

                        vVel *= mVelocityScalingFactor;

                        //vVel.y = 0;

                        bool anyFingerExtended = true;
                        
                        anyFingerExtended = msg.tracking_event->pHands[0].index.is_extended
                            || msg.tracking_event->pHands[0].middle.is_extended
                            || msg.tracking_event->pHands[0].ring.is_extended
                            || msg.tracking_event->pHands[0].pinky.is_extended;

                        Leap_MotionMessage vMsg = Leap_MotionMessage(timeSinceLast, vVel, vPos, anyFingerExtended ? 5.0f : -5.0f);
                        //Leap_MotionMessage vMsg = Leap_MotionMessage(timeSinceLast, vVel, vPos, 10.0f);

                        this->queueSendMessage(mLogicSystem, Mq::LEAPFRAME_MOTION, vMsg);

                        mPreviousTrackingMessage = msg;
                    }
                }
                break;
            }
        }
    }
    
    //-----------------------------------------------------------------------------------
    void LeapSystem::processIncomingMessage(Mq::MessageId messageId, const void* data)
    {

    }

    //-----------------------------------------------------------------------------------
    void LeapSystem::finishFrameParallel(void)
    {
        //Notify the GraphicsSystem we're done with this frame.
        if (mGraphicsSystem)
        {
            this->queueSendMessage(mGraphicsSystem, Mq::LEAPFRAME_FINISHED, NULL);
        }

        BaseSystem::finishFrameParallel();
    }
}