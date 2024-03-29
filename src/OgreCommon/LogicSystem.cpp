
#include "LogicSystem.h"
#include "GameState.h"
#include "SdlInputHandler.h"
#include "GameEntityManager.h"

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreConfigFile.h"

#include "OgreCamera.h"

#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreArchiveManager.h"

#include "Compositor/OgreCompositorManager2.h"

#include "OgreOverlaySystem.h"

#include "Leap\LeapSystem.h"
#include "LogicGameState.h"

#if OGRE_USE_SDL2
    #include <SDL_syswm.h>
#endif

namespace MyThirdOgre
{
    LogicSystem::LogicSystem( GameState *gameState ) :
        BaseSystem( gameState ),
        mGraphicsSystem( 0 ),
        mGameEntityManager( 0 ),
        mCurrentTransformIdx( 1 ),
        mWindowWidth( 0 ),
        mWindowHeight( 0 ),
        mMouseX ( new int ),
        mMouseY ( new int )
    {
        //mCurrentTransformIdx is 1, 0 and NUM_GAME_ENTITY_BUFFERS - 1 are taken by GraphicsSytem at startup
        //The range to fill is then [2; NUM_GAME_ENTITY_BUFFERS-1]
        for( Ogre::uint32 i=2; i<NUM_GAME_ENTITY_BUFFERS-1; ++i )
            mAvailableTransformIdx.push_back( i );
    }
    //-----------------------------------------------------------------------------------
    LogicSystem::~LogicSystem()
    {
        delete mMouseX;
        mMouseX = 0;

        delete mMouseY;
        mMouseY = 0;
    }
    //-----------------------------------------------------------------------------------
    void LogicSystem::_notifyWindowWidth(float width)
    {
        mWindowWidth = width;
    }
    //-----------------------------------------------------------------------------------
    void LogicSystem::_notifyWindowHeight(float height)
    {
        mWindowHeight = height;
    }
    //-----------------------------------------------------------------------------------
    void LogicSystem::finishFrameParallel(void)
    {
        if( mGameEntityManager )
            mGameEntityManager->finishFrameParallel();

        //Notify the GraphicsSystem we're done rendering this frame.
        if( mGraphicsSystem )
        {
            size_t idxToSend = mCurrentTransformIdx;

            if( mAvailableTransformIdx.empty() )
            {
                //Don't relinquish our only ID left.
                //If you end up here too often, Graphics' thread is too slow,
                //or you need to increase NUM_GAME_ENTITY_BUFFERS
                idxToSend = std::numeric_limits<Ogre::uint32>::max();
            }
            else
            {
                //Until Graphics constantly releases the indices we send them, to avoid writing
                //to transform data that may be in use by the other thread (race condition)
                mCurrentTransformIdx = mAvailableTransformIdx.front();
                mAvailableTransformIdx.pop_front();
            }

            this->queueSendMessage( mGraphicsSystem, Mq::LOGICFRAME_FINISHED, idxToSend );
        }

        BaseSystem::finishFrameParallel();
    }
    //-----------------------------------------------------------------------------------
    void LogicSystem::processIncomingMessage( Mq::MessageId messageId, const void *data )
    {
        switch( messageId )
        {
        case Mq::LOGICFRAME_FINISHED:
            {
                Ogre::uint32 newIdx = *reinterpret_cast<const Ogre::uint32*>( data );
                assert( (mAvailableTransformIdx.empty() ||
                        newIdx == (mAvailableTransformIdx.back() + 1) % NUM_GAME_ENTITY_BUFFERS) &&
                        "Indices are arriving out of order!!!" );

                mAvailableTransformIdx.push_back( newIdx );
            }
            break;
        case Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT:
            mGameEntityManager->_notifyGameEntitiesRemoved( *reinterpret_cast<const Ogre::uint32*>(
                                                                data ) );
            break;
        case Mq::SDL_EVENT:
            {
                //TODO
                auto evt = *reinterpret_cast<const SDL_Event*>(data);
                switch (evt.type) {
                    case SDL_KEYDOWN:
                        mCurrentGameState->keyPressed(*reinterpret_cast<const SDL_KeyboardEvent*>(data));
                        break;
                    case SDL_KEYUP:
                        mCurrentGameState->keyReleased(*reinterpret_cast<const SDL_KeyboardEvent*>(data));
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        mCurrentGameState->mousePressed(*reinterpret_cast<const SDL_MouseButtonEvent*>(data), evt.button.button);
                        break;
                    case SDL_MOUSEBUTTONUP:
                        mCurrentGameState->mouseReleased(*reinterpret_cast<const SDL_MouseButtonEvent*>(data), evt.button.button);
                        break;
                    case SDL_MOUSEMOTION:
                        mCurrentGameState->mouseMoved(evt);
                        break;
                    case SDL_MOUSEWHEEL:
                        mCurrentGameState->mouseWheelChanged(*reinterpret_cast<const SDL_MouseWheelEvent*>(data));
                        break;
                    default: 
                        break;
                }
            }
            break;
        case Mq::LEAPFRAME_MOTION:
            {
                auto hand = dynamic_cast<LogicGameState*>(mCurrentGameState)->getHand();

                if (hand) {
                    auto leapMotionFrame = reinterpret_cast<const Leap_MotionMessage*>(data);
                    hand->setPosition(leapMotionFrame->timeSinceLast, leapMotionFrame->position);
                    hand->setVelocity(leapMotionFrame->timeSinceLast, leapMotionFrame->velocity);
                    hand->setInk(leapMotionFrame->timeSinceLast, leapMotionFrame->ink);
                }

            }
            break;
        case Mq::FIELD_COMPUTE_SYSTEM_WRITE_VELOCITIES:
            {
                auto field = dynamic_cast<LogicGameState*>(mCurrentGameState)->getField();

                if (field) {
                    auto velocityMsg = reinterpret_cast<const FieldComputeSystem_VelocityMessage*>(data);

                    auto c = field->getCell(velocityMsg->velocity.first);
                    
                    if (c) {
                        c->setVelocity(velocityMsg->velocity.second.vVelocity);
                    }
                    else {
                        int f = 0;
                    }
                }
            }
            break;
        case Mq::FIELD_COMPUTE_SYSTEM_RECIEVE_STAGING_TEXTURE:
        {
            auto field = dynamic_cast<LogicGameState*>(mCurrentGameState)->getField();

            if (field && field->getComputeSystem()) {
                auto stagingTextureMessage = reinterpret_cast<const FieldComputeSystem_StagingTextureMessage*>(data);

                field->getComputeSystem()->receiveStagingTextureAndReset(stagingTextureMessage->mStagingTexture);
            }
        }
        break;
        default:
            break;
        }
    }
}