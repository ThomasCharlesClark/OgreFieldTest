#pragma enable_d3d11_debug_symbols

#include "GraphicsSystem.h"
#include "LogicSystem.h"
#include "GameEntityManager.h"
#include "GraphicsGameState.h"
#include "LogicGameState.h"
#include "SdlInputHandler.h"

#include "Threading/YieldTimer.h"

#include "OgreWindow.h"
#include "OgreTimer.h"

#include "Threading/OgreThreads.h"
#include "Threading/OgreBarrier.h"

#include "Leap/LeapSystem.h"
#include "Leap/LeapSystemState.h"

#include <iostream>

using namespace MyThirdOgre;

extern const double cFrametime;
const double cFrametime = 1.0 / 60.0;
extern const double cLogicFrametime;
const double cLogicFrametime = 1.0 / 60.0;
extern const double cLeapFrametime;
const double cLeapFrametime = 1.0 / 240.0;

extern bool gFakeFrameskip;
bool gFakeFrameskip = false;

extern bool gFakeSlowmo;
bool gFakeSlowmo = false;

unsigned long renderThread(Ogre::ThreadHandle* threadHandle);
unsigned long logicThread(Ogre::ThreadHandle* threadHandle);
unsigned long leapThread(Ogre::ThreadHandle* threadHandle);
THREAD_DECLARE(renderThread);
THREAD_DECLARE(logicThread);
THREAD_DECLARE(leapThread);

struct ThreadData
{
    GraphicsSystem* graphicsSystem;
    LogicSystem* logicSystem;
    LeapSystem* leapSystem;
    Ogre::Barrier* barrier;
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
int main()
#endif
{
    GraphicsGameState graphicsGameState(
        "This is an advanced multithreading tutorial. For the simple version see Tutorial 05\n"
        "We introduce the 'GameEntity' structure which encapsulates a game object data:\n"
        "It contains its graphics (i.e. Entity and SceneNode) and its physics/logic data\n"
        "(a transform, the hkpEntity/btRigidBody pointers, etc)\n"
        "GameEntity is created via the GameEntityManager; which is responsible for telling\n"
        "the render thread to create the graphics; and delays deleting the GameEntity until\n"
        "all threads are done using it.\n"
        "Position/Rot./Scale is updated via a ring buffer, which ensures that the Logic \n"
        "thread is never writing to the transforms while being read by the Render thread\n"
        "You could gain some performance and memory by purposedly not caring and leaving\n"
        "a race condition (Render thread reading the transforms while Logic may be \n"
        "updating it) if you don't mind a very occasional flickering.\n"
        "\n"
        "The Logic thread is in charge of simulating the transforms and ultimately, updating\n"
        "the transforms.\n"
        "The Render thread is in charge of interpolating these transforms and leaving the \n"
        "rendering to Ogre (culling, updating the scene graph, skeletal animations, rendering)\n"
        "\n"
        "Render-split multithreaded rendering is very powerful and scales well to two cores\n"
        "but it requires a different way of thinking. You don't directly create Ogre objects.\n"
        "You request them via messages that need first to bake all necesary information (do \n"
        "you want an Entity, an Item, a particle FX?), and they get created asynchronously.\n"
        "A unit may be created in a logic frame, but may still not be rendered yet, and may\n"
        "take one or two render frames to appear on screen.\n"
        "\n"
        "Skeletal animation is not covered by this tutorial, but the same principle applies.\n"
        "First define a few baked structures about the animations you want to use, and then\n"
        "send messages for synchronizing it (i.e. play X animation, jump to time Y, set blend\n"
        "weight Z, etc)");
    GraphicsSystem graphicsSystem(&graphicsGameState);
    LogicGameState logicGameState;
    LogicSystem logicSystem(&logicGameState);
    LeapSystemState leapSystemState;
    LeapSystem leapSystem(&leapSystemState);

    Ogre::Barrier barrier(3);

    graphicsGameState._notifyGraphicsSystem(&graphicsSystem);
    logicGameState._notifyLogicSystem(&logicSystem);

    graphicsSystem._notifyLogicSystem(&logicSystem);
    logicSystem._notifyGraphicsSystem(&graphicsSystem);

    leapSystemState._notifyLogicSystem(&logicSystem);
    leapSystem._notifyGraphicsSystem(&graphicsSystem);
    leapSystem._notifyLogicSystem(&logicSystem);

    GameEntityManager gameEntityManager(&graphicsSystem, &logicSystem);

    ThreadData threadData;
    threadData.graphicsSystem = &graphicsSystem;
    threadData.logicSystem = &logicSystem;
    threadData.leapSystem = &leapSystem;
    threadData.barrier = &barrier;

    Ogre::ThreadHandlePtr threadHandles[3];
    threadHandles[0] = Ogre::Threads::CreateThread(THREAD_GET(renderThread), 0, &threadData);
    threadHandles[1] = Ogre::Threads::CreateThread(THREAD_GET(logicThread), 1, &threadData);
    threadHandles[2] = Ogre::Threads::CreateThread(THREAD_GET(leapThread), 2, &threadData);

    Ogre::Threads::WaitForThreads(3, threadHandles);

    return 0;
}

//---------------------------------------------------------------------
unsigned long renderThreadApp(Ogre::ThreadHandle* threadHandle)
{
    ThreadData* threadData = reinterpret_cast<ThreadData*>(threadHandle->getUserParam());
    GraphicsSystem* graphicsSystem = threadData->graphicsSystem;
    LogicSystem* logicSystem = threadData->logicSystem;
    LeapSystem* leapSystem = threadData->leapSystem;
    Ogre::Barrier* barrier = threadData->barrier;

    graphicsSystem->initialize("Fluid Dynamics");

    barrier->sync();

    if (graphicsSystem->getQuit())
    {
        graphicsSystem->deinitialize();
        return 0; //User cancelled config
    }

    // can only notify inputHandler after it actually exists - new is called in graphicsSystem::initialise
    // it is quite possible to notify the input handler about the logic system in here, but for neatness I have 
    // left that up to the logic thread.
    graphicsSystem->getInputHandler()->_notifyGraphicsSystem(threadData->graphicsSystem);
    barrier->sync();

    graphicsSystem->createScene01();
    barrier->sync();

    graphicsSystem->createScene02();
    barrier->sync();

    Ogre::Window* renderWindow = graphicsSystem->getRenderWindow();

    Ogre::SceneManager* scnMgr = graphicsSystem->getSceneManager();

    Ogre::Timer timer;

    Ogre::uint64 startTime = timer.getMicroseconds();

    double timeSinceLast = 1.0 / 60.0;

    while (!graphicsSystem->getQuit())
    {
        graphicsSystem->beginFrameParallel();
        graphicsSystem->update(timeSinceLast);
        graphicsSystem->finishFrameParallel();

        if (!renderWindow->isVisible())
        {
            //Don't burn CPU cycles unnecessary when we're minimized.
            Ogre::Threads::Sleep(500);
        }

        if (gFakeFrameskip)
            Ogre::Threads::Sleep(120);

        Ogre::uint64 endTime = timer.getMicroseconds();
        timeSinceLast = (endTime - startTime) / 1000000.0;
        timeSinceLast = std::min(1.0, timeSinceLast); //Prevent from going haywire.
        startTime = endTime;
    }

    barrier->sync();

    graphicsSystem->destroyScene();
    barrier->sync();

    graphicsSystem->deinitialize();
    barrier->sync();

    return 0;
}
unsigned long renderThread(Ogre::ThreadHandle* threadHandle)
{
    unsigned long retVal = -1;

    try
    {
        retVal = renderThreadApp(threadHandle);
    }
    catch (Ogre::Exception & e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBoxA(NULL, e.getFullDescription().c_str(), "An exception has occured!",
            MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        std::cerr << "An exception has occured: " <<
            e.getFullDescription().c_str() << std::endl;
#endif

        abort();
    }

    return retVal;
}
//---------------------------------------------------------------------
unsigned long logicThread(Ogre::ThreadHandle* threadHandle)
{
    ThreadData* threadData = reinterpret_cast<ThreadData*>(threadHandle->getUserParam());
    GraphicsSystem* graphicsSystem = threadData->graphicsSystem;
    LogicSystem* logicSystem = threadData->logicSystem;
    LeapSystem* leapSystem = threadData->leapSystem;
    Ogre::Barrier* barrier = threadData->barrier;

    logicSystem->initialize();
    barrier->sync();

    if (graphicsSystem->getQuit())
    {
        logicSystem->deinitialize();
        return 0; //Render thread cancelled early
    }

    graphicsSystem->getInputHandler()->_notifyLogicSystem(threadData->logicSystem);
    barrier->sync();

    float width = graphicsSystem->getRenderWindow()->getWidth(), 
         height = graphicsSystem->getRenderWindow()->getHeight();

    logicSystem->_notifyWindowWidth(width);
    logicSystem->_notifyWindowHeight(height);

    logicSystem->createScene01();
    barrier->sync();

    logicSystem->createScene02();
    barrier->sync();

    Ogre::Window* renderWindow = graphicsSystem->getRenderWindow();

    Ogre::Timer timer;
    YieldTimer yieldTimer(&timer);

    Ogre::uint64 startTime = timer.getMicroseconds();

    while (!graphicsSystem->getQuit())
    {
        logicSystem->beginFrameParallel();
        
        SDL_GetRelativeMouseState(logicSystem->getMouseX(), logicSystem->getMouseY());
        //SDL_GetMouseState(logicSystem->getMouseX(), logicSystem->getMouseY());

        logicSystem->update(static_cast<float>(cLogicFrametime));

        logicSystem->finishFrameParallel();

        logicSystem->finishFrame();

        if (gFakeSlowmo)
            Ogre::Threads::Sleep(120);

        if (!renderWindow->isVisible())
        {
            //Don't burn CPU cycles unnecessary when we're minimized.
            Ogre::Threads::Sleep(500);
        }

        //YieldTimer will wait until the current time is greater than startTime + cFrametime
        startTime = yieldTimer.yield(cLogicFrametime, startTime);
    }

    barrier->sync();

    logicSystem->destroyScene();
    barrier->sync();

    logicSystem->deinitialize();
    barrier->sync();

    return 0;
}



//---------------------------------------------------------------------
unsigned long leapThread(Ogre::ThreadHandle* threadHandle)
{
    ThreadData* threadData = reinterpret_cast<ThreadData*>(threadHandle->getUserParam());
    GraphicsSystem* graphicsSystem = threadData->graphicsSystem;
    LogicSystem* logicSystem = threadData->logicSystem;
    LeapSystem* leapSystem = threadData->leapSystem;
    Ogre::Barrier* barrier = threadData->barrier;

    leapSystem->initialize();

    barrier->sync();

    if (graphicsSystem->getQuit())
    {
        leapSystem->deinitialize();
        return 0; //Render thread cancelled early
    }

    barrier->sync();

    Ogre::Window* renderWindow = graphicsSystem->getRenderWindow();

    Ogre::Timer timer;
    YieldTimer yieldTimer(&timer);

    Ogre::uint64 startTime = timer.getMicroseconds();

    barrier->sync();

    leapSystem->createScene01();

    barrier->sync();

    while (!graphicsSystem->getQuit())
    {
        leapSystem->beginFrameParallel();

        leapSystem->pollConnection(static_cast<float>(cLeapFrametime));

        leapSystem->finishFrameParallel();

        leapSystem->finishFrame();
        
        if (gFakeSlowmo)
            Ogre::Threads::Sleep(120);

        if (!renderWindow->isVisible())
        {
            //Don't burn CPU cycles unnecessary when we're minimized.
            Ogre::Threads::Sleep(500);
        }

        //YieldTimer will wait until the current time is greater than startTime + cLeapFrametime
        startTime = yieldTimer.yield(cLeapFrametime, startTime);
    }

    leapSystem->destroyConnection();

    barrier->sync();
    leapSystem->destroyScene();

    barrier->sync();
    leapSystem->deinitialize();

    barrier->sync();

    return 0;
}
