
#ifndef _Demo_CameraController_H_
#define _Demo_CameraController_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"
#include "GameEntity.h"
#include "OgrePlatform.h"
#include "OgreQuaternion.h"

namespace MyThirdOgre
{

    class CameraControllerMultiThreading
    {
        GameEntity* mCameraEntity;
        /*bool                mMouseMovedRecently;
        float               mRecentMouseMoveTimeAccumulator;*/

        bool                mSpeedModifier;
        bool                mWASD[4];
        bool                mSlideUpDown[2];
        float               mWindowWidth;
        float               mWindowHeight;
        float               mCameraYaw;
        float               mCameraPitch;
        float               mCameraEpsilon;

        Ogre::Quaternion    mYaw;
        Ogre::Quaternion    mPitch;

    public: float       mCameraBaseSpeed;
    public: float       mCameraBaseAngularRotationSpeed;
    public: float       mCameraSpeedBoost;

    public:
        CameraControllerMultiThreading(GameEntity* cameraEntity, float mWindowWidth, float mWindowHeight);

        void update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex, int* mouseX, int* mouseY);

        /// Returns true if we've handled the event
        bool keyPressed(const SDL_KeyboardEvent& arg);
        /// Returns true if we've handled the event
        bool keyReleased(const SDL_KeyboardEvent& arg);
        /// Whoosh!
        void mouseMoved(const SDL_Event& arg);
    };
}

#endif
