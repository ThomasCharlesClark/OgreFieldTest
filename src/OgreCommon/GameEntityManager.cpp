
#include "GameEntityManager.h"
#include "GameEntity.h"

#include "LogicSystem.h"
#include "OgreManualObject2.h"
#include "FieldComputeSystem.h"

namespace MyThirdOgre
{
    const size_t cNumTransforms = 250;

    GameEntityManager::GameEntityManager( Mq::MessageQueueSystem *graphicsSystem,
                                          LogicSystem *logicSystem ) :
        mCurrentId( 0 ),
        mScheduledForRemovalCurrentSlot( (size_t)-1 ),
        mGraphicsSystem( graphicsSystem ),
        mLogicSystem( logicSystem )
    {
        mLogicSystem->_notifyGameEntityManager( this );
    }
    //-----------------------------------------------------------------------------------
    GameEntityManager::~GameEntityManager()
    {
        mLogicSystem->_notifyGameEntityManager( 0 );

        {
            GameEntityVecVec::iterator itor = mScheduledForRemoval.begin();
            GameEntityVecVec::iterator end  = mScheduledForRemoval.end();
            while( itor != end )
                destroyAllGameEntitiesIn( *itor++ );
            mScheduledForRemoval.clear();
            mScheduledForRemovalAvailableSlots.clear();
        }

        destroyAllGameEntitiesIn( mGameEntities[Ogre::SCENE_DYNAMIC] );
        destroyAllGameEntitiesIn( mGameEntities[Ogre::SCENE_STATIC] );

        std::vector<GameEntityTransform*>::const_iterator itor = mTransformBuffers.begin();
        std::vector<GameEntityTransform*>::const_iterator end  = mTransformBuffers.end();

        while( itor != end )
        {
            OGRE_FREE_SIMD( *itor, Ogre::MEMCATEGORY_SCENE_OBJECTS );
            ++itor;
        }

        mTransformBuffers.clear();
        mAvailableTransforms.clear();
    }
    //-----------------------------------------------------------------------------------
    GameEntity* GameEntityManager::addGameEntity( const Ogre::String& name, 
                                                  const Ogre::SceneMemoryMgrTypes type,
                                                  const MovableObjectDefinition *moDefinition,
                                                  const Ogre::Vector3 &initialPos,
                                                  const Ogre::Quaternion &initialRot,
                                                  const Ogre::Vector3 &initialScale,
                                                  const bool useAlpha, 
                                                  const float alpha,
                                                  const bool visible )
    {
        GameEntity *gameEntity = new GameEntity( mCurrentId++, moDefinition, type );
        gameEntity->mTransparency = alpha;

        CreatedGameEntity cge;
        cge.gameEntity  = gameEntity;
        cge.initialTransform.vPos   = initialPos;
        cge.initialTransform.qRot   = initialRot;
        cge.initialTransform.vScale = initialScale;
        cge.gameEntity->mTransparency = alpha;
        cge.useAlpha = useAlpha;
        cge.visible = visible;
        cge.name = name;

        size_t slot, bufferIdx;
        aquireTransformSlot( slot, bufferIdx );

        gameEntity->mTransformBufferIdx = bufferIdx;
        for( int i=0; i<NUM_GAME_ENTITY_BUFFERS; ++i )
        {
            gameEntity->mTransform[i] = mTransformBuffers[bufferIdx] + slot + cNumTransforms * i;
            memcpy( gameEntity->mTransform[i], &cge.initialTransform, sizeof(GameEntityTransform) );
        }

        mGameEntities[type].push_back( gameEntity );

        mLogicSystem->queueSendMessage( mGraphicsSystem, Mq::GAME_ENTITY_ADDED, cge );

        return gameEntity;
    }
    //-----------------------------------------------------------------------------------
    GameEntity* GameEntityManager::addGameEntity( const Ogre::String& name, 
                                                  const Ogre::SceneMemoryMgrTypes type,
                                                  const MovableObjectDefinition* moDefinition, 
                                                  const Ogre::SceneManager::PrefabType prefabType,
                                                  const Ogre::String datablockName,
                                                  const Ogre::Vector3& initialPos, 
                                                  const Ogre::Quaternion& initialRot,
                                                  const Ogre::Vector3& initialScale,
                                                  const bool useAlpha,
                                                  const float alpha,
                                                  const bool visible,
                                                  Ogre::Vector3 vColour,
                                                  Ogre::TextureGpu* mTex)
    {
        GameEntity* gameEntity = new GameEntity(mCurrentId++, moDefinition, type);
        gameEntity->mManualObjectDatablockName = datablockName;
        gameEntity->mPrefabType = prefabType;
        gameEntity->mTransparency = alpha;
        gameEntity->mTextureGpu = mTex;

        CreatedGameEntity cge;
        cge.gameEntity = gameEntity;
        cge.initialTransform.vPos = initialPos;
        cge.initialTransform.qRot = initialRot;
        cge.initialTransform.vScale = initialScale;
        cge.useAlpha = useAlpha;
        cge.visible = visible;
        cge.name = name;
        cge.vColour = vColour;

        size_t slot, bufferIdx;
        aquireTransformSlot(slot, bufferIdx);

        gameEntity->mTransformBufferIdx = bufferIdx;
        for (int i = 0; i < NUM_GAME_ENTITY_BUFFERS; ++i)
        {
            gameEntity->mTransform[i] = mTransformBuffers[bufferIdx] + slot + cNumTransforms * i;
            memcpy(gameEntity->mTransform[i], &cge.initialTransform, sizeof(GameEntityTransform));
        }

        mGameEntities[type].push_back(gameEntity);

        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_ADDED, cge);

        return gameEntity;
    }
    //-----------------------------------------------------------------------------------
    GameEntity* GameEntityManager::addGameEntity(const Ogre::String& name,
        const Ogre::SceneMemoryMgrTypes type,
        const MovableObjectDefinition* moDefinition,
        const std::vector<Ogre::Vector3> vertexList,
        const Ogre::String datablockName,
        const Ogre::Vector3& initialPos,
        const Ogre::Quaternion& initialRot,
        const Ogre::Vector3& initialScale,
        const bool useAlpha,
        const float alpha,
        const bool visible,
        Ogre::Vector3 vColour,
        Ogre::TextureGpu* mTex)
    {
        GameEntity* gameEntity = new GameEntity(mCurrentId++, moDefinition, type);
        gameEntity->mManualObjectDatablockName = datablockName;
        gameEntity->mManualObjectDefinition = ManualObjectLineListDefinition();
        gameEntity->mManualObjectDefinition.points = vertexList;
        gameEntity->mTransparency = alpha;
        gameEntity->mTextureGpu = mTex;

        CreatedGameEntity cge;
        cge.gameEntity = gameEntity;
        cge.initialTransform.vPos = initialPos;
        cge.initialTransform.qRot = initialRot;
        cge.initialTransform.vScale = initialScale;
        cge.useAlpha = useAlpha;
        cge.visible = visible;
        cge.name = name;
        cge.vColour = vColour;

        size_t slot, bufferIdx;
        aquireTransformSlot(slot, bufferIdx);

        gameEntity->mTransformBufferIdx = bufferIdx;
        for (int i = 0; i < NUM_GAME_ENTITY_BUFFERS; ++i)
        {
            gameEntity->mTransform[i] = mTransformBuffers[bufferIdx] + slot + cNumTransforms * i;
            memcpy(gameEntity->mTransform[i], &cge.initialTransform, sizeof(GameEntityTransform));
        }

        mGameEntities[type].push_back(gameEntity);

        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_ADDED, cge);

        return gameEntity;
    }
    //-----------------------------------------------------------------------------------
	GameEntity* GameEntityManager::addGameEntity( const Ogre::String& name,
                                                  const Ogre::SceneMemoryMgrTypes type, 
                                                  const MovableObjectDefinition* moDefinition,
                                                  const Ogre::String& datablockName,
                                                  const ManualObjectLineListDefinition& manualObjectLineListDef,
                                                  const Ogre::Vector3& initialPos, 
                                                  const Ogre::Quaternion& initialRot, 
                                                  const Ogre::Vector3& initialScale,
                                                  const bool useAlpha,
                                                  const float alpha,
                                                  const bool visible)
	{
        GameEntity* gameEntity = new GameEntity(mCurrentId++, moDefinition, type);
        gameEntity->mManualObjectDatablockName = datablockName;
        gameEntity->mManualObjectDefinition = manualObjectLineListDef;
        gameEntity->mTransparency = alpha;

        CreatedGameEntity cge;
        cge.gameEntity = gameEntity;
        cge.initialTransform.vPos = initialPos;
        cge.initialTransform.qRot = initialRot;
        cge.initialTransform.vScale = initialScale;
        cge.useAlpha = useAlpha;
        cge.visible = visible;
        cge.name = name;

        size_t slot, bufferIdx;
        aquireTransformSlot(slot, bufferIdx);

        gameEntity->mTransformBufferIdx = bufferIdx;
        for (int i = 0; i < NUM_GAME_ENTITY_BUFFERS; ++i)
        {
            gameEntity->mTransform[i] = mTransformBuffers[bufferIdx] + slot + cNumTransforms * i;
            memcpy(gameEntity->mTransform[i], &cge.initialTransform, sizeof(GameEntityTransform));
        }

        mGameEntities[type].push_back(gameEntity);

        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_ADDED, cge);

        return gameEntity;
	}
    //-----------------------------------------------------------------------------------
    FieldComputeSystem* GameEntityManager::addFieldComputeSystem(const Ogre::String& name,
        const Ogre::SceneMemoryMgrTypes type,
        const MovableObjectDefinition* moDefinition,
        const Ogre::String& computeJobName,
        const Ogre::Vector3& initialPos,
        const Ogre::Quaternion& initialRot,
        const Ogre::Vector3& initialScale,
        const int columnCount,
        const int rowCount,
        const bool velocityVisible,
        const bool useAlpha,
        const float alpha,
        const bool visible)
    {

        FieldComputeSystem* gameEntity = new FieldComputeSystem(
            mCurrentId++,
            moDefinition,
            type,
            this,
            columnCount,
            rowCount,
            velocityVisible
        );
        gameEntity->mTransparency = alpha;

        CreatedGameEntity cge;
        cge.gameEntity = gameEntity;
        cge.initialTransform.vPos = initialPos;
        cge.initialTransform.qRot = initialRot;
        cge.initialTransform.vScale = initialScale;
        cge.useAlpha = useAlpha;
        cge.visible = visible;
        cge.name = name;

        size_t slot, bufferIdx;
        aquireTransformSlot(slot, bufferIdx);

        gameEntity->mTransformBufferIdx = bufferIdx;
        for (int i = 0; i < NUM_GAME_ENTITY_BUFFERS; ++i)
        {
            gameEntity->mTransform[i] = mTransformBuffers[bufferIdx] + slot + cNumTransforms * i;
            memcpy(gameEntity->mTransform[i], &cge.initialTransform, sizeof(GameEntityTransform));
        }

        mGameEntities[type].push_back(gameEntity);

        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_ADDED, cge);

        return gameEntity;
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::removeGameEntity( GameEntity *toRemove )
    {
        Ogre::uint32 slot = getScheduledForRemovalAvailableSlot();
        mScheduledForRemoval[slot].push_back( toRemove );
        GameEntityVec::iterator itor = std::lower_bound( mGameEntities[toRemove->mType].begin(),
                                                         mGameEntities[toRemove->mType].end(),
                                                         toRemove, GameEntity::OrderById );
        assert( itor != mGameEntities[toRemove->mType].end() && *itor == toRemove );
        mGameEntities[toRemove->mType].erase( itor );
        mLogicSystem->queueSendMessage( mGraphicsSystem, Mq::GAME_ENTITY_REMOVED, toRemove );
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::_notifyGameEntitiesRemoved( size_t slot )
    {
        destroyAllGameEntitiesIn( mScheduledForRemoval[slot] );

        mScheduledForRemoval[slot].clear();
        mScheduledForRemovalAvailableSlots.push_back( slot );
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::destroyAllGameEntitiesIn( GameEntityVec &container )
    {
        GameEntityVec::const_iterator itor = container.begin();
        GameEntityVec::const_iterator end  = container.end();

        while( itor != end )
        {
            releaseTransformSlot( (*itor)->mTransformBufferIdx, (*itor)->mTransform[0] );
            delete *itor;
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::aquireTransformSlot( size_t &outSlot, size_t &outBufferIdx )
    {
        if( mAvailableTransforms.empty() )
        {
            GameEntityTransform *buffer = reinterpret_cast<GameEntityTransform*>( OGRE_MALLOC_SIMD(
                        sizeof(GameEntityTransform) * cNumTransforms * NUM_GAME_ENTITY_BUFFERS,
                        Ogre::MEMCATEGORY_SCENE_OBJECTS ) );
            mTransformBuffers.push_back( buffer );
            mAvailableTransforms.push_back( Region( 0, cNumTransforms, mTransformBuffers.size() - 1 ) );
        }

        Region &region = mAvailableTransforms.back();
        outSlot = region.slotOffset++;
        --region.count;
        outBufferIdx = region.bufferIdx;

        if( region.count == 0 )
            mAvailableTransforms.pop_back();
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::releaseTransformSlot( size_t bufferIdx, GameEntityTransform *transform )
    {
        //Try to prevent a lot of fragmentation by adding the slot to an existing region.
        //It won't fully avoid it, but this is good/simple enough. If you want to fully
        //prevent fragmentation, see StagingBuffer::mergeContiguousBlocks implementation.
        const size_t slot = transform - mTransformBuffers[bufferIdx];

        std::vector<Region>::iterator itor = mAvailableTransforms.begin();
        std::vector<Region>::iterator end  = mAvailableTransforms.end();

        while( itor != end )
        {
            if( itor->bufferIdx == bufferIdx &&
                ( itor->slotOffset == slot + 1 || slot == itor->slotOffset + itor->count ) )
            {
                break;
            }

            ++itor;
        }

        if( itor != end )
        {
            if( itor->slotOffset == slot + 1 )
                --itor->slotOffset;
            else //if( slot == itor->slot + itor->count )
                ++itor->count;
        }
        else
        {
            mAvailableTransforms.push_back( Region( slot, 1, bufferIdx ) );
        }
    }
    //-----------------------------------------------------------------------------------
    Ogre::uint32 GameEntityManager::getScheduledForRemovalAvailableSlot(void)
    {
        if( mScheduledForRemovalCurrentSlot >= mScheduledForRemoval.size() )
        {
            if( mScheduledForRemovalAvailableSlots.empty() )
            {
                mScheduledForRemovalAvailableSlots.push_back( mScheduledForRemoval.size() );
                mScheduledForRemoval.push_back( GameEntityVec() );
            }

            mScheduledForRemovalCurrentSlot = mScheduledForRemovalAvailableSlots.back();
            mScheduledForRemovalAvailableSlots.pop_back();
        }

        return mScheduledForRemovalCurrentSlot;
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::finishFrameParallel(void)
    {
        if( mScheduledForRemovalCurrentSlot < mScheduledForRemoval.size() )
        {
            mLogicSystem->queueSendMessage( mGraphicsSystem, Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT,
                                            mScheduledForRemovalCurrentSlot );

            mScheduledForRemovalCurrentSlot = (size_t)-1;
        }
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::gameEntityColourChange(GameEntity* entity, Ogre::Vector3 colour) {
        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_COLOUR_CHANGE, 
            GameEntityColourChange(entity, colour));
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::gameEntityAlphaChange(GameEntity* entity, Ogre::Real alpha) {
        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_ALPHA_CHANGE,
            GameEntityAlphaChange(entity, alpha));
    }
    //-----------------------------------------------------------------------------------
    void GameEntityManager::toggleGameEntityVisibility(GameEntity* entity, bool visible) {
        mLogicSystem->queueSendMessage(mGraphicsSystem, Mq::GAME_ENTITY_VISIBILITY_CHANGE, GameEntityVisibilityChange(entity, visible));
    }
}
