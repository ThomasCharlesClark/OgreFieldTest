
#include "CameraControllerMultiThreading.h"
#include "GraphicsSystem.h"
#include "OgreCamera.h"
#include "OgreEntity.h"
#include "OgreWindow.h"

namespace MyThirdOgre
{
    CameraControllerMultiThreading::CameraControllerMultiThreading(GameEntity* cameraEntity, float windowWidth, float windowHeight) :
        mSpeedModifier(false),
        mCameraYaw(0),
        mCameraPitch(0),
        mCameraBaseSpeed(10),
        mCameraBaseAngularRotationSpeed(0.5f),
        mCameraSpeedBoost(5),
        mCameraEntity(cameraEntity),
        mWindowWidth(windowWidth),
        mWindowHeight(windowHeight),
        mYaw(Ogre::Quaternion::IDENTITY),
        mPitch(Ogre::Quaternion::IDENTITY)
    {
        memset(mWASD, 0, sizeof(mWASD));
        memset(mSlideUpDown, 0, sizeof(mSlideUpDown));
    }

    //-----------------------------------------------------------------------------------
    void CameraControllerMultiThreading::update(float timeSinceLast, Ogre::uint32 currentTransformIndex, Ogre::uint32 previousTransformIndex, int* mouseX, int* mouseY)
    {
        mCameraYaw += -*mouseX / mWindowWidth;
        mCameraPitch += -*mouseY / mWindowHeight;

        Ogre::Quaternion qPrev = mCameraEntity->mTransform[previousTransformIndex]->qRot;

        int camMovementZ = mWASD[2] - mWASD[0];
        int camMovementX = mWASD[3] - mWASD[1];

        int slideUpDown = mSlideUpDown[0] - mSlideUpDown[1];

        Ogre::Vector3 camMovementDir(camMovementX, slideUpDown, camMovementZ);

        camMovementDir = qPrev * camMovementDir;

        camMovementDir.normalise();

        camMovementDir *= timeSinceLast * mCameraBaseSpeed * (1 + mSpeedModifier * mCameraSpeedBoost);

        mCameraEntity->mTransform[currentTransformIndex]->vPos =
            mCameraEntity->mTransform[previousTransformIndex]->vPos + camMovementDir;       
         
        if (mCameraEntity->mSceneNode) {
            Ogre::Vector3 mRight = qPrev * Ogre::Vector3(1, 0, 0);

            mPitch.FromAngleAxis(Ogre::Radian(mCameraPitch * mCameraBaseAngularRotationSpeed), mRight);
            mPitch.normalise();

            mYaw.FromAngleAxis(Ogre::Radian(mCameraYaw * mCameraBaseAngularRotationSpeed), Ogre::Vector3::UNIT_Y);
            mYaw.normalise();
        }

        qPrev = mYaw * mPitch * qPrev;

        qPrev.normalise();

        // There's no need to interpolate here - that is handled by GraphicsSystem's update.
        mCameraEntity->mTransform[currentTransformIndex]->qRot = qPrev;

        mCameraYaw = 0;

        mCameraPitch = 0;
    }
    //-----------------------------------------------------------------------------------
    bool CameraControllerMultiThreading::keyPressed(const SDL_KeyboardEvent& arg)
    {
        if (arg.keysym.scancode == SDL_SCANCODE_LSHIFT)
            mSpeedModifier = true;

        if (arg.keysym.scancode == SDL_SCANCODE_W)
            mWASD[0] = true;
        else if (arg.keysym.scancode == SDL_SCANCODE_A)
            mWASD[1] = true;
        else if (arg.keysym.scancode == SDL_SCANCODE_S)
            mWASD[2] = true;
        else if (arg.keysym.scancode == SDL_SCANCODE_D)
            mWASD[3] = true;
        else if (arg.keysym.scancode == SDL_SCANCODE_PAGEUP)
            mSlideUpDown[0] = true;
        else if (arg.keysym.scancode == SDL_SCANCODE_PAGEDOWN)
            mSlideUpDown[1] = true;
        else
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    bool CameraControllerMultiThreading::keyReleased(const SDL_KeyboardEvent& arg)
    {
        if (arg.keysym.scancode == SDL_SCANCODE_LSHIFT)
            mSpeedModifier = false;

        if (arg.keysym.scancode == SDL_SCANCODE_W)
            mWASD[0] = false;
        else if (arg.keysym.scancode == SDL_SCANCODE_A)
            mWASD[1] = false;
        else if (arg.keysym.scancode == SDL_SCANCODE_S)
            mWASD[2] = false;
        else if (arg.keysym.scancode == SDL_SCANCODE_D)
            mWASD[3] = false;
        else if (arg.keysym.scancode == SDL_SCANCODE_PAGEUP)
            mSlideUpDown[0] = false;
        else if (arg.keysym.scancode == SDL_SCANCODE_PAGEDOWN)
            mSlideUpDown[1] = false;
        else
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    void CameraControllerMultiThreading::mouseMoved(const SDL_Event& arg)
    {
       /* mCameraYaw += -arg.motion.xrel / mWindowWidth;
        mCameraPitch += -arg.motion.yrel / mWindowHeight;*/
    }
}
