
#ifndef _Demo_MyGameState_H_
#define _Demo_MyGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"
#include "../src/FieldTest/Field.h"
#include "CameraControllerMultiThreading.h"

union SDL_Event;
struct SDL_MouseButtonEvent;
struct SDL_MouseWheelEvent;

struct SDL_TextEditingEvent;
struct SDL_TextInputEvent;
struct SDL_KeyboardEvent;

struct SDL_JoyButtonEvent;
struct SDL_JoyAxisEvent;
struct SDL_JoyHatEvent;

namespace MyThirdOgre
{
    struct GameEntity;
    struct MovableObjectDefinition;
    class LogicSystem;

    class LogicGameState : public GameState
    {

        Field               *mField;

        LogicSystem         *mLogicSystem;

        CameraControllerMultiThreading    *mCameraController;
        // Entities are destroyed by the GameEntityManager
		GameEntity          *mCameraEntity;
        // MovableObjectDefinitions are not
		MovableObjectDefinition *mCameraMoDef;

    public:
        LogicGameState();
        ~LogicGameState();

        void _notifyLogicSystem(LogicSystem* logicSystem) { mLogicSystem = logicSystem; }

        virtual void createScene01(void);
        virtual void update(float timeSinceLast);

        virtual void keyPressed(const SDL_KeyboardEvent& arg);
        virtual void keyReleased(const SDL_KeyboardEvent& arg);

        virtual void mouseMoved(const SDL_Event& arg);
        virtual void mousePressed(const SDL_MouseButtonEvent& arg, Ogre::uint8 id);
        virtual void mouseReleased(const SDL_MouseButtonEvent& arg, Ogre::uint8 id);
        virtual void mouseWheelChanged(const SDL_MouseWheelEvent& arg);

        virtual void textEditing(const SDL_TextEditingEvent& arg);
        virtual void textInput(const SDL_TextInputEvent& arg);
    };
}

#endif
