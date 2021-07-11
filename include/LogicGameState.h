
#ifndef _Demo_MyGameState_H_
#define _Demo_MyGameState_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"
#include "../src/FieldTest/Field.h"

namespace MyThirdOgre
{
    class LogicSystem;
    struct GameEntity;
    struct MovableObjectDefinition;

    class LogicGameState : public GameState
    {
        float               mDisplacement;
        float               mRotationRadians;

        Field* mField;

        LogicSystem* mLogicSystem;

    public:
        LogicGameState();
        ~LogicGameState();

        void _notifyLogicSystem(LogicSystem* logicSystem) { mLogicSystem = logicSystem; }

        virtual void createScene01(void);
        virtual void update(float timeSinceLast);
    };
}

#endif
