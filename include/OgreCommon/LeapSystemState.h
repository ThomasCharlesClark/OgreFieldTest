#ifndef _LEAP_SYSTEM_STATE_H
#define _LEAP_SYSTEM_STATE_H

#include "LeapSystem.h"
#include "GameState.h"
#include "OgrePrerequisites.h"
#include "TutorialGameState.h"
#include "OgreVector3.h"
#include "OgreQuaternion.h"

namespace MyThirdOgre
{
	struct GameEntity;
	struct MovableObjectDefinition;
	class LogicSystem;

	class LeapSystemState : public GameState
	{
		LogicSystem* mLogicSystem;

    public:
        LeapSystemState();
        ~LeapSystemState();

		void _notifyLogicSystem(LogicSystem* logicSystem) { mLogicSystem = logicSystem; };
	};
}

#endif // !_LEAP_SYSTEM_STATE_H