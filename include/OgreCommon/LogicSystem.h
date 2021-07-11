
#ifndef _Demo_LogicSystem_H_
#define _Demo_LogicSystem_H_

#include "BaseSystem.h"
#include "OgrePrerequisites.h"
#include "SDL_events.h"

#include <deque>

namespace MyThirdOgre
{
    class GameEntityManager;

    class LogicSystem : public BaseSystem
    {
    protected:
        BaseSystem          *mGraphicsSystem;
        GameEntityManager   *mGameEntityManager;

        Ogre::uint32                mCurrentTransformIdx;
        std::deque<Ogre::uint32>    mAvailableTransformIdx;

        /// @see MessageQueueSystem::processIncomingMessage
        virtual void processIncomingMessage( Mq::MessageId messageId, const void *data );

    public:
        LogicSystem( GameState *gameState );
        virtual ~LogicSystem();

        void _notifyGraphicsSystem( BaseSystem *graphicsSystem )    { mGraphicsSystem = graphicsSystem; }
        void _notifyGameEntityManager( GameEntityManager *mgr )     { mGameEntityManager = mgr; }

        void finishFrameParallel(void);

        GameEntityManager* getGameEntityManager(void)               { return mGameEntityManager; }
        Ogre::uint32 getCurrentTransformIdx(void) const             { return mCurrentTransformIdx; }

        virtual void keyPressed(const SDL_KeyboardEvent& arg);
        virtual void keyReleased(const SDL_KeyboardEvent& arg);
        virtual void mouseMoved(const SDL_MouseMotionEvent& arg);
        virtual void mouseWheelChanged(const SDL_MouseWheelEvent& arg);
    };
}

#endif
