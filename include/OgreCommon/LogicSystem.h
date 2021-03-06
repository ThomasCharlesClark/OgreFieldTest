
#ifndef _Demo_LogicSystem_H_
#define _Demo_LogicSystem_H_

#include "BaseSystem.h"
#include "OgrePrerequisites.h"
#include "SDL_events.h"
#include "OgreWindow.h"
#include "GraphicsSystem.h"

#include <deque>

namespace MyThirdOgre
{
    class GameEntityManager;
    class CameraController;
    class GraphicsSystem;

    class LogicSystem : public BaseSystem
    {
        float               mWindowWidth;
        float               mWindowHeight;

        int* mMouseX;
        int* mMouseY;

    protected:
        GraphicsSystem          *mGraphicsSystem;
        GameEntityManager   *mGameEntityManager;

        Ogre::uint32                mCurrentTransformIdx;
        std::deque<Ogre::uint32>    mAvailableTransformIdx;

        /// @see MessageQueueSystem::processIncomingMessage
        virtual void processIncomingMessage( Mq::MessageId messageId, const void *data );

    public:
        LogicSystem( GameState *gameState );
        virtual ~LogicSystem();

        void _notifyGraphicsSystem(GraphicsSystem* graphicsSystem )    { mGraphicsSystem = graphicsSystem; }
        void _notifyGameEntityManager( GameEntityManager *mgr )     { mGameEntityManager = mgr; }
        virtual void _notifyWindowWidth(float width);
        virtual void _notifyWindowHeight(float height);

        void finishFrameParallel(void);

        float getWindowWidth(void) { return mWindowWidth; }
        float getWindowHeight(void) { return mWindowHeight; }

        int* getMouseX(void) { return mMouseX; }
        int* getMouseY(void) { return mMouseY; }

        GraphicsSystem* getGraphicsSystem(void)                     { return mGraphicsSystem; }
        GameEntityManager* getGameEntityManager(void)               { return mGameEntityManager; }
        Ogre::uint32 getCurrentTransformIdx(void) const             { return mCurrentTransformIdx; }
    };
}

#endif
