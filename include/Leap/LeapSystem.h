#ifndef _LEAP_SYSTEM_H
#define _LEAP_SYSTEM_H

#include "LeapC.h"
#include "OgrePrerequisites.h"
#include "LeapSystemState.h"
#include "BaseSystem.h"
#include "OgreVector3.h"

namespace MyThirdOgre
{
	struct Leap_VelocityMessage
	{
		float timeSinceLast;
		Ogre::Vector3 velocity;

		Leap_VelocityMessage(float t, Ogre::Vector3 v) : timeSinceLast(t), velocity(v)
		{

		}
	};

	class LeapSystem : public BaseSystem
	{
		LEAP_CONNECTION mConnectionHandle;

		LEAP_CONNECTION_MESSAGE mPreviousTrackingMessage;

		bool mRunning;
		float mVelocityScalingFactor;

	protected:
		BaseSystem* mLogicSystem;
		BaseSystem* mGraphicsSystem;

		virtual void createLeapConnection(void);
		virtual void destroyLeapConnection(void);

		/// @see MessageQueueSystem::processIncomingMessage
		virtual void processIncomingMessage(Mq::MessageId messageId, const void* data);

	public:
		LeapSystem(GameState* gameState);
		virtual ~LeapSystem();

		void finishFrameParallel(void);

		void initialize(void);
		void deinitialize(void);

		bool isRunning(void) { return mRunning; }

		void _notifyGraphicsSystem(BaseSystem* graphicsSystem) { mGraphicsSystem = graphicsSystem; }
		void _notifyLogicSystem(BaseSystem* logicSystem) { mLogicSystem = logicSystem; };

		virtual void pollConnection(float timeSinceLast);
	};
}

#endif // !_LEAP_SYSTEM_H