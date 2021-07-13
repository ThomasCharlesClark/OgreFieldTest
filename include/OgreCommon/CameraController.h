
#ifndef _Demo_CameraController_H_
#define _Demo_CameraController_H_

#include "OgrePrerequisites.h"
#include "TutorialGameState.h"
#include "OgreCamera.h"
#include "GameEntity.h"
#include "OgrePlatform.h"

namespace MyThirdOgre
{

    class CameraController
    {
        GameEntity*         mCameraEntity;
        bool                mUseSceneNode;

        bool                mSpeedMofifier;
        bool                mWASD[4];
        bool                mSlideUpDown[2];
        float               mWindowWidth;
        float               mWindowHeight;
        float               mCameraYaw;
        float               mCameraPitch;
        public: float       mCameraBaseSpeed;
        public: float       mCameraSpeedBoost;

    protected:
        Ogre::Camera*       getCamera(void) { return static_cast<Ogre::Camera*>(mCameraEntity->mMovableObject); }

    public:
        CameraController( GameEntity *cameraEntity, float mWindowWidth, float mWindowHeight, bool useSceneNode=false );

        void update( float timeSinceLast, Ogre::uint32 transformIndex);

        /// Returns true if we've handled the event
        bool keyPressed( const SDL_KeyboardEvent &arg );
        /// Returns true if we've handled the event
        bool keyReleased( const SDL_KeyboardEvent &arg );

        void mouseMoved( const SDL_Event &arg );
    };
}

#endif
