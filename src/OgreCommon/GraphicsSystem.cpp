
#include "GraphicsSystem.h"
#include "GameState.h"
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
    #include "SdlInputHandler.h"
#endif
#include "GameEntity.h"

#include "OgreRoot.h"
#include "OgreException.h"
#include "OgreConfigFile.h"

#include "OgreCamera.h"
#include "OgreItem.h"

#include "OgreHlmsUnlit.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreArchiveManager.h"

#include "Compositor/OgreCompositorManager2.h"

#include "OgreOverlaySystem.h"
#include "OgreOverlayManager.h"

#include "OgreTextureGpuManager.h"

#include "OgreWindowEventUtilities.h"
#include "OgreWindow.h"

#include "OgreFileSystemLayer.h"

#include "OgreHlmsDiskCache.h"
#include "OgreGpuProgramManager.h"

#include "OgreLogManager.h"

#include "OgreEntity.h"

#include "OgreHlmsDatablock.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreSceneManager.h"

#include "LogicSystem.h"
#include "FieldComputeSystem.h"

#include "Vao/OgreUavBufferPacked.h"

#include <fstream>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//

//#include "OgreMaterialManager.h"
//#include "OgreMaterial.h"
//#include "OgreTechnique.h"
//#include "OgrePass.h"
//#include "Vao/OgreVaoManager.h"
//#include "OgreDescriptorSetUav.h"
//#include "OgreDescriptorSetTexture.h"
//
//#include "Vao/OgreReadOnlyBufferPacked.h"
//
//#include "OgreTextureGpu.h"
//#include "OgreTextureGpuManager.h"
//
//#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"

#include "OgreRenderSystem.h"
#include "OgreWireAabb.h"

//
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if OGRE_USE_SDL2
    #include <SDL_syswm.h>
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    #include "OSX/macUtils.h"
    #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        #include "System/iOS/iOSUtils.h"
    #else
        #include "System/OSX/OSXUtils.h"
    #endif
#endif

namespace MyThirdOgre
{
    GraphicsSystem::GraphicsSystem( GameState *gameState,
                                    Ogre::String resourcePath ,
                                    Ogre::ColourValue backgroundColour ) :
        BaseSystem( gameState ),
        mLogicSystem( 0 ),
    #if OGRE_USE_SDL2
        mSdlWindow( 0 ),
        mInputHandler( 0 ),
    #endif
        mRoot( 0 ),
        mRenderWindow( 0 ),
        mSceneManager( 0 ),
        mCamera( 0 ),
        mPrimaryWorkspace( 0 ),
        mWorkspaces( {} ),
        mPluginsFolder( "./" ),
        mResourcePath( resourcePath ),
        mOverlaySystem( 0 ),
        mAccumTimeSinceLastLogicFrame( 0 ),
        mCurrentTransformIdx( 0 ),
        mThreadGameEntityToUpdate( 0 ),
        mThreadWeight( 0 ),
        mQuit( false ),
        mAlwaysAskForConfig( false ),
        mUseHlmsDiskCache( true ),
        mUseMicrocodeCache( true ),
        mBackgroundColour( backgroundColour ),
        mFieldComputeSystems({}),
        mWireAabb(0),
        mAdditionalDebugText("")
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        // Note:  macBundlePath works for iOS too. It's misnamed.
        mResourcePath = Ogre::macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        mResourcePath = Ogre::macBundlePath() + "/";
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        mPluginsFolder = mResourcePath;
#endif
        if( isWriteAccessFolder( mPluginsFolder, "Ogre.log" ) )
            mWriteAccessFolder = mPluginsFolder;
        else
        {
            Ogre::FileSystemLayer filesystemLayer( OGRE_VERSION_NAME );
            mWriteAccessFolder = filesystemLayer.getWritablePath( "" );
        }
    }
    //-----------------------------------------------------------------------------------
    GraphicsSystem::~GraphicsSystem()
    {
        if( mRoot )
        {
            Ogre::LogManager::getSingleton().logMessage(
                        "WARNING: GraphicsSystem::deinitialize() not called!!!", Ogre::LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------------------
    bool GraphicsSystem::isWriteAccessFolder( const Ogre::String &folderPath,
                                              const Ogre::String &fileToSave )
    {
        if( !Ogre::FileSystemLayer::createDirectory( folderPath ) )
            return false;

        std::ofstream of( (folderPath + fileToSave).c_str(),
                          std::ios::out | std::ios::binary | std::ios::app );
        if( !of )
            return false;

        return true;
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::initialize( const Ogre::String &windowTitle )
    {
    #if OGRE_USE_SDL2
        //if( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
        if( SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK |
                      SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS ) != 0 )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INTERNAL_ERROR, "Cannot initialize SDL2!",
                         "GraphicsSystem::initialize" );
        }
    #endif

        Ogre::String pluginsPath;
        // only use plugins.cfg if not static
    #ifndef OGRE_STATIC_LIB
    #if OGRE_DEBUG_MODE && !((OGRE_PLATFORM == OGRE_PLATFORM_APPLE) || (OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS))
        pluginsPath = mPluginsFolder + "plugins_d.cfg";
    #else
        pluginsPath = mPluginsFolder + "plugins.cfg";
    #endif
    #endif

        mRoot = OGRE_NEW Ogre::Root( pluginsPath,
                                     mWriteAccessFolder + "ogre.cfg",
                                     mWriteAccessFolder + "Ogre.log" );

        mStaticPluginLoader.install( mRoot );

        // enable sRGB Gamma Conversion mode by default for all renderers,
        // but still allow to override it via config dialog
        Ogre::RenderSystemList::const_iterator itor = mRoot->getAvailableRenderers().begin();
        Ogre::RenderSystemList::const_iterator endt = mRoot->getAvailableRenderers().end();

        while( itor != endt )
        {
            Ogre::RenderSystem *rs = *itor;
            rs->setConfigOption( "sRGB Gamma Conversion", "Yes" );
            ++itor;
        }

        if( mAlwaysAskForConfig || !mRoot->restoreConfig() )
        {
            if( !mRoot->showConfigDialog() )
            {
                mQuit = true;
                return;
            }
        }

    #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        if(!mRoot->getRenderSystem())
        {
            Ogre::RenderSystem *renderSystem =
                    mRoot->getRenderSystemByName( "Metal Rendering Subsystem" );
            mRoot->setRenderSystem( renderSystem );
        }
    #endif

        mRoot->initialise( false, windowTitle );

        Ogre::ConfigOptionMap& cfgOpts = mRoot->getRenderSystem()->getConfigOptions();

        int width   = 1280;
        int height  = 720;

    #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        {
            Ogre::Vector2 screenRes = iOSUtils::getScreenResolutionInPoints();
            width = static_cast<int>( screenRes.x );
            height = static_cast<int>( screenRes.y );
        }
    #endif

        Ogre::ConfigOptionMap::iterator opt = cfgOpts.find( "Video Mode" );
        if( opt != cfgOpts.end() )
        {
            //Ignore leading space
            const Ogre::String::size_type start = opt->second.currentValue.find_first_of("012356789");
            //Get the width and height
            Ogre::String::size_type widthEnd = opt->second.currentValue.find(' ', start);
            // we know that the height starts 3 characters after the width and goes until the next space
            Ogre::String::size_type heightEnd = opt->second.currentValue.find(' ', widthEnd+3);
            // Now we can parse out the values
            width   = Ogre::StringConverter::parseInt( opt->second.currentValue.substr( 0, widthEnd ) );
            height  = Ogre::StringConverter::parseInt( opt->second.currentValue.substr(
                                                           widthEnd+3, heightEnd ) );
        }

        Ogre::NameValuePairList params;
        bool fullscreen = Ogre::StringConverter::parseBool( cfgOpts["Full Screen"].currentValue );
    #if OGRE_USE_SDL2
        int screen = 0;
        int posX = SDL_WINDOWPOS_CENTERED_DISPLAY(screen);
        int posY = SDL_WINDOWPOS_CENTERED_DISPLAY(screen);

        if(fullscreen)
        {
            posX = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
            posY = SDL_WINDOWPOS_UNDEFINED_DISPLAY(screen);
        }

        mSdlWindow = SDL_CreateWindow(
                    windowTitle.c_str(),    // window title
                    posX,               // initial x position
                    posY,               // initial y position
                    width,              // width, in pixels
                    height,             // height, in pixels
                    SDL_WINDOW_SHOWN
                      | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_RESIZABLE );

        //Get the native whnd
        SDL_SysWMinfo wmInfo;
        SDL_VERSION( &wmInfo.version );

        if( SDL_GetWindowWMInfo( mSdlWindow, &wmInfo ) == SDL_FALSE )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_INTERNAL_ERROR,
                         "Couldn't get WM Info! (SDL2)",
                         "GraphicsSystem::initialize" );
        }

        Ogre::String winHandle;
        switch( wmInfo.subsystem )
        {
        #if defined(SDL_VIDEO_DRIVER_WINDOWS)
        case SDL_SYSWM_WINDOWS:
            // Windows code
            winHandle = Ogre::StringConverter::toString( (uintptr_t)wmInfo.info.win.window );
            break;
        #endif
        #if defined(SDL_VIDEO_DRIVER_WINRT)
        case SDL_SYSWM_WINRT:
            // Windows code
            winHandle = Ogre::StringConverter::toString( (uintptr_t)wmInfo.info.winrt.window );
            break;
        #endif
        #if defined(SDL_VIDEO_DRIVER_COCOA)
        case SDL_SYSWM_COCOA:
            winHandle  = Ogre::StringConverter::toString(WindowContentViewHandle(wmInfo));
            break;
        #endif
        #if defined(SDL_VIDEO_DRIVER_X11)
        case SDL_SYSWM_X11:
            winHandle = Ogre::StringConverter::toString( (uintptr_t)wmInfo.info.x11.window );
            break;
        #endif
        default:
            OGRE_EXCEPT( Ogre::Exception::ERR_NOT_IMPLEMENTED,
                         "Unexpected WM! (SDL2)",
                         "GraphicsSystem::initialize" );
            break;
        }

        #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
            params.insert( std::make_pair("externalWindowHandle",  winHandle) );
        #else
            params.insert( std::make_pair("parentWindowHandle",  winHandle) );
        #endif
    #endif

        params.insert( std::make_pair("title", windowTitle) );
        params.insert( std::make_pair("gamma", cfgOpts["sRGB Gamma Conversion"].currentValue) );
        params.insert( std::make_pair("FSAA", cfgOpts["FSAA"].currentValue) );
        params.insert( std::make_pair("vsync", cfgOpts["VSync"].currentValue) );
        params.insert( std::make_pair("reverse_depth", "Yes" ) );

        initMiscParamsListener( params );

        mRenderWindow = Ogre::Root::getSingleton().createRenderWindow( windowTitle, width, height,
                                                                       fullscreen, &params );

        mOverlaySystem = OGRE_NEW Ogre::v1::OverlaySystem();

        setupResources();
        loadResources();
        chooseSceneManager();
        createCamera();
        mPrimaryWorkspace = setupCompositor();
        mWorkspaces.push_back(mPrimaryWorkspace);
        mWireAabb = mSceneManager->createWireAabb();

        //if (mUseCompute) {
        //    mWorkspaces.push_back(setupComputeTestCompositor());
        //}

    #if OGRE_USE_SDL2
        mInputHandler = new SdlInputHandler( mSdlWindow, mCurrentGameState,
                                             mCurrentGameState, mCurrentGameState, mCurrentGameState );
    #endif

        BaseSystem::initialize();

#if OGRE_PROFILING
        Ogre::Profiler::getSingleton().setEnabled( true );
    #if OGRE_PROFILING == OGRE_PROFILING_INTERNAL
        Ogre::Profiler::getSingleton().endProfile( "" );
    #endif
    #if OGRE_PROFILING == OGRE_PROFILING_INTERNAL_OFFLINE
        Ogre::Profiler::getSingleton().getOfflineProfiler().setDumpPathsOnShutdown(
                    mWriteAccessFolder + "ProfilePerFrame",
                    mWriteAccessFolder + "ProfileAccum" );
    #endif
#endif






    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::cleanupComputeJobs(void)
    {
        for (auto fieldIter = mFieldComputeSystems.begin(); fieldIter < mFieldComputeSystems.end(); fieldIter++)
        {
            (*fieldIter)->deinitialise();
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::deinitialize(void)
    {
        cleanupComputeJobs();

        if (mWireAabb) {
            mSceneManager->destroyWireAabb(mWireAabb);
            mWireAabb = 0;
        }

        BaseSystem::deinitialize();

        saveTextureCache();
        saveHlmsDiskCache();

        if( mSceneManager )
            mSceneManager->removeRenderQueueListener( mOverlaySystem );

        OGRE_DELETE mOverlaySystem;
        mOverlaySystem = 0;

    #if OGRE_USE_SDL2
        delete mInputHandler;
        mInputHandler = 0;
    #endif

        OGRE_DELETE mRoot;
        mRoot = 0;

    #if OGRE_USE_SDL2
        if( mSdlWindow )
        {
            // Restore desktop resolution on exit
            SDL_SetWindowFullscreen( mSdlWindow, 0 );
            SDL_DestroyWindow( mSdlWindow );
            mSdlWindow = 0;
        }

        SDL_Quit();
    #endif


    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::update( float timeSinceLast )
    {
        for (auto& iter : mFieldComputeSystems) {
            iter->update(timeSinceLast);
        }

        Ogre::WindowEventUtilities::messagePump();

    #if OGRE_USE_SDL2
        SDL_Event evt;
        while( SDL_PollEvent( &evt ) )
        {
            switch( evt.type )
            {
            case SDL_WINDOWEVENT:
                handleWindowEvent( evt );
                break;
            case SDL_QUIT:
                mQuit = true;
                break;
            default:
                break;
            }
            mInputHandler->_handleSdlEvents( evt );
        }
    #endif

        BaseSystem::update( timeSinceLast );

        if( mRenderWindow->isVisible() )
            mQuit |= !mRoot->renderOneFrame();

        mAccumTimeSinceLastLogicFrame += timeSinceLast;

        //SDL_SetWindowPosition( mSdlWindow, 0, 0 );
        /*SDL_Rect rect;
        SDL_GetDisplayBounds( 0, &rect );
        SDL_GetDisplayBounds( 0, &rect );*/
    }
    //-----------------------------------------------------------------------------------
    #if OGRE_USE_SDL2
    void GraphicsSystem::handleWindowEvent( const SDL_Event& evt )
    {
        switch( evt.window.event )
        {
            /*case SDL_WINDOWEVENT_MAXIMIZED:
                SDL_SetWindowBordered( mSdlWindow, SDL_FALSE );
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                SDL_SetWindowBordered( mSdlWindow, SDL_TRUE );
                break;*/
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                int w,h;
                SDL_GetWindowSize( mSdlWindow, &w, &h );
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
                mRenderWindow->requestResolution( w, h );
#endif
                mRenderWindow->windowMovedOrResized();
                break;
            case SDL_WINDOWEVENT_RESIZED:
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
                mRenderWindow->requestResolution( evt.window.data1, evt.window.data2 );
#endif
                mRenderWindow->windowMovedOrResized();
                break;
            case SDL_WINDOWEVENT_CLOSE:
                break;
        case SDL_WINDOWEVENT_SHOWN:
            mRenderWindow->_setVisible( true );
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            mRenderWindow->_setVisible( false );
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            mRenderWindow->setFocused( true );
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            mRenderWindow->setFocused( false );
            break;
        }
    }
    #endif
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::processIncomingMessage( Mq::MessageId messageId, const void *data )
    {
        switch( messageId )
        {
        case Mq::LOGICFRAME_FINISHED:
            {
                Ogre::uint32 newIdx = *reinterpret_cast<const Ogre::uint32*>( data );

                if( newIdx != std::numeric_limits<Ogre::uint32>::max() )
                {
                    mAccumTimeSinceLastLogicFrame = 0;
                    //Tell the LogicSystem we're no longer using the index previous to the current one.
                    this->queueSendMessage( mLogicSystem, Mq::LOGICFRAME_FINISHED,
                                            (mCurrentTransformIdx + NUM_GAME_ENTITY_BUFFERS - 1) %
                                            NUM_GAME_ENTITY_BUFFERS );

                    assert( (mCurrentTransformIdx + 1) % NUM_GAME_ENTITY_BUFFERS == newIdx &&
                            "Graphics is receiving indices out of order!!!" );

                    //Get the new index the LogicSystem is telling us to use.
                    mCurrentTransformIdx = newIdx;
                }
            }
            break;
        case Mq::GAME_ENTITY_ADDED:
            gameEntityAdded( reinterpret_cast<const GameEntityManager::CreatedGameEntity*>( data ) );
            break;
        case Mq::GAME_ENTITY_REMOVED:
            gameEntityRemoved( *reinterpret_cast<GameEntity * const *>( data ) );
            break;
        case Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT:
            //Acknowledge/notify back that we're done with this slot.
            this->queueSendMessage( mLogicSystem, Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT,
                                    *reinterpret_cast<const Ogre::uint32*>( data ) );
            break;
        case Mq::GAME_ENTITY_COLOUR_CHANGE:
            this->changeGameEntityColour( reinterpret_cast<const GameEntityManager::GameEntityColourChange*>( data ));
            break;
        case Mq::GAME_ENTITY_ALPHA_CHANGE:
            this->changeGameEntityAlpha(reinterpret_cast<const GameEntityManager::GameEntityAlphaChange*>(data));
            break;
        case Mq::GAME_ENTITY_VISIBILITY_CHANGE:
            this->changeGameEntityVisibility(reinterpret_cast<const GameEntityManager::GameEntityVisibilityChange*>(data));
            break;
        case Mq::LEAPFRAME_FINISHED:
            // noop
            break;
        case Mq::FIELD_COMPUTE_SYSTEM_WRITE_FILE_TESTING:
            {
                for (auto& iter : mFieldComputeSystems) {
                    iter->getTextureTicket3D()->download(iter->getPrimaryVelocityTexture(), 0, false);
                    iter->setDownloadingTextureViaTicket(true);
                }

                //for (auto& iter : mFieldComputeSystems) {
                //    iter->getRenderTargetTexture()->writeContentsToFile("velocity.png", 0, 1, true);
                //}

                //auto msg = reinterpret_cast<const FieldComputeSystem_TestMessage*>(data);
                //msg->mUavTextureGpu->writeContentsToFile(
                //    Ogre::String("testing-") + 
                //    Ogre::StringConverter::toString(msg->mTimeSinceLast) +
                //    Ogre::String(".png"), 0, 1, true);
            }
            break;
        case Mq::REMOVE_STAGING_TEXTURE:
            {
                Ogre::TextureGpuManager* texMgr = this->getRoot()->getRenderSystem()->getTextureGpuManager();
                auto msg = const_cast<FieldComputeSystem_StagingTextureMessage*>(reinterpret_cast<const FieldComputeSystem_StagingTextureMessage*>(data));
                texMgr->removeStagingTexture(msg->mStagingTexture);
                msg->mStagingTexture = 0;
            }
            break;
        default:
            break;
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::addResourceLocation( const Ogre::String &archName, const Ogre::String &typeName,
                                              const Ogre::String &secName )
    {
#if (OGRE_PLATFORM == OGRE_PLATFORM_APPLE) || (OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS)
        // OS X does not set the working directory relative to the app,
        // In order to make things portable on OS X we need to provide
        // the loading with it's own bundle path location
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    Ogre::String( Ogre::macBundlePath() + "/" + archName ), typeName, secName );
#else
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::loadTextureCache(void)
    {
#if !OGRE_NO_JSON
        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
        Ogre::Archive *rwAccessFolderArchive = archiveManager.load( mWriteAccessFolder,
                                                                    "FileSystem", true );
        try
        {
            const Ogre::String filename = "textureMetadataCache.json";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr stream = rwAccessFolderArchive->open( filename );
                std::vector<char> fileData;
                fileData.resize( stream->size() + 1 );
                if( !fileData.empty() )
                {
                    stream->read( &fileData[0], stream->size() );
                    //Add null terminator just in case (to prevent bad input)
                    fileData.back() = '\0';
                    Ogre::TextureGpuManager *textureManager =
                            mRoot->getRenderSystem()->getTextureGpuManager();
                    textureManager->importTextureMetadataCache( stream->getName(), &fileData[0], false );
                }
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage(
                            "[INFO] Texture cache not found at " + mWriteAccessFolder +
                            "/textureMetadataCache.json" );
            }
        }
        catch( Ogre::Exception &e )
        {
            Ogre::LogManager::getSingleton().logMessage( e.getFullDescription() );
        }

        archiveManager.unload( rwAccessFolderArchive );
#endif
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::saveTextureCache(void)
    {
        if( mRoot->getRenderSystem() )
        {
            Ogre::TextureGpuManager *textureManager = mRoot->getRenderSystem()->getTextureGpuManager();
            if( textureManager )
            {
                Ogre::String jsonString;
                textureManager->exportTextureMetadataCache( jsonString );
                const Ogre::String path = mWriteAccessFolder + "/textureMetadataCache.json";
                std::ofstream file( path.c_str(), std::ios::binary | std::ios::out );
                if( file.is_open() )
                    file.write( jsonString.c_str(), static_cast<std::streamsize>( jsonString.size() ) );
                file.close();
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::loadHlmsDiskCache(void)
    {
        if( !mUseMicrocodeCache && !mUseHlmsDiskCache )
            return;

        Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();
        Ogre::HlmsDiskCache diskCache( hlmsManager );

        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

        Ogre::Archive *rwAccessFolderArchive = archiveManager.load( mWriteAccessFolder,
                                                                    "FileSystem", true );

        if( mUseMicrocodeCache )
        {
            //Make sure the microcode cache is enabled.
            Ogre::GpuProgramManager::getSingleton().setSaveMicrocodesToCache( true );
            const Ogre::String filename = "microcodeCodeCache.cache";
            if( rwAccessFolderArchive->exists( filename ) )
            {
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->open( filename );
                Ogre::GpuProgramManager::getSingleton().loadMicrocodeCache( shaderCacheFile );
            }
        }

        if( mUseHlmsDiskCache )
        {
            for( size_t i=Ogre::HLMS_LOW_LEVEL + 1u; i<Ogre::HLMS_MAX; ++i )
            {
                Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                if( hlms )
                {
                    Ogre::String filename = "hlmsDiskCache" +
                                            Ogre::StringConverter::toString( i ) + ".bin";

                    try
                    {
                        if( rwAccessFolderArchive->exists( filename ) )
                        {
                            Ogre::DataStreamPtr diskCacheFile = rwAccessFolderArchive->open( filename );
                            diskCache.loadFrom( diskCacheFile );
                            diskCache.applyTo( hlms );
                        }
                    }
                    catch( Ogre::Exception& )
                    {
                        Ogre::LogManager::getSingleton().logMessage(
                                    "Error loading cache from " + mWriteAccessFolder + "/" +
                                    filename + "! If you have issues, try deleting the file "
                                    "and restarting the app" );
                    }
                }
            }
        }

        archiveManager.unload( mWriteAccessFolder );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::saveHlmsDiskCache(void)
    {
        if( mRoot->getRenderSystem() && Ogre::GpuProgramManager::getSingletonPtr() &&
            (mUseMicrocodeCache || mUseHlmsDiskCache) )
        {
            Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();
            Ogre::HlmsDiskCache diskCache( hlmsManager );

            Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

            Ogre::Archive *rwAccessFolderArchive = archiveManager.load( mWriteAccessFolder,
                                                                        "FileSystem", false );

            if( mUseHlmsDiskCache )
            {
                for( size_t i=Ogre::HLMS_LOW_LEVEL + 1u; i<Ogre::HLMS_MAX; ++i )
                {
                    Ogre::Hlms *hlms = hlmsManager->getHlms( static_cast<Ogre::HlmsTypes>( i ) );
                    if( hlms )
                    {
                        diskCache.copyFrom( hlms );

                        Ogre::DataStreamPtr diskCacheFile =
                                rwAccessFolderArchive->create( "hlmsDiskCache" +
                                                               Ogre::StringConverter::toString( i ) +
                                                               ".bin" );
                        diskCache.saveTo( diskCacheFile );
                    }
                }
            }

            if( Ogre::GpuProgramManager::getSingleton().isCacheDirty() && mUseMicrocodeCache )
            {
                const Ogre::String filename = "microcodeCodeCache.cache";
                Ogre::DataStreamPtr shaderCacheFile = rwAccessFolderArchive->create( filename );
                Ogre::GpuProgramManager::getSingleton().saveMicrocodeCache( shaderCacheFile );
            }

            archiveManager.unload( mWriteAccessFolder );
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::setupResources(void)
    {
        // Load resource paths from config file
        Ogre::ConfigFile cf;
        cf.load(mResourcePath + "resources2.cfg");

        // Go through all sections & settings in the file
        Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

        Ogre::String secName, typeName, archName;
        while( seci.hasMoreElements() )
        {
            secName = seci.peekNextKey();
            Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

            if( secName != "Hlms" )
            {
                Ogre::ConfigFile::SettingsMultiMap::iterator i;
                for (i = settings->begin(); i != settings->end(); ++i)
                {
                    typeName = i->first;
                    archName = i->second;
                    addResourceLocation( archName, typeName, secName );
                }
            }
        }

        Ogre::String originalDataFolder = cf.getSetting("ComputeFolder", "Hlms", "");

        const char* c_locations[4] =
        {
            "",
            "GLSL",
            "HLSL",
            "Metal"
        };

        for (size_t i = 0; i < 4; ++i)
        {
            Ogre::String dataFolder = originalDataFolder + c_locations[i];
            addResourceLocation(dataFolder, "FileSystem", "General");
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::registerHlms(void)
    {
        Ogre::ConfigFile cf;
        cf.load( mResourcePath + "resources2.cfg" );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
        Ogre::String rootHlmsFolder = Ogre::macBundlePath() + '/' +
                                  cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#else
        Ogre::String rootHlmsFolder = mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#endif

        if( rootHlmsFolder.empty() )
            rootHlmsFolder = "./";
        else if( *(rootHlmsFolder.end() - 1) != '/' )
            rootHlmsFolder += "/";

        //At this point rootHlmsFolder should be a valid path to the Hlms data folder

        Ogre::HlmsUnlit *hlmsUnlit = 0;
        Ogre::HlmsPbs *hlmsPbs = 0;

        //For retrieval of the paths to the different folders needed
        Ogre::String mainFolderPath;
        Ogre::StringVector libraryFoldersPaths;
        Ogre::StringVector::const_iterator libraryFolderPathIt;
        Ogre::StringVector::const_iterator libraryFolderPathEn;

        Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();
        
        {
            //Create & Register HlmsUnlit
            //Get the path to all the subdirectories used by HlmsUnlit
            Ogre::HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
            Ogre::Archive *archiveUnlit = archiveManager.load( rootHlmsFolder + mainFolderPath,
                                                               "FileSystem", true );
            Ogre::ArchiveVec archiveUnlitLibraryFolders;
            libraryFolderPathIt = libraryFoldersPaths.begin();
            libraryFolderPathEn = libraryFoldersPaths.end();
            while( libraryFolderPathIt != libraryFolderPathEn )
            {
                Ogre::Archive *archiveLibrary =
                        archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
                archiveUnlitLibraryFolders.push_back( archiveLibrary );
                ++libraryFolderPathIt;
            }

            //Create and register the unlit Hlms
            hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
            Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
        }

        {
            //Create & Register HlmsPbs
            //Do the same for HlmsPbs:
            Ogre::HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
            Ogre::Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath,
                                                             "FileSystem", true );

            //Get the library archive(s)
            Ogre::ArchiveVec archivePbsLibraryFolders;
            libraryFolderPathIt = libraryFoldersPaths.begin();
            libraryFolderPathEn = libraryFoldersPaths.end();
            while( libraryFolderPathIt != libraryFolderPathEn )
            {
                Ogre::Archive *archiveLibrary =
                        archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
                archivePbsLibraryFolders.push_back( archiveLibrary );
                ++libraryFolderPathIt;
            }

            //Create and register
            hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &archivePbsLibraryFolders );
            Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
        }


        Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();
        if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
        {
            //Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
            //and below to avoid saturating AMD's discard limit (8MB) or
            //saturate the PCIE bus in some low end machines.
            bool supportsNoOverwriteOnTextureBuffers;
            renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
                                              &supportsNoOverwriteOnTextureBuffers );

            if( !supportsNoOverwriteOnTextureBuffers )
            {
                hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
                hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::loadResources(void)
    {
        registerHlms();

        loadTextureCache();
        loadHlmsDiskCache();

        // Initialise, parse scripts etc
        Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );

        // Initialize resources for LTC area lights and accurate specular reflections (IBL)
        Ogre::Hlms *hlms = mRoot->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
        OGRE_ASSERT_HIGH( dynamic_cast<Ogre::HlmsPbs*>( hlms ) );
        Ogre::HlmsPbs *hlmsPbs = static_cast<Ogre::HlmsPbs*>( hlms );
        try
        {
            hlmsPbs->loadLtcMatrix();
        }
        catch( Ogre::FileNotFoundException &e )
        {
            Ogre::LogManager::getSingleton().logMessage( e.getFullDescription(), Ogre::LML_CRITICAL );
            Ogre::LogManager::getSingleton().logMessage(
                "WARNING: LTC matrix textures could not be loaded. Accurate specular IBL reflections "
                "and LTC area lights won't be available or may not function properly!",
                Ogre::LML_CRITICAL );
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::chooseSceneManager(void)
    {
#if OGRE_DEBUG_MODE
        //Debugging multithreaded code is a PITA, disable it.
        const size_t numThreads = 1;
#else
        //getNumLogicalCores() may return 0 if couldn't detect
        const size_t numThreads = std::max<size_t>(1, 8);// Ogre::PlatformInformation::getNumLogicalCores() );
#endif
        // Create the SceneManager, in this case a generic one
        mSceneManager = mRoot->createSceneManager( Ogre::ST_GENERIC,
                                                   numThreads,
                                                   "ExampleSMInstance" );

        mSceneManager->addRenderQueueListener( mOverlaySystem );
        mSceneManager->getRenderQueue()->setSortRenderQueue(
                    Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId,
                    Ogre::RenderQueue::StableSort );

        //Set sane defaults for proper shadow mapping
        mSceneManager->setShadowDirectionalLightExtrusionDistance( 500.0f );
        mSceneManager->setShadowFarDistance( 500.0f );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::createCamera(void)
    {
        mCamera = mSceneManager->createCamera( "Main Camera" );

        mCamera->setPosition( Ogre::Vector3( 0, 0, 0 ) );
        mCamera->lookAt( Ogre::Vector3( 0, 0, -1 ) );

        mCamera->setNearClipDistance( 0.2f );
        mCamera->setFarClipDistance( 1000.0f );
        mCamera->setAutoAspectRatio( true );
    }
    //-----------------------------------------------------------------------------------
    Ogre::CompositorWorkspace* GraphicsSystem::setupCompositor(void)
    {
        Ogre::CompositorManager2 *compositorManager = mRoot->getCompositorManager2();

        const Ogre::String workspaceName( "Demo Workspace" );

        if( !compositorManager->hasWorkspaceDefinition( workspaceName ) )
        {
            compositorManager->createBasicWorkspaceDef( workspaceName, mBackgroundColour,
                                                        Ogre::IdString() );
        }

        return compositorManager->addWorkspace(
            mSceneManager,
            mRenderWindow->getTexture(),
            mCamera,
            workspaceName,
            true);
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::initMiscParamsListener( Ogre::NameValuePairList &params )
    {
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::stopCompositor(void)
    {
        Ogre::CompositorManager2* compositorManager = mRoot->getCompositorManager2();

        auto it = mWorkspaces.begin();

        while (it != mWorkspaces.end())
        {
            compositorManager->removeWorkspace(*it);
            *it = 0;
            it = mWorkspaces.erase(it);
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::restartCompositor(void)
    {
        stopCompositor();
        mPrimaryWorkspace = setupCompositor();

        mWorkspaces.push_back(mPrimaryWorkspace);
    }
    //-----------------------------------------------------------------------------------
    struct GameEntityCmp
    {
        bool operator () ( const GameEntity *_l, const Ogre::Matrix4 * RESTRICT_ALIAS _r ) const
        {
            const Ogre::Transform &transform = _l->mSceneNode->_getTransform();
            return &transform.mDerivedTransform[transform.mIndex] < _r;
        }

        bool operator () ( const Ogre::Matrix4 * RESTRICT_ALIAS _r, const GameEntity *_l ) const
        {
            const Ogre::Transform &transform = _l->mSceneNode->_getTransform();
            return _r < &transform.mDerivedTransform[transform.mIndex];
        }

        bool operator () ( const GameEntity *_l, const GameEntity *_r ) const
        {
            const Ogre::Transform &lTransform = _l->mSceneNode->_getTransform();
            const Ogre::Transform &rTransform = _r->mSceneNode->_getTransform();
            return &lTransform.mDerivedTransform[lTransform.mIndex] < &rTransform.mDerivedTransform[rTransform.mIndex];
        }
    };
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::gameEntityAdded( const GameEntityManager::CreatedGameEntity *cge )
    {
        Ogre::SceneNode *sceneNode = mSceneManager->getRootSceneNode( cge->gameEntity->mType )->
                createChildSceneNode( cge->gameEntity->mType,
                                      cge->initialTransform.vPos,
                                      cge->initialTransform.qRot );

        sceneNode->setName(cge->name);

        sceneNode->setScale( cge->initialTransform.vScale );

        cge->gameEntity->mSceneNode = sceneNode;

        switch (cge->gameEntity->mMoDefinition->moType) 
        {
            case MoTypeItem:
            {
                Ogre::Item* item = mSceneManager->createItem(cge->gameEntity->mMoDefinition->meshName,
                    cge->gameEntity->mMoDefinition->resourceGroup,
                    cge->gameEntity->mType);

                Ogre::StringVector materialNames = cge->gameEntity->mMoDefinition->submeshMaterials;
                size_t minMaterials = std::min(materialNames.size(), item->getNumSubItems());

                for (size_t i = 0; i < minMaterials; ++i)
                {
                    item->getSubItem(i)->setDatablockOrMaterialName(materialNames[i],
                        cge->gameEntity->mMoDefinition->
                        resourceGroup);

                    if (cge->useAlpha) {

                        auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());

                        Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
                        diffuseBlock.mU = Ogre::TAM_CLAMP;
                        diffuseBlock.mV = Ogre::TAM_CLAMP;
                        diffuseBlock.mW = Ogre::TAM_CLAMP;
                        datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

                        datablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::Transparent, true);
                    }
                }

                if (minMaterials == 0) {
                    //item->setDatablock("lambert3");
                    /*item->getSubItem(0)->setDatablockOrMaterialName("lambert3",
                        cge->gameEntity->mMoDefinition->
                        resourceGroup);*/
                }

                if (cge->useAlpha) {

                    auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(0)->getDatablock());

                    if (datablock)
                    {
                        datablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::Transparent, false);

                        //Ogre::HlmsMacroblock macroBlock = Ogre::HlmsMacroblock();
                        //macroBlock.mDepthWrite = true;
                        //macroBlock.mDepthCheck = true;

                        //datablock->setMacroblock(macroBlock);
                    }
                }

                item->setRenderQueueGroup(49u);

                //if (mWireAabb)
                //    mWireAabb->track(item);

                cge->gameEntity->mMovableObject = item;
            }
            break;
            case MoTypeStaticManualLineList: 
            {
                Ogre::ManualObject* mo = mSceneManager->createManualObject(Ogre::SCENE_STATIC);

                mo->begin(cge->gameEntity->mManualObjectDatablockName, Ogre::OT_LINE_LIST);

                for (auto p : cge->gameEntity->mManualObjectDefinition.points) {
                    mo->position(p);
                }

                for (auto l : cge->gameEntity->mManualObjectDefinition.lines) {
                    mo->line(l.first, l.second);
                }

                mo->end();

                cge->gameEntity->mMovableObject = mo;
            }
            break;
            case MoTypeDynamicManualLineList:
            {
                Ogre::ManualObject* mo = mSceneManager->createManualObject(Ogre::SCENE_DYNAMIC);

                mo->begin(cge->gameEntity->mManualObjectDatablockName, Ogre::OT_LINE_LIST);

                for (auto p : cge->gameEntity->mManualObjectDefinition.points) {
                    mo->position(p);
                }

                for (auto l : cge->gameEntity->mManualObjectDefinition.lines) {
                    mo->line(l.first, l.second);
                }

                mo->end();

                cge->gameEntity->mMovableObject = mo;
            }
            break;
            case MoTypeDynamicTriangleList:
            {
                Ogre::ManualObject* mo = mSceneManager->createManualObject(Ogre::SCENE_DYNAMIC);

                mo->begin(cge->gameEntity->mManualObjectDatablockName, Ogre::OT_TRIANGLE_LIST);

                float uvOffset = 0.0f;

                for (size_t i = 0; i < cge->gameEntity->mManualObjectDefinition.points.size();)
                {
                    mo->position(cge->gameEntity->mManualObjectDefinition.points[i]);
                    mo->normal(0.0f, 1.0f, 0.0f);
                    mo->tangent(1.0f, 0.0f, 0.0f);
                    mo->textureCoord(0.0f + uvOffset, 1.0f + uvOffset);

                    mo->position(cge->gameEntity->mManualObjectDefinition.points[i + 1]);
                    mo->normal(0.0f, 1.0f, 0.0f);
                    mo->tangent(1.0f, 0.0f, 0.0f);
                    mo->textureCoord(1.0f + uvOffset, 1.0f + uvOffset);

                    mo->position(cge->gameEntity->mManualObjectDefinition.points[i + 2]);
                    mo->normal(0.0f, 1.0f, 0.0f);
                    mo->tangent(1.0f, 0.0f, 0.0f);
                    mo->textureCoord(1.0f + uvOffset, 0.0f + uvOffset);

                    mo->position(cge->gameEntity->mManualObjectDefinition.points[i + 3]);
                    mo->normal(0.0f, 1.0f, 0.0f);
                    mo->tangent(1.0f, 0.0f, 0.0f);
                    mo->textureCoord(0.0f + uvOffset, 0.0f + uvOffset);

                    mo->quad(i, i + 1, i + 2, i + 3);

                    i += 4;
                }

                mo->end();

                if (cge->gameEntity->mTextureGpu == 0)
                {
                    mo->getSection(0)->setDatablock(cge->gameEntity->mManualObjectDatablockName);

                    assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(mo->getSection(0)->getDatablock()));

                    auto personalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(mo->getSection(0)->getDatablock());

                    Ogre::HlmsSamplerblock diffuseBlock(*personalDatablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
                    diffuseBlock.mU = Ogre::TAM_CLAMP;
                    diffuseBlock.mV = Ogre::TAM_CLAMP;
                    diffuseBlock.mW = Ogre::TAM_CLAMP;
                    personalDatablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

                    if (cge->useAlpha)
                        personalDatablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::Transparent, false);

                    if (cge->vColour != Ogre::Vector3::ZERO)
                        personalDatablock->setDiffuse(cge->vColour);

                    mo->getSection(0)->setDatablock(personalDatablock);
                }
                else
                {
                    Ogre::HlmsManager* hlmsMgr = this->getRoot()->getHlmsManager();

                    Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsMgr->getHlms(Ogre::HLMS_PBS));

                    auto personalDatablock = static_cast<Ogre::HlmsPbsDatablock*>(hlmsPbs->createDatablock(
                        "",
                        "",
                        Ogre::HlmsMacroblock(),
                        Ogre::HlmsBlendblock(),
                        Ogre::HlmsParamVec()
                    ));

                    personalDatablock->setFresnel(Ogre::Vector3::ZERO, true);

                    personalDatablock->setTexture(Ogre::PbsTextureTypes::PBSM_DIFFUSE, cge->gameEntity->mTextureGpu);

                    Ogre::HlmsSamplerblock diffuseBlock(*personalDatablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
                    diffuseBlock.mU = Ogre::TAM_CLAMP;
                    diffuseBlock.mV = Ogre::TAM_CLAMP;
                    diffuseBlock.mW = Ogre::TAM_CLAMP;

                    personalDatablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

                    personalDatablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::TransparencyModes::Transparent, true, true);

                    mo->getSection(0)->setDatablock(personalDatablock);
                }

                mo->setRenderQueueGroup(50u);

                //if (mWireAabb)
                //    mWireAabb->track(mo);

                cge->gameEntity->mMovableObject = mo;
            }
            break;
            case MoTypePrefabPlane:
            {
                Ogre::v1::Entity* pft = mSceneManager->createEntity(Ogre::SceneManager::PrefabType::PT_PLANE, Ogre::SCENE_DYNAMIC);

                pft->setDatablock(cge->gameEntity->mManualObjectDatablockName);

                assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(pft->getSubEntity(0)->getDatablock()));

                auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(pft->getSubEntity(0)->getDatablock());

                auto personalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablock->clone(Ogre::StringConverter::toString(cge->gameEntity->getId()) + Ogre::String("personalDataBlock")));

                Ogre::HlmsSamplerblock diffuseBlock(*personalDatablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
                diffuseBlock.mU = Ogre::TAM_CLAMP;
                diffuseBlock.mV = Ogre::TAM_CLAMP;
                diffuseBlock.mW = Ogre::TAM_CLAMP;
                personalDatablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

                if (cge->useAlpha)
                    personalDatablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::Transparent, true);

                if (cge->vColour != Ogre::Vector3::ZERO)
                    personalDatablock->setDiffuse(cge->vColour);

                pft->getSubEntity(0)->setDatablock(personalDatablock);

                cge->gameEntity->mMovableObject = pft;

                /*Ogre::v1::Entity* pft = mSceneManager->createEntity(Ogre::SceneManager::PrefabType::PT_PLANE, cge->gameEntity->mType);

                if (cge->gameEntity->mTextureGpu == 0) 
                {
                    pft->setDatablock(cge->gameEntity->mManualObjectDatablockName);

                    assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(pft->getSubEntity(0)->getDatablock()));

                    auto personalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(pft->getSubEntity(0)->getDatablock());

                    Ogre::HlmsSamplerblock diffuseBlock(*personalDatablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
                    diffuseBlock.mU = Ogre::TAM_CLAMP;
                    diffuseBlock.mV = Ogre::TAM_CLAMP;
                    diffuseBlock.mW = Ogre::TAM_CLAMP;
                    personalDatablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

                    if (cge->useAlpha)
                        personalDatablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::Transparent, true);

                    if (cge->vColour != Ogre::Vector3::ZERO)
                        personalDatablock->setDiffuse(cge->vColour);

                    pft->getSubEntity(0)->setDatablock(personalDatablock);
                }
                else
                {
                    Ogre::HlmsManager* hlmsMgr = this->getRoot()->getHlmsManager();

                    Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsMgr->getHlms(Ogre::HLMS_PBS));

                    auto personalDatablock = static_cast<Ogre::HlmsPbsDatablock*>(hlmsPbs->createDatablock(
                        "",
                        "",
                        Ogre::HlmsMacroblock(),
                        Ogre::HlmsBlendblock(),
                        Ogre::HlmsParamVec()
                    ));

                    personalDatablock->setFresnel(Ogre::Vector3::ZERO, true);

                    personalDatablock->setTexture(Ogre::PbsTextureTypes::PBSM_DIFFUSE, cge->gameEntity->mTextureGpu);

                    Ogre::HlmsSamplerblock diffuseBlock(*personalDatablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
                    diffuseBlock.mU = Ogre::TAM_CLAMP;
                    diffuseBlock.mV = Ogre::TAM_CLAMP;
                    diffuseBlock.mW = Ogre::TAM_CLAMP;
                    
                    personalDatablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

                    personalDatablock->setTransparency(1.0f, Ogre::HlmsPbsDatablock::TransparencyModes::Fade, true, true);

                    pft->getSubEntity(0)->setDatablock(personalDatablock);

                }


                cge->gameEntity->mMovableObject = pft;*/
            }
            break;
            case MoTypeCamera:
            {
                auto c = mSceneManager->getCameras()[0];
                auto o = c->getOrientation();
                auto p = c->getPosition();

                cge->gameEntity->mMovableObject = c;

                //mSceneManager->destroySceneNode(sceneNode);

                //sceneNode = c->getParentSceneNode();

                c->detachFromParent();

                //sceneNode->rotate(o);
                //sceneNode->translate(p);

                //sceneNode->setOrientation(c->getOrientation());
                //sceneNode->setPosition(c->getPosition());

                //if (cge->gameEntity->mMovableObject->isAttached())
                //    cge->gameEntity->mMovableObject->detachFromParent();
            }
            break;
            case MoTypeFieldComputeSystem:
            {
                assert(static_cast<FieldComputeSystem*>(cge->gameEntity));

                auto fieldComputeSystem = static_cast<FieldComputeSystem*>(cge->gameEntity);

                fieldComputeSystem->_notifyGraphicsSystem(this);

                Ogre::CompositorManager2* compositorManager = mRoot->getCompositorManager2();

                const Ogre::String workspaceName("Test Compute Workspace");

                if (!compositorManager->hasWorkspaceDefinition(workspaceName))
                {
                    compositorManager->createBasicWorkspaceDef(workspaceName, mBackgroundColour,
                        Ogre::IdString());
                }

                Ogre::Root* root = this->getRoot();

                Ogre::HlmsCompute* hlmsCompute = root->getHlmsManager()->getComputeHlms();

                fieldComputeSystem->setTestComputeJob(hlmsCompute->findComputeJob("TestJob"));
                fieldComputeSystem->setAdvectionCopyComputeJob(hlmsCompute->findComputeJob("AdvectionCopy"));
                fieldComputeSystem->setBoundaryConditionsComputeJob(hlmsCompute->findComputeJob("BoundaryConditions"));
                fieldComputeSystem->setClearBuffersComputeJob(hlmsCompute->findComputeJob("ClearBuffers1"));
                fieldComputeSystem->setClearBuffersTwoComputeJob(hlmsCompute->findComputeJob("ClearBuffers2"));
                fieldComputeSystem->setVelocityAdvectionComputeJob(hlmsCompute->findComputeJob("VelocityAdvection"));
                fieldComputeSystem->setInkAdvectionComputeJob(hlmsCompute->findComputeJob("InkAdvection"));
                fieldComputeSystem->setAddImpulsesComputeJob(hlmsCompute->findComputeJob("AddImpulses"));
                fieldComputeSystem->setDivergenceComputeJob(hlmsCompute->findComputeJob("Divergence"));
                fieldComputeSystem->setJacobiPressureComputeJob(hlmsCompute->findComputeJob("JacobiPressure"));
                fieldComputeSystem->setJacobiDiffusionComputeJob(hlmsCompute->findComputeJob("JacobiDiffusion"));
                fieldComputeSystem->setSubtractPressureGradientComputeJob(hlmsCompute->findComputeJob("SubtractPressureGradient"));
                fieldComputeSystem->setVorticityComputationComputeJob(hlmsCompute->findComputeJob("VorticityComputation"));
                fieldComputeSystem->setVorticityConfinementComputeJob(hlmsCompute->findComputeJob("VorticityConfinement"));

                fieldComputeSystem->setMaterial(Ogre::MaterialManager::getSingleton().load(
                    "DrawFromUavBuffer", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).
                    staticCast<Ogre::Material>());

                Ogre::RenderSystem* renderSystem = this->getRoot()->getRenderSystem();
                Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

                Ogre::TextureGpuManager* texMgr = root->getRenderSystem()->getTextureGpuManager();

                fieldComputeSystem->setVelocityStagingTexture(texMgr->getStagingTexture(
                    fieldComputeSystem->getBufferResolutionWidth(),
                    fieldComputeSystem->getBufferResolutionHeight(),
                    fieldComputeSystem->getDepth(),
                    fieldComputeSystem->getDepth(),
                    fieldComputeSystem->getPixelFormat3D()));

                fieldComputeSystem->setInkStagingTexture(texMgr->getStagingTexture(
                    fieldComputeSystem->getBufferResolutionWidth(),
                    fieldComputeSystem->getBufferResolutionHeight(),
                    fieldComputeSystem->getDepth(),
                    fieldComputeSystem->getDepth(),
                    fieldComputeSystem->getPixelFormat3D()));

                fieldComputeSystem->setAsyncTextureTicket2D(texMgr->createAsyncTextureTicket(
                    fieldComputeSystem->getBufferResolutionWidth(),
                    fieldComputeSystem->getBufferResolutionHeight(),
                    fieldComputeSystem->getDepth(),
                    fieldComputeSystem->getTextureType2D(),
                    fieldComputeSystem->getPixelFormat2D()));

                fieldComputeSystem->setAsyncTextureTicket3D(texMgr->createAsyncTextureTicket(
                    fieldComputeSystem->getBufferResolutionWidth(),
                    fieldComputeSystem->getBufferResolutionHeight(),
                    fieldComputeSystem->getDepth(),
                    fieldComputeSystem->getTextureType3D(),
                    fieldComputeSystem->getPixelFormat3D()));

                Ogre::TextureGpu* textures[8];

                textures[0] =
                    texMgr->createTexture(
                        "renderTexture",
                        "renderTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::RenderToTexture,
                        fieldComputeSystem->getTextureType2D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[1] =
                    texMgr->createTexture(
                        "primaryVelocityTexture",
                        "primaryVelocityTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[2] =
                    texMgr->createTexture(
                        "secondaryVelocityTexture",
                        "secondaryVelocityTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::Reinterpretable | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[3] =
                    texMgr->createTexture(
                        "pressureTexture",
                        "pressureTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::ManualTexture | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[4] =
                    texMgr->createTexture(
                        "pressureGradientTexture",
                        "pressureGradientTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::ManualTexture | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[5] =
                    texMgr->createTexture(
                        "divergenceTexture",
                        "divergenceTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::ManualTexture | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[6] =
                    texMgr->createOrRetrieveTexture(
                        "primaryInkTexture",
                        "primaryInkTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::ManualTexture | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                textures[7] =
                    texMgr->createTexture(
                        "secondaryInkTexture",
                        "secondaryInkTexture",
                        Ogre::GpuPageOutStrategy::Discard,
                        Ogre::TextureFlags::ManualTexture | Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::Uav,
                        fieldComputeSystem->getTextureType3D(),
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                        0u,
                        0u);

                fieldComputeSystem->setRenderTargetTexture(textures[0]);

                fieldComputeSystem->setVelocityTexture(textures[1]);

                fieldComputeSystem->setSecondaryVelocityTexture(textures[2]);

                fieldComputeSystem->setPressureTexture(textures[3]);

                fieldComputeSystem->setPressureGradientTexture(textures[4]);

                fieldComputeSystem->setDivergenceTexture(textures[5]);

                fieldComputeSystem->setInkTexture(textures[6]);

                fieldComputeSystem->setSecondaryInkTexture(textures[7]);

                size_t uavBufferNumElements = fieldComputeSystem->getBufferResolutionWidth() * fieldComputeSystem->getBufferResolutionHeight();

                auto buffer = vaoManager->createUavBuffer(
                    uavBufferNumElements, 
                    sizeof(Particle), 
                    Ogre::BufferBindFlags::BB_FLAG_UAV | Ogre::BufferBindFlags::BB_FLAG_TEX,
                    0, 
                    false);

                // thanks https://forums.ogre3d.org/viewtopic.php?t=96286
                auto mCpuInstanceBuffer = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(
                    uavBufferNumElements * sizeof(Particle), Ogre::MEMCATEGORY_GENERAL));
                
                float* RESTRICT_ALIAS instanceBuffer = reinterpret_cast<float*>(mCpuInstanceBuffer);

                const float* instanceBufferStart = instanceBuffer;

                //auto c = Ogre::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
                auto c = Ogre::Vector4(0.0450033244f, 0.1421513113f, 0.4302441212f, 1.0f);

                for (auto i = 0; i < uavBufferNumElements; ++i) {

                    *instanceBuffer++ = 0.0f; //ink

                    *instanceBuffer++ = c.x; // colour
                    *instanceBuffer++ = c.y;
                    *instanceBuffer++ = c.z;
                    *instanceBuffer++ = c.w;

                    *instanceBuffer++ = 0.0f; // velocity.x
                    *instanceBuffer++ = 0.0f; // velocity.y
                    *instanceBuffer++ = 0.0f; // velocity.z

                }

                OGRE_ASSERT_LOW((size_t)(instanceBuffer - instanceBufferStart) * sizeof(float) <=
                    buffer->getTotalSizeBytes());

                memset(instanceBuffer, 0, buffer->getTotalSizeBytes() -
                    (static_cast<size_t>(instanceBuffer - instanceBufferStart) * sizeof(float)));

                // this just sets the entire buffer to opaque black
                buffer->upload(mCpuInstanceBuffer, 0u, buffer->getNumElements());
                
                fieldComputeSystem->addUavBuffer(buffer);

                fieldComputeSystem->getRenderTargetTexture()->setNumMipmaps(1);
                fieldComputeSystem->getRenderTargetTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getRenderTargetTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat2D());

                fieldComputeSystem->getPrimaryVelocityTexture()->setNumMipmaps(1);
                fieldComputeSystem->getPrimaryVelocityTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getPrimaryVelocityTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                fieldComputeSystem->getSecondaryVelocityTexture()->setNumMipmaps(1);
                fieldComputeSystem->getSecondaryVelocityTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getSecondaryVelocityTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                fieldComputeSystem->getPressureTexture()->setNumMipmaps(1);
                fieldComputeSystem->getPressureTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getPressureTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                fieldComputeSystem->getPressureGradientTexture()->setNumMipmaps(1);
                fieldComputeSystem->getPressureGradientTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getPressureGradientTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                fieldComputeSystem->getDivergenceTexture()->setNumMipmaps(1);
                fieldComputeSystem->getDivergenceTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getDivergenceTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                fieldComputeSystem->getPrimaryInkTexture()->setNumMipmaps(1);
                fieldComputeSystem->getPrimaryInkTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getPrimaryInkTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                fieldComputeSystem->getSecondaryInkTexture()->setNumMipmaps(1);
                fieldComputeSystem->getSecondaryInkTexture()->setResolution(fieldComputeSystem->getBufferResolutionWidth(), fieldComputeSystem->getBufferResolutionHeight());
                fieldComputeSystem->getSecondaryInkTexture()->setPixelFormat(fieldComputeSystem->getPixelFormat3D());

                //Ogre::DescriptorSetUav::TextureSlot uavSlot(Ogre::DescriptorSetUav::TextureSlot::makeEmpty());

                //uavSlot.access = Ogre::ResourceAccess::ReadWrite;
                //uavSlot.pixelFormat = fieldComputeSystem->getPixelFormat2D();
                //uavSlot.texture = fieldComputeSystem->getRenderTargetTexture();

                //Ogre::DescriptorSetUav::BufferSlot bufferSlot(Ogre::DescriptorSetUav::BufferSlot::makeEmpty());

                //bufferSlot.access = Ogre::ResourceAccess::ReadWrite;
                //bufferSlot.buffer = fieldComputeSystem->getUavBuffer(0);
                //bufferSlot.sizeBytes = uavBufferNumElements * 4;
                //bufferSlot.offset = 0;

                //fieldComputeSystem->getTestComputeJob()->_setUavTexture(0, uavSlot);

                //uavSlot.pixelFormat = fieldComputeSystem->getPixelFormat3D();

                //uavSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                //fieldComputeSystem->getTestComputeJob()->_setUavTexture(1, uavSlot);

                //uavSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                //fieldComputeSystem->getTestComputeJob()->_setUavTexture(2, uavSlot);


                //Ogre::DescriptorSetTexture2::TextureSlot textureSlot(Ogre::DescriptorSetTexture2::TextureSlot::makeEmpty());
                //textureSlot.generalReadWrite = true;// Ogre::ResourceAccess::ReadWrite;
                //textureSlot.pixelFormat = fieldComputeSystem->getPixelFormat3D();


                // Inputs are added to the primaries
               /* uavSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getAddImpulsesComputeJob()->_setUavTexture(0, uavSlot);*/

                // noooope: now using a temporary ink 3D texture created on the GPU
                // primaryInkTexture should STAY PUT GADDAMMIT
                /*uavSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                fieldComputeSystem->getAddImpulsesComputeJob()->_setUavTexture(1, uavSlot);*/

                //fieldComputeSystem->getAddImpulsesComputeJob()->_setUavBuffer(0, bufferSlot);



                // Copy writes the primaries directly to the secondaries
                /*uavSlot.texture = fieldComputeSystem->getSecondaryVelocityTexture();
                fieldComputeSystem->getAdvectionCopyComputeJob()->_setUavTexture(0, uavSlot);

                uavSlot.texture = fieldComputeSystem->getSecondaryInkTexture();
                fieldComputeSystem->getAdvectionCopyComputeJob()->_setUavTexture(1, uavSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getAdvectionCopyComputeJob()->setTexture(0, textureSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                fieldComputeSystem->getAdvectionCopyComputeJob()->setTexture(1, textureSlot);*/
                


                // Boundary Conditions does... stuff
                /*uavSlot.texture = fieldComputeSystem->getSecondaryVelocityTexture();
                fieldComputeSystem->getBoundaryConditionsComputeJob()->_setUavTexture(0, uavSlot);

                uavSlot.texture = fieldComputeSystem->getSecondaryInkTexture();
                fieldComputeSystem->getBoundaryConditionsComputeJob()->_setUavTexture(1, uavSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getBoundaryConditionsComputeJob()->setTexture(0, textureSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                fieldComputeSystem->getBoundaryConditionsComputeJob()->setTexture(1, textureSlot);*/



                // Advection reads the secondaries and advects those onto to the primaries
                /*uavSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getAdvectionComputeJob()->_setUavTexture(0, uavSlot);

                textureSlot.texture = fieldComputeSystem->getSecondaryVelocityTexture();
                fieldComputeSystem->getAdvectionComputeJob()->setTexture(0, textureSlot);*/


                // Advection reads the secondaries and advects those onto to the primaries

                /*uavSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                fieldComputeSystem->getInkAdvectionComputeJob()->_setUavTexture(0, uavSlot);

                uavSlot.texture = fieldComputeSystem->getSecondaryInkTexture();
                fieldComputeSystem->getInkAdvectionComputeJob()->_setUavTexture(1, uavSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getInkAdvectionComputeJob()->setTexture(0, textureSlot);*/



                /*uavSlot.texture = fieldComputeSystem->getDivergenceTexture();
                fieldComputeSystem->getDivergenceComputeJob()->_setUavTexture(0, uavSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getDivergenceComputeJob()->setTexture(0, textureSlot);*/





                // Jacobi Diffusion should only occur if we decide the fluid has some visocity
                /*uavSlot.texture = fieldComputeSystem->getSecondaryVelocityTexture();
                fieldComputeSystem->getJacobiDiffusionComputeJob()->_setUavTexture(0, uavSlot);

                uavSlot.texture = fieldComputeSystem->getSecondaryInkTexture();
                fieldComputeSystem->getJacobiDiffusionComputeJob()->_setUavTexture(1, uavSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                fieldComputeSystem->getJacobiDiffusionComputeJob()->setTexture(0, textureSlot);

                textureSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                fieldComputeSystem->getJacobiDiffusionComputeJob()->setTexture(1, textureSlot);*/







                //uavSlot.texture = fieldComputeSystem->getPressureTexture();
                //fieldComputeSystem->getJacobiPressureComputeJob()->_setUavTexture(0, uavSlot);

                //textureSlot.texture = fieldComputeSystem->getDivergenceTexture();
                //fieldComputeSystem->getJacobiPressureComputeJob()->setTexture(0, textureSlot);








                //uavSlot.texture = fieldComputeSystem->getPrimaryVelocityTexture();
                //fieldComputeSystem->getSubtractPressureGradientComputeJob()->_setUavTexture(0, uavSlot);

                //textureSlot.texture = fieldComputeSystem->getPressureTexture();
                //fieldComputeSystem->getSubtractPressureGradientComputeJob()->setTexture(0, textureSlot);




                //fieldComputeSystem->getClearBuffersComputeJob()->_setUavBuffer(0, bufferSlot);
                //uavSlot.texture = fieldComputeSystem->getSecondaryInkTexture();
                //fieldComputeSystem->getClearBuffersComputeJob()->_setUavTexture(1, uavSlot);

                //fieldComputeSystem->getClearBuffersTwoComputeJob()->_setUavBuffer(0, bufferSlot);
                //uavSlot.texture = fieldComputeSystem->getPrimaryInkTexture();
                //fieldComputeSystem->getClearBuffersTwoComputeJob()->_setUavTexture(1, uavSlot);



                bool canUseSynchronousUpload = fieldComputeSystem
                    ->getRenderTargetTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident 
                    && fieldComputeSystem->getRenderTargetTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getRenderTargetTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getRenderTargetTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);


                canUseSynchronousUpload = fieldComputeSystem
                    ->getPrimaryVelocityTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getPrimaryVelocityTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getPrimaryVelocityTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getPrimaryVelocityTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);


                canUseSynchronousUpload = fieldComputeSystem
                    ->getSecondaryVelocityTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getSecondaryVelocityTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getSecondaryVelocityTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getSecondaryVelocityTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);


                canUseSynchronousUpload = fieldComputeSystem
                    ->getPressureTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getPressureTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getPressureTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getPressureTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);


                canUseSynchronousUpload = fieldComputeSystem
                    ->getPressureGradientTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getPressureGradientTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getPressureGradientTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getPressureGradientTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);

                canUseSynchronousUpload = fieldComputeSystem
                    ->getDivergenceTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getDivergenceTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getDivergenceTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getDivergenceTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);


                canUseSynchronousUpload = fieldComputeSystem
                    ->getPrimaryInkTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getPrimaryInkTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getPrimaryInkTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getPrimaryInkTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);


                canUseSynchronousUpload = fieldComputeSystem
                    ->getSecondaryInkTexture()
                    ->getNextResidencyStatus() == Ogre::GpuResidency::Resident
                    && fieldComputeSystem->getSecondaryInkTexture()->isDataReady();

                if (!canUseSynchronousUpload) {
                    fieldComputeSystem->getSecondaryInkTexture()->waitForData();
                }

                fieldComputeSystem
                    ->getSecondaryInkTexture()
                    ->scheduleTransitionTo(Ogre::GpuResidency::Resident, 0, true);









                auto tVec = Ogre::CompositorChannelVec({
                        fieldComputeSystem->getRenderTargetTexture(),
                        fieldComputeSystem->getPrimaryVelocityTexture(),
                        fieldComputeSystem->getSecondaryVelocityTexture(),
                        fieldComputeSystem->getPressureTexture(),
                        fieldComputeSystem->getPressureGradientTexture(),
                        fieldComputeSystem->getDivergenceTexture(),
                        fieldComputeSystem->getPrimaryInkTexture(),
                        fieldComputeSystem->getSecondaryInkTexture()
                    });

                auto workspace = compositorManager->addWorkspace(
                    mSceneManager,
                    tVec,
                    mCamera,
                    workspaceName,
                    true,
                    -1,
                    fieldComputeSystem->getUavBuffers());
            
                auto seq = workspace->getNodeSequence();


                //auto v = workspace->isValid();

                //assert(v == true, "Workspace Invalid");

                mWorkspaces.push_back(workspace);

                mFieldComputeSystems.push_back(fieldComputeSystem);

                Ogre::ManualObject* mo = mSceneManager->createManualObject(Ogre::SCENE_STATIC);

                mo->begin(cge->gameEntity->mManualObjectDatablockName, Ogre::OT_LINE_LIST);

                for (auto p : cge->gameEntity->mManualObjectDefinition.points) {
                    mo->position(p);
                }

                for (auto l : cge->gameEntity->mManualObjectDefinition.lines) {
                    mo->line(l.first, l.second);
                }

                mo->end();

                cge->gameEntity->mMovableObject = mo;

                fieldComputeSystem->initialise();
            }
            break;
            default: 
            {
                throw "MyThirdOgre::MovableObjectType not catered for in GraphicsSystem::gameEntityAdded!";
            }
            break;
        }

        sceneNode->attachObject( cge->gameEntity->mMovableObject );

        sceneNode->setVisible( cge->visible );

        //Keep them sorted on how Ogre's internal memory manager assigned them memory,
        //to avoid false cache sharing when we update the nodes concurrently.
        const Ogre::Transform &transform = sceneNode->_getTransform();
        GameEntityVec::iterator itGameEntity = std::lower_bound(
                    mGameEntities[cge->gameEntity->mType].begin(),
                    mGameEntities[cge->gameEntity->mType].end(),
                    &transform.mDerivedTransform[transform.mIndex],
                    GameEntityCmp() );
        mGameEntities[cge->gameEntity->mType].insert( itGameEntity, cge->gameEntity );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::gameEntityRemoved( GameEntity *toRemove )
    {
        const Ogre::Transform &transform = toRemove->mSceneNode->_getTransform();
        GameEntityVec::iterator itGameEntity = std::lower_bound(
                    mGameEntities[toRemove->mType].begin(),
                    mGameEntities[toRemove->mType].end(),
                    &transform.mDerivedTransform[transform.mIndex],
                    GameEntityCmp() );

        assert( itGameEntity != mGameEntities[toRemove->mType].end() && *itGameEntity == toRemove );
        mGameEntities[toRemove->mType].erase( itGameEntity );

        toRemove->mSceneNode->getParentSceneNode()->removeAndDestroyChild( toRemove->mSceneNode );
        toRemove->mSceneNode = 0;

        assert( dynamic_cast<Ogre::Item*>( toRemove->mMovableObject ) );

        mSceneManager->destroyItem( static_cast<Ogre::Item*>( toRemove->mMovableObject ) );
        toRemove->mMovableObject = 0;
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::updateGameEntities( const GameEntityVec &gameEntities, float weight )
    {
        mThreadGameEntityToUpdate   = &gameEntities;
        mThreadWeight               = weight;

        //Note: You could execute a non-blocking scalable task and do something else, you should
        //wait for the task to finish right before calling renderOneFrame or before trying to
        //execute another UserScalableTask (you would have to be careful, but it could work).
        mSceneManager->executeUserScalableTask( this, true );
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::execute( size_t threadId, size_t numThreads )
    {
        size_t currIdx = mCurrentTransformIdx;
        size_t prevIdx = (mCurrentTransformIdx + NUM_GAME_ENTITY_BUFFERS - 1) % NUM_GAME_ENTITY_BUFFERS;

        const size_t objsPerThread = (mThreadGameEntityToUpdate->size() + (numThreads - 1)) / numThreads;
        const size_t toAdvance = std::min( threadId * objsPerThread, mThreadGameEntityToUpdate->size() );

        GameEntityVec::const_iterator itor = mThreadGameEntityToUpdate->begin() + toAdvance;
        GameEntityVec::const_iterator end  = mThreadGameEntityToUpdate->begin() +
                                                                std::min( toAdvance + objsPerThread,
                                                                          mThreadGameEntityToUpdate->size() );
        while( itor != end )
        {
            GameEntity *gEnt = *itor;
            Ogre::Vector3 interpVec = Ogre::Math::lerp( gEnt->mTransform[prevIdx]->vPos,
                                                        gEnt->mTransform[currIdx]->vPos, mThreadWeight );
            gEnt->mSceneNode->setPosition( interpVec );

            interpVec = Ogre::Math::lerp( gEnt->mTransform[prevIdx]->vScale,
                                          gEnt->mTransform[currIdx]->vScale, mThreadWeight );
            gEnt->mSceneNode->setScale( interpVec );

            Ogre::Quaternion interpQ = Ogre::Quaternion::nlerp(
                mThreadWeight, gEnt->mTransform[prevIdx]->qRot, gEnt->mTransform[currIdx]->qRot, true);
            gEnt->mSceneNode->setOrientation(interpQ);
            
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::changeGameEntityColour(const GameEntityManager::GameEntityColourChange* change) 
    {
        if (change->gameEntity) {
             Ogre::v1::Entity* pEnt = 0;
             pEnt = static_cast<Ogre::v1::Entity*>(change->gameEntity->mMovableObject);
             if (pEnt) {
                 auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(pEnt->getSubEntity(0)->getDatablock());
                 if (datablock) 
                    datablock->setDiffuse(change->colour);
             }
         }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::changeGameEntityAlpha(const GameEntityManager::GameEntityAlphaChange* change)
    {
        if (change->gameEntity) {
            Ogre::v1::Entity* pEnt = 0;
            pEnt = static_cast<Ogre::v1::Entity*>(change->gameEntity->mMovableObject);
            if (pEnt) {
                auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(pEnt->getSubEntity(0)->getDatablock());
                if (datablock) {
                    datablock->setTransparency(change->alpha, Ogre::HlmsPbsDatablock::Transparent, true);
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void GraphicsSystem::changeGameEntityVisibility(const GameEntityManager::GameEntityVisibilityChange* change) 
    {
        if (change->gameEntity) {
            if (change->gameEntity->mSceneNode) {
                change->gameEntity->mSceneNode->setVisible(change->visible);
            }
        }
    }
}
