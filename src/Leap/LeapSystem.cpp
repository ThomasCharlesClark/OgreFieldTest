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
        mVelocityScalingFactor(2.0f)
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

                //wchar_t mHandCount[256];

                //swprintf_s(mHandCount, L"%d", msg.tracking_event->nHands);

                //DBOut(mHandCount);

                if (msg.tracking_event->nHands >= 1) {

                    Ogre::Vector3 vVel = Ogre::Vector3(msg.tracking_event->pHands[0].palm.velocity.v);

                    Ogre::Vector3 vPos = Ogre::Vector3(msg.tracking_event->pHands[0].palm.position.v);

                    if (mPreviousTrackingMessage.pointer != NULL)
                        if (msg.tracking_event->pHands[0].palm.position.v != mPreviousTrackingMessage.tracking_event->pHands[0].palm.position.v)
                            vPos = Ogre::Vector3(msg.tracking_event->pHands[0].palm.position.v - mPreviousTrackingMessage.tracking_event->pHands[0].palm.position.v);
                    
                    vPos = 1 / vPos.normalise();

                    vPos *= mVelocityScalingFactor;

                    vVel.normalise();

                    vVel *= mVelocityScalingFactor;

					//auto y = vVel.y;
					//auto z = vVel.z;

					//vVel.z = y;
					//vVel.y = z;

                    Leap_VelocityMessage vMsg = Leap_VelocityMessage(timeSinceLast, vVel);

                    //this->queueSendMessage(mLogicSystem, Mq::LEAPFRAME_VELOCITY, vMsg);

                    mLogicSystem->receiveMessageImmediately(Mq::LEAPFRAME_VELOCITY, vMsg);

                    mPreviousTrackingMessage = msg;
                }
                
                break;
            }

            int f = 0;
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