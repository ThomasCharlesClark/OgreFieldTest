
#include "TutorialGameState.h"
#include "CameraController.h"
#include "GraphicsSystem.h"

#include "OgreSceneManager.h"

#include "OgreOverlayManager.h"
#include "OgreOverlay.h"
#include "OgreOverlayContainer.h"
#include "OgreTextAreaOverlayElement.h"

#include "OgreRoot.h"
#include "OgreFrameStats.h"

#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsCompute.h"
#include "OgreGpuProgramManager.h"

#include "OgreCamera.h"

using namespace MyThirdOgre;

namespace MyThirdOgre
{
    TutorialGameState::TutorialGameState( const Ogre::String &helpDescription ) :
        mGraphicsSystem( 0 ),
        mCameraController( 0 ),
        mHelpDescription( helpDescription ),
        mDisplayHelpMode( 1 ),
        mNumDisplayHelpModes( 2 ),
        mDebugText( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    TutorialGameState::~TutorialGameState()
    {
        if (mCameraController) {
            delete mCameraController;
            mCameraController = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::_notifyGraphicsSystem( GraphicsSystem *graphicsSystem )
    {
        mGraphicsSystem = graphicsSystem;
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::createScene01(void)
    {

//#if OGRE_DEBUG_MODE
        //createDebugTextOverlay();
//#endif

        Ogre::Light* light = mGraphicsSystem->getSceneManager()->createLight();
        Ogre::SceneNode* lightNode = mGraphicsSystem->getSceneManager()->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject(light);
        light->setPowerScale(Ogre::Math::PI * 2); //Since we don't do HDR, counter the PBS' division by PI
        light->setType(Ogre::Light::LT_DIRECTIONAL);
        light->setDirection(Ogre::Vector3(-1, -1, -1).normalisedCopy());

        //Ogre::Light* light2 = mGraphicsSystem->getSceneManager()->createLight();
        //Ogre::SceneNode* lightNode2 = mGraphicsSystem->getSceneManager()->getRootSceneNode()->createChildSceneNode();
        //lightNode2->attachObject(light2);
        //lightNode2->setPosition(0, 10, 0);
        //light2->setPowerScale(10.0f); //Since we don't do HDR, counter the PBS' division by PI
        //light2->setType(Ogre::Light::LT_POINT);

        //mCameraController = new CameraController(mGraphicsSystem, false);
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::createDebugTextOverlay(void)
    {
        Ogre::v1::OverlayManager &overlayManager = Ogre::v1::OverlayManager::getSingleton();
        Ogre::v1::Overlay *overlay = overlayManager.create( "DebugText" );

        Ogre::v1::OverlayContainer *panel = static_cast<Ogre::v1::OverlayContainer*>(
            overlayManager.createOverlayElement("Panel", "DebugPanel"));
        mDebugText = static_cast<Ogre::v1::TextAreaOverlayElement*>(
                    overlayManager.createOverlayElement( "TextArea", "DebugText" ) );
        mDebugText->setFontName( "DebugFont" );
        mDebugText->setCharHeight( 0.025f );

        mDebugTextShadow= static_cast<Ogre::v1::TextAreaOverlayElement*>(
                    overlayManager.createOverlayElement( "TextArea", "0DebugTextShadow" ) );
        mDebugTextShadow->setFontName( "DebugFont" );
        mDebugTextShadow->setCharHeight( 0.025f );
        mDebugTextShadow->setColour( Ogre::ColourValue::Black );
        mDebugTextShadow->setPosition( 0.002f, 0.002f );

        panel->addChild( mDebugTextShadow );
        panel->addChild( mDebugText );
        overlay->add2D( panel );
        overlay->show();
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::generateDebugText( float timeSinceLast, Ogre::String &outText )
    {
        if( mDisplayHelpMode == 0 )
        {
            outText = mHelpDescription;
            outText += "\n\nPress F1 to toggle help";
            outText += "\n\nProtip: Ctrl+F1 will reload PBS shaders (for real time template editing).\n"
                "Ctrl+F2 reloads Unlit shaders.\n"
                "Ctrl+F3 reloads Compute shaders.\n"
                "Note: If the modified templates produce invalid shader code, "
                "crashes or exceptions can happen.";
            return;
        }

        const Ogre::FrameStats *frameStats = mGraphicsSystem->getRoot()->getFrameStats();

        Ogre::String finalText;
        finalText.reserve( 128 );
        finalText  = "Frame time:\t";
        finalText += Ogre::StringConverter::toString( timeSinceLast * 1000.0f );
        finalText += " ms\n";
        finalText += "Frame FPS:\t";
        finalText += Ogre::StringConverter::toString( 1.0f / timeSinceLast );
        finalText += "\nAvg time:\t";
        finalText += Ogre::StringConverter::toString( frameStats->getAvgTime() );
        finalText += " ms\n";
        finalText += "Avg FPS:\t";
        finalText += Ogre::StringConverter::toString( 1000.0f / frameStats->getAvgTime() );
        finalText += "\nFPS:\t";
        finalText += Ogre::StringConverter::toString(frameStats->getFps());
        finalText += "\n\nPress F1 to toggle help";

        outText.swap( finalText );

        mDebugText->setCaption(finalText);
        mDebugTextShadow->setCaption(finalText);
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::update( float timeSinceLast )
    {
        if( mDisplayHelpMode != 0 )
        {
            //Show FPS
            if (mDebugText) {
                Ogre::String finalText;
                generateDebugText(timeSinceLast, finalText);
                mDebugText->setCaption(finalText);
                mDebugTextShadow->setCaption(finalText);
            }
        }

        if( mCameraController )
            mCameraController->update( timeSinceLast );
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::keyPressed( const SDL_KeyboardEvent &arg )
    {
        bool handledEvent = false;

        if( mCameraController )
            handledEvent = mCameraController->keyPressed( arg );

        if( !handledEvent )
            GameState::keyPressed( arg );
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::keyReleased( const SDL_KeyboardEvent &arg )
    {
        if( arg.keysym.scancode == SDL_SCANCODE_F1 && (arg.keysym.mod & ~(KMOD_NUM|KMOD_CAPS)) == 0 )
        {
            mDisplayHelpMode = (mDisplayHelpMode + 1) % mNumDisplayHelpModes;
            if (mDebugText) {
                Ogre::String finalText;
                generateDebugText(0, finalText);
                mDebugText->setCaption(finalText);
                mDebugTextShadow->setCaption(finalText);
            }
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_F1 && (arg.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
        {
            //Hot reload of PBS shaders. We need to clear the microcode cache
            //to prevent using old compiled versions.
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_PBS );
            Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
            hlms->reloadFrom( hlms->getDataFolder() );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_F2  && (arg.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
        {
            //Hot reload of Unlit shaders.
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

            Ogre::Hlms *hlms = hlmsManager->getHlms( Ogre::HLMS_UNLIT );
            Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
            hlms->reloadFrom( hlms->getDataFolder() );
        }
        else if( arg.keysym.scancode == SDL_SCANCODE_F3 && (arg.keysym.mod & (KMOD_LCTRL|KMOD_RCTRL)) )
        {
            //Hot reload of Compute shaders.
            Ogre::Root *root = mGraphicsSystem->getRoot();
            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();

            Ogre::Hlms *hlms = hlmsManager->getComputeHlms();
            Ogre::GpuProgramManager::getSingleton().clearMicrocodeCache();
            hlms->reloadFrom( hlms->getDataFolder() );
        }
        else
        {
            bool handledEvent = false;

            if( mCameraController )
                handledEvent = mCameraController->keyReleased( arg );

            if( !handledEvent )
                GameState::keyReleased( arg );
        }
    }
    //-----------------------------------------------------------------------------------
    void TutorialGameState::mouseMoved( const SDL_Event &arg )
    {
       if( mCameraController )
            mCameraController->mouseMoved( arg );

        GameState::mouseMoved( arg );
    }
}
