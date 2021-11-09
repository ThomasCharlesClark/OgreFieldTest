
#ifndef _Demo_GraphicsSystem_H_
#define _Demo_GraphicsSystem_H_

#include "BaseSystem.h"
#include "GameEntityManager.h"
#include "System/StaticPluginLoader.h"
#include "OgrePrerequisites.h"
#include "OgreColourValue.h"
#include "OgreOverlayPrerequisites.h"

#include "Threading/OgreUniformScalableTask.h"
#include "SdlEmulationLayer.h"
#include "OgreOverlaySystem.h"

#include "LogicSystem.h"

#if OGRE_USE_SDL2
    #include <SDL.h>
#endif

#include <GameEntityManager.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// 
#include "OgreRenderSystem.h"
#include "OgreWireAabb.h"
//
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace MyThirdOgre
{
    class SdlInputHandler;
    class FieldComputeSystem;

    class GraphicsSystem : public BaseSystem, public Ogre::UniformScalableTask
    {
    protected:

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //  
        // 
        //std::vector<std::pair<Ogre::TextureGpu*, Ogre::HlmsComputeJob*>>       mComputeTexturesJobs;
        std::vector<FieldComputeSystem*> mFieldComputeSystems;
        //
        //
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        BaseSystem* mLogicSystem;

#if OGRE_USE_SDL2
        SDL_Window* mSdlWindow;
        SdlInputHandler* mInputHandler;
#endif

        Ogre::Root* mRoot;
        Ogre::Window* mRenderWindow;
        Ogre::SceneManager* mSceneManager;
        Ogre::Camera* mCamera;
        Ogre::CompositorWorkspace*  mPrimaryWorkspace;
        std::vector<Ogre::CompositorWorkspace*> mWorkspaces;
        Ogre::String                mPluginsFolder;
        Ogre::String                mWriteAccessFolder;
        Ogre::String                mResourcePath;
        Ogre::WireAabb*             mWireAabb;
        Ogre::String                mAdditionalDebugText;

        Ogre::v1::OverlaySystem* mOverlaySystem;

        StaticPluginLoader          mStaticPluginLoader;

        /// Tracks the amount of elapsed time since we last
        /// heard from the LogicSystem finishing a frame
        float               mAccumTimeSinceLastLogicFrame;
        Ogre::uint32        mCurrentTransformIdx;
        GameEntityVec       mGameEntities[Ogre::NUM_SCENE_MEMORY_MANAGER_TYPES];
        GameEntityVec const* mThreadGameEntityToUpdate;
        float               mThreadWeight;

        bool                mQuit;
        bool                mAlwaysAskForConfig;
        bool                mUseHlmsDiskCache;
        bool                mUseMicrocodeCache;

        Ogre::ColourValue   mBackgroundColour;

#if OGRE_USE_SDL2
        void handleWindowEvent(const SDL_Event& evt);
#endif

        bool isWriteAccessFolder(const Ogre::String& folderPath, const Ogre::String& fileToSave);

        /// @see MessageQueueSystem::processIncomingMessage
        virtual void processIncomingMessage(Mq::MessageId messageId, const void* data);

        static void addResourceLocation(const Ogre::String& archName, const Ogre::String& typeName,
            const Ogre::String& secName);
        void loadTextureCache(void);
        void saveTextureCache(void);
        void loadHlmsDiskCache(void);
        void saveHlmsDiskCache(void);
        virtual void setupResources(void);
        virtual void registerHlms(void);
        /// Optional override method where you can perform resource group loading
        /// Must at least do ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
        virtual void loadResources(void);
        virtual void chooseSceneManager(void);
        virtual void createCamera(void);
        /// Virtual so that advanced samples such as Sample_Compositor can override this
        /// method to change the default behavior if setupCompositor() is overridden, be
        /// aware @mBackgroundColour will be ignored
        virtual Ogre::CompositorWorkspace* setupCompositor(void);

        //virtual Ogre::CompositorWorkspace* setupComputeTestCompositor(void);

        /// Called right before initializing Ogre's first window, so the params can be customized
        virtual void initMiscParamsListener(Ogre::NameValuePairList& params);

        /// Optional override method where you can create resource listeners (e.g. for loading screens)
        virtual void createResourceListener(void) {}

        void gameEntityAdded(const GameEntityManager::CreatedGameEntity* createdGameEntity);
        void gameEntityRemoved(GameEntity* toRemove);
    public:
        GraphicsSystem(GameState* gameState,
            Ogre::String resourcePath = Ogre::String(""),
            Ogre::ColourValue backgroundColour = Ogre::ColourValue(0.2f, 0.4f, 0.6f));
        virtual ~GraphicsSystem();

        void _notifyLogicSystem(BaseSystem* logicSystem) { mLogicSystem = logicSystem; }

        void initialize(const Ogre::String& windowTitle);
        void deinitialize(void);

        void update(float timeSinceLast);

        /** Updates the SceneNodes of all the game entities in the container,
            interpolating them according to weight, reading the transforms from
            mCurrentTransformIdx and mCurrentTransformIdx-1.
        @param gameEntities
            The container with entities to update.
        @param weight
            The interpolation weight, ideally in range [0; 1]
        */
        void updateGameEntities(const GameEntityVec& gameEntities, float weight);

        /// Overload Ogre::UniformScalableTask. @see updateGameEntities
        virtual void execute(size_t threadId, size_t numThreads);

        /// Returns the GameEntities that are ready to be rendered. May include entities
        /// that are scheduled to be removed (i.e. they are no longer updated by logic)
        const GameEntityVec& getGameEntities(Ogre::SceneMemoryMgrTypes type) const
        {
            return mGameEntities[type];
        }

#if OGRE_USE_SDL2
        SdlInputHandler* getInputHandler(void) { return mInputHandler; }
#endif

        void setQuit(void) { mQuit = true; }
        bool getQuit(void) const { return mQuit; }

        float getAccumTimeSinceLastLogicFrame(void) const { return mAccumTimeSinceLastLogicFrame; }

        Ogre::Root* getRoot(void) const { return mRoot; }
        Ogre::Window* getRenderWindow(void) const { return mRenderWindow; }
        Ogre::SceneManager* getSceneManager(void) const { return mSceneManager; }
        Ogre::Camera* getCamera(void) const { return mCamera; }
        Ogre::CompositorWorkspace* getCompositorWorkspace(void) const { return mPrimaryWorkspace; }
        Ogre::v1::OverlaySystem* getOverlaySystem(void) const { return mOverlaySystem; }

        const Ogre::String& getPluginsFolder(void) const { return mPluginsFolder; }
        const Ogre::String& getWriteAccessFolder(void) const { return mWriteAccessFolder; }
        const Ogre::String& getResourcePath(void) const { return mResourcePath; }

        virtual void stopCompositor(void);
        virtual void restartCompositor(void);

        virtual void changeGameEntityColour(const GameEntityManager::GameEntityColourChange* change);

        virtual void changeGameEntityAlpha(const GameEntityManager::GameEntityAlphaChange* change);

        virtual void changeGameEntityVisibility(const GameEntityManager::GameEntityVisibilityChange* change);

        virtual void cleanupComputeJobs(void);

        Ogre::String getAdditionalDebugText(void) { return mAdditionalDebugText; }
        virtual void setAdditionalDebugText(Ogre::String t) { mAdditionalDebugText = t; };
    };
}

#endif
