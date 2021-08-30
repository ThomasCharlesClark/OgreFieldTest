
#ifndef _Demo_GameEntityManager_H_
#define _Demo_GameEntityManager_H_

#include "Threading/MessageQueueSystem.h"
#include "GameEntity.h"
#include "OgreManualObject2.h"
#include "OgreSceneManager.h"

#include <string>

namespace MyThirdOgre
{
    class GraphicsSystem;
    class LogicSystem;
    struct ManualObjectLineListDefinition;
    struct FieldComputeSystem;

    class GameEntityManager
    {
    public:
        struct CreatedGameEntity
        {
            GameEntity          *gameEntity;
            GameEntityTransform initialTransform;
            bool                useAlpha;
            bool                visible;
            Ogre::String        name;
            Ogre::Vector3       vColour;
        };

        struct GameEntityColourChange 
        {
            GameEntity          *gameEntity;
            Ogre::Vector3       colour;

            GameEntityColourChange(GameEntity* ge, Ogre::Vector3 c) {
                gameEntity = ge;
                colour = c;
            };
        };

        struct GameEntityAlphaChange
        {
            GameEntity          *gameEntity;
            Ogre::Real          alpha;

            GameEntityAlphaChange(GameEntity* ge, Ogre::Real a) {
                gameEntity = ge;
                alpha = a;
            };
        };

        struct GameEntityVisibilityChange
        {
            GameEntity* gameEntity;
            bool        visible;

            GameEntityVisibilityChange(GameEntity* ge, bool v) {
                gameEntity = ge;
                visible = v;
            };
        };

        typedef std::vector<GameEntityVec> GameEntityVecVec;

    private:
        struct Region
        {
            size_t slotOffset;
            size_t count;
            size_t bufferIdx;

            Region( size_t _slotOffset, size_t _count, size_t _bufferIdx ) :
                slotOffset( _slotOffset ),
                count( _count ),
                bufferIdx( _bufferIdx )
            {
            }
        };

        //We assume mCurrentId never wraps
        Ogre::uint32    mCurrentId;
        GameEntityVec   mGameEntities[Ogre::NUM_SCENE_MEMORY_MANAGER_TYPES];

        std::vector<GameEntityTransform*>   mTransformBuffers;
        std::vector<Region>                 mAvailableTransforms;

        GameEntityVecVec    mScheduledForRemoval;
        size_t              mScheduledForRemovalCurrentSlot;
        std::vector<size_t> mScheduledForRemovalAvailableSlots;

        Ogre::uint32 getScheduledForRemovalAvailableSlot(void);
        void destroyAllGameEntitiesIn( GameEntityVec &container );

        void aquireTransformSlot( size_t &outSlot, size_t &outBufferIdx );
        void releaseTransformSlot( size_t bufferIdx, GameEntityTransform *transform );


    public:
        Mq::MessageQueueSystem* mGraphicsSystem;
        LogicSystem* mLogicSystem;

        GameEntityManager( Mq::MessageQueueSystem *graphicsSystem,
                           LogicSystem *logicSystem );
        ~GameEntityManager();

        /** Creates a GameEntity, adding it to the world, and scheduling for the Graphics
            thread to create the appropiate SceneNode and Item pointers.
            MUST BE CALLED FROM LOGIC THREAD.
        @param type
            Whether this GameEntity is dynamic (going to change transform frequently), or
            static (will move/rotate/scale very, very infrequently)
        @param initialPos
            Starting position of the GameEntity
        @param initialRot
            Starting orientation of the GameEntity
        @param initialScale
            Starting scale of the GameEntity
        @return
            Pointer of GameEntity ready to be used by the Logic thread. Take in mind
            not all of its pointers may filled yet (the ones that are not meant to
            be used by the logic thread)
        */
        GameEntity* addGameEntity( const Ogre::String& name,
                                   const Ogre::SceneMemoryMgrTypes type,
                                   const MovableObjectDefinition *moDefinition,
                                   const Ogre::Vector3 &initialPos,
                                   const Ogre::Quaternion &initialRot,
                                   const Ogre::Vector3 &initialScale,
                                   const bool useAlpha = false, 
                                   const float alpha = 1.0f,
                                   const bool visible = true );

        GameEntity* addGameEntity(  const Ogre::String& name,
                                    const Ogre::SceneMemoryMgrTypes type,
                                    const MovableObjectDefinition* moDefinition,
                                    const Ogre::SceneManager::PrefabType prefabType,
                                    const Ogre::String datablockName,
                                    const Ogre::Vector3& initialPos,
                                    const Ogre::Quaternion& initialRot,
                                    const Ogre::Vector3& initialScale,
                                    const bool useAlpha = false,
                                    const float alpha = 1.0f,
                                    const bool visible = true,
                                    const Ogre::Vector3 vColour = Ogre::Vector3::ZERO,
                                    Ogre::TextureGpu* mTex = NULL);

        GameEntity* addGameEntity(const Ogre::String& name,
            const Ogre::SceneMemoryMgrTypes type,
            const MovableObjectDefinition* moDefinition,
            const std::vector<Ogre::Vector3> vertexList,
            const Ogre::String datablockName,
            const Ogre::Vector3& initialPos,
            const Ogre::Quaternion& initialRot,
            const Ogre::Vector3& initialScale,
            const bool useAlpha = false,
            const float alpha = 1.0f,
            const bool visible = true,
            const Ogre::Vector3 vColour = Ogre::Vector3::ZERO,
            Ogre::TextureGpu* mTex = NULL);

        GameEntity* addGameEntity( const Ogre::String& name,
                                   const Ogre::SceneMemoryMgrTypes type,
                                   const MovableObjectDefinition* moDefinition,
                                   const Ogre::String& datablockName,
                                   const ManualObjectLineListDefinition& manualObjectDef,
                                   const Ogre::Vector3& initialPos,
                                   const Ogre::Quaternion& initialRot,
                                   const Ogre::Vector3& initialScale,
                                   const bool useAlpha = false,
                                   const float alpha = 1.0f,
                                   const bool visible = true );

        FieldComputeSystem* addFieldComputeSystem(  const Ogre::String& name,
                                                    const Ogre::SceneMemoryMgrTypes type,
                                                    const MovableObjectDefinition* moDefinition,
                                                    const Ogre::String& computeJobName,
                                                    const Ogre::Vector3& initialPos,
                                                    const Ogre::Quaternion& initialRot,
                                                    const Ogre::Vector3& initialScale,
                                                    const bool useAlpha = false,
                                                    const float alpha = 1.0f,
                                                    const bool visible = true);

        /** Removes the GameEntity from the world. The pointer is not immediately destroyed,
            we first need to release data in other threads (i.e. Graphics).
            It will be destroyed after the Render thread confirms it is done with it
            (via a Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT message)
        */
		void removeGameEntity( GameEntity *toRemove );
        /// Must be called by LogicSystem when Mq::GAME_ENTITY_SCHEDULED_FOR_REMOVAL_SLOT message arrives
        void _notifyGameEntitiesRemoved( size_t slot );

        /// Must be called every frame from the LOGIC THREAD.
        void finishFrameParallel(void);

        virtual void gameEntityColourChange(GameEntity* entity, Ogre::Vector3 colour);

        virtual void gameEntityAlphaChange(GameEntity* entity, Ogre::Real alpha);
        
        virtual void toggleGameEntityVisibility(GameEntity* entity, bool visible);
    };
}

#endif
