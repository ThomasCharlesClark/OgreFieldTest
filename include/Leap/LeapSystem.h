#ifndef _LEAP_SYSTEM_H
#define _LEAP_SYSTEM_H

#include "LeapC.h"
#include "OgrePrerequisites.h"
#include "LeapSystemState.h"
#include "BaseSystem.h"
#include "OgreVector3.h"

namespace MyThirdOgre
{
	struct Leap_MotionMessage
	{
		float timeSinceLast;
		Ogre::Vector3 position;
		Ogre::Vector3 velocity;
		bool addInk;

		Leap_MotionMessage(float t, Ogre::Vector3 v, Ogre::Vector3 p, bool add)
			:	timeSinceLast(t),
				velocity(v),
				position(p),
				addInk(add)
		{

		}
	};

	class LeapSystem : public BaseSystem
	{
		LEAP_CONNECTION					mConnectionHandle;

		LEAP_CONNECTION_MESSAGE			mPreviousTrackingMessage;

		bool							mRunning;
		float							mVelocityScalingFactor;
		float							mPositionScalingFactor;

	protected:
		BaseSystem						*mLogicSystem;
		BaseSystem						*mGraphicsSystem;

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