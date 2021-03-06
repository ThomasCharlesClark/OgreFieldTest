
#ifndef _Demo_GameEntity_H_
#define _Demo_GameEntity_H_

#include "OgreVector3.h"
#include "OgreQuaternion.h"
#include "OgreStringVector.h"
#include "OgreSceneManager.h"

#include <vector>

namespace MyThirdOgre
{
    #define NUM_GAME_ENTITY_BUFFERS 4

    enum MovableObjectType
    {
        MoTypeItem,
        MoTypeEntity,
        NumMovableObjectType,
        MoTypeStaticManualLineList,
        MoTypeDynamicManualLineList,
        MoTypeDynamicTriangleList,
        MoTypeDynamicManualObject,
        MoTypePrefabPlane,
        MoTypeCamera,
        MoTypeFieldComputeSystem
    };

    struct MovableObjectDefinition
    {
        Ogre::String        meshName;
        Ogre::String        resourceGroup;
        Ogre::StringVector  submeshMaterials;
        MovableObjectType   moType;
    };

    struct GameEntityTransform
    {
        Ogre::Vector3       vPos;
        Ogre::Quaternion    qRot;
        Ogre::Vector3       vScale;
    };

    struct ManualObjectLineListDefinition
    {
        std::vector<Ogre::Vector3> points;
        std::vector<std::pair<int, int>> lines;
    };

    class GameEntity
    {
    private:
        Ogre::uint32 mId;

    public:
        //----------------------------------------
        // Only used by Graphics thread
        //----------------------------------------
        float                   mTransparency;
        Ogre::SceneNode         *mSceneNode;
        Ogre::MovableObject*    mMovableObject; //Could be Entity, InstancedEntity, Item.

        Ogre::String                    mManualObjectDatablockName;
        Ogre::SceneManager::PrefabType  mPrefabType;
        ManualObjectLineListDefinition  mManualObjectDefinition;
        Ogre::TextureGpu*               mTextureGpu;

        //Your custom pointers go here, i.e. physics representation.
        //used only by Logic thread (hkpEntity, btRigidBody, etc)

        //----------------------------------------
        // Used by both Logic and Graphics threads
        //----------------------------------------
        GameEntityTransform     *mTransform[NUM_GAME_ENTITY_BUFFERS];
        Ogre::SceneMemoryMgrTypes       mType;

        //----------------------------------------
        // Read-only
        //----------------------------------------
        MovableObjectDefinition const   *mMoDefinition;
        size_t                   mTransformBufferIdx;

        GameEntity( Ogre::uint32 id, const MovableObjectDefinition *moDefinition,
                    Ogre::SceneMemoryMgrTypes type ) :
            mId( id ),
            mSceneNode( 0 ),
            mMovableObject( 0 ),
            mType( type ),
            mMoDefinition( moDefinition ),
            mTransformBufferIdx( 0 ),
            mTransparency( 0 ),
            mTextureGpu( 0 )
        {
            for( int i=0; i<NUM_GAME_ENTITY_BUFFERS; ++i )
                mTransform[i] = 0;
        }
        virtual ~GameEntity() 
        {

        }

        Ogre::uint32 getId(void) const          { return mId; }

        bool operator < ( const GameEntity *_r ) const
        {
            return mId < _r->mId;
        }

        static bool OrderById( const GameEntity *_l, const GameEntity *_r )
        {
            return _l->mId < _r->mId;
        }
    };

    typedef std::vector<GameEntity*> GameEntityVec;
}

#endif
