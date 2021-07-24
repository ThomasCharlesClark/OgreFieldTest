#include <sstream>

#include "Headers/Cell.h"

#include "OgreSceneManager.h"
#include "OgreManualObject2.h"

#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"

#include "OgreItem.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsSamplerblock.h"

#include "OgreMaterial.h"
#include "OgrePass.h"
#include "Headers/MyMath.h"

#include "OgreEntity.h"

namespace MyThirdOgre 
{
    Cell::Cell(
        int rowIndex,
        int columnIndex,
        int layerIndex,
        int rowCount,
        int columnCount,
        float maxPressure,
        GameEntityManager* geMgr
    ) :
        mCellCoords(rowIndex, layerIndex, columnIndex),
        mRowCount(rowCount),
        mColumnCount(columnCount),
        mBoundary(rowIndex == 0 || columnIndex == 0 || rowIndex == rowCount - 1 || columnIndex == columnCount - 1),
        mVelocityArrowEntity(0),
        mVelocityArrowMoDef(0),
        mPressureGradientArrowEntity(0),
        mPressureGradientArrowMoDef(0),
        mPlaneEntity(0),
        mPlaneMoDef(0),
        mSphereEntity(0),
        mSphereMoDef(0),
        mSphere(0),
        mMaxPressure(maxPressure),
        mGameEntityManager(geMgr)
        // interesting pressure - based surfaces:
        // (0.2 * ((rowIndex * rowIndex) - (2 * (rowIndex * columnIndex))) + 3)
        //pow(2.71828, -((rowIndex*rowIndex) + (columnIndex * columnIndex)))
    {
        // Set state. Can no longer do this in the default parameter constructor list, because of dependence on mCellCoord
        mState =
        {
            mState.bIsBoundary = mBoundary,
            mState.vPos = Ogre::Vector3(mCellCoords.mIndexX, 0, -mCellCoords.mIndexZ),
            mState.vVel = mBoundary ? Ogre::Vector3::ZERO :
                                      Ogre::Vector3::ZERO,

                                        //Ogre::Vector3(Ogre::Math::RangeRandom(-1.5f, 1.5f), 0.0, Ogre::Math::RangeRandom(-1.5f, 1.5f)),
 
                                        //Ogre::Vector3(mCellCoords.mIndexX, 0, -mCellCoords.mIndexZ),

            mState.vPressureGradient = Ogre::Vector3::ZERO,
            mState.qRot = Ogre::Quaternion::IDENTITY,
            mState.rPressure = 0.0f,// Ogre::Math::RangeRandom(0.001f, 1.005f),
            mState.bActive = false
        };

        mOriginalState = CellState(mState);

        //createVelocityArrow();

        if (!mBoundary)
            createPressureGradientArrow();

        createPressureIndicator();

        //createBoundingSphere(geMgr);

        // Quaternion Cheatsheet:
        // Take the degrees you want to rotate e.g. 135°
        // halve it = 67.5°
        // convert it to radians = 1 degree is equivalent to (π/180) radians
        // therefore multiply by (π/180)
        // == 0.3827rad
        // for W, cosine it
        // for x,y,z, sine it
    }

    Cell::~Cell()
    {
        // mArrowEntity and mPlaneEntity are now in the hands of the SceneManager. They are created on the render thread,
        // and the graphics system will dispose of them when appropriate.

        if (mVelocityArrowMoDef) {
            delete mVelocityArrowMoDef;
            mVelocityArrowMoDef = 0;
        }

        if (mPressureGradientArrowMoDef) {
            delete mPressureGradientArrowMoDef;
            mPressureGradientArrowMoDef = 0;
        }

        if (mPlaneMoDef) {
            delete mPlaneMoDef;
            mPlaneMoDef = 0;
        }

        if (mSphereMoDef) {
            delete mSphereMoDef;
            mSphereMoDef = 0;
        }
    }
    
    void Cell::resetState(void) {
        mState.vPos = mOriginalState.vPos;
        mState.vVel = mOriginalState.vVel;
        mState.qRot = mOriginalState.qRot;
        mState.rPressure = mOriginalState.rPressure;
    }

    void Cell::createVelocityArrow(void) 
    {
        mVelocityArrowMoDef = new MovableObjectDefinition();
        mVelocityArrowMoDef->meshName = "";
        mVelocityArrowMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mVelocityArrowMoDef->moType = MoTypeDynamicManualLineList;

        ManualObjectLineListDefinition arrowLineList;

        arrowLineList.points = std::vector<Ogre::Vector3>
        {
            Ogre::Vector3(0.0f, 0.0f, -0.25f),
            Ogre::Vector3(0.0f, 0.f, 0.25f),
            Ogre::Vector3(0.0f, 0.0f, 0.25f),
            Ogre::Vector3(0.2f, 0.0f, 0.0f),
            Ogre::Vector3(-0.2f, 0.0f, 0.0f)
        };

        arrowLineList.lines = std::vector<std::pair<int, int>>
        {
            { 0, 1 },
            { 2, 3 },
            { 1, 4 }
        };

        if (mBoundary) {
            //outer edges:
            if (mCellCoords.mIndexX == 0) { // x == 0 edge
                mState.qRot = Ogre::Quaternion(0.5253, 0.0, 0.5253, 0.0);
            } else if (mCellCoords.mIndexX == mRowCount - 1) { // opposite x edge
                mState.qRot = Ogre::Quaternion(0.5253, 0.0, -0.5253, 0.0);
            } else if (mCellCoords.mIndexZ == 0) {
                mState.qRot = Ogre::Quaternion(0.0, 0.0, 1.0, 0.0);
            } else if (mCellCoords.mIndexZ == mColumnCount - 1) { // opposite z edge
                mState.qRot = Ogre::Quaternion(1.0, 0.0, 0.0, 0.0);
            }

            //corners:  // x == 0, z == 0
            if (mCellCoords.mIndexX == 0 && mCellCoords.mIndexZ == 0) {
                mState.qRot = Ogre::Quaternion(0.3827, 0.0, 0.9239, 0.0);
            }           // x == max, z == 0
            else if (mCellCoords.mIndexX == mRowCount - 1 && mCellCoords.mIndexZ == 0) { 
                mState.qRot = Ogre::Quaternion(-0.3827, 0.0, 0.9239, 0.0);
            }           // x == 0, z == max
            else if (mCellCoords.mIndexX == 0 && mCellCoords.mIndexZ == mColumnCount - 1) { 
                mState.qRot = Ogre::Quaternion(-0.9239, 0.0, -0.3827, 0.0);
            }           // x == max, z == max
            else if (mCellCoords.mIndexX == mRowCount - 1 && mCellCoords.mIndexZ == mColumnCount - 1) { 
                mState.qRot = Ogre::Quaternion(0.9239, 0.0, -0.3827, 0.0);
            }
        }

        mVelocityArrowEntity = mGameEntityManager->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mVelocityArrowMoDef,
            mBoundary ? "UnlitRed" : "UnlitWhite",
            arrowLineList,
            Ogre::Vector3(mState.vPos.x, 0.01f, mState.vPos.z),
            mState.qRot,
            mState.rPressure == 0 ? Ogre::Vector3::ZERO : Ogre::Vector3::UNIT_SCALE
        );
    }

    void Cell::createPressureGradientArrow(void)
    {
        mPressureGradientArrowMoDef = new MovableObjectDefinition();
        mPressureGradientArrowMoDef->meshName = "";
        mPressureGradientArrowMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mPressureGradientArrowMoDef->moType = MoTypeDynamicManualLineList;

        ManualObjectLineListDefinition arrowLineList;

        arrowLineList.points = std::vector<Ogre::Vector3>
        {
            Ogre::Vector3(0.0f, 0.0f, -0.25f),
            Ogre::Vector3(0.0f, 0.f, 0.25f),
            Ogre::Vector3(0.0f, 0.0f, 0.25f),
            Ogre::Vector3(0.2f, 0.0f, 0.0f),
            Ogre::Vector3(-0.2f, 0.0f, 0.0f)
        };

        arrowLineList.lines = std::vector<std::pair<int, int>>
        {
            { 0, 1 },
            { 2, 3 },
            { 1, 4 }
        };

        mPressureGradientArrowEntity = mGameEntityManager->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mPressureGradientArrowMoDef,
            "UnlitGreen",
            arrowLineList,
            Ogre::Vector3(mState.vPos.x, 0.02f, mState.vPos.z),
            Ogre::Quaternion(1, 0, 0, 0),
            Ogre::Vector3(0.5f, 0.0f, 0.5f)
        );
    }

    void Cell::createPressureIndicator(void)
    {
        mPlaneMoDef = new MovableObjectDefinition();
        mPlaneMoDef->meshName = "";
        mPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mPlaneMoDef->moType = MoTypePrefabPlane;

        Ogre::Quaternion pRot = Ogre::Quaternion::IDENTITY;

        pRot.FromAngleAxis(Ogre::Radian(Ogre::Degree(-90)), Ogre::Vector3::UNIT_X);

        mPlaneEntity = mGameEntityManager->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_STATIC,
            mPlaneMoDef,
            Ogre::SceneManager::PrefabType::PT_PLANE,
            mBoundary ? "Red" : "Blue",
            mState.vPos,
            pRot,
            Ogre::Vector3(0.005f, 0.005f, 0.005f),
            true,
            mBoundary ? 0.4f : mState.rPressure);
    }

    void Cell::createBoundingSphere(void)
    {
        mSphere = new Ogre::Sphere(mState.vPos, 1.0f);

        mSphereMoDef = new MovableObjectDefinition();
        mSphereMoDef->meshName = "Sphere1000.mesh";
        mSphereMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mSphereMoDef->moType = MoTypeItem;

        mSphereEntity = mGameEntityManager->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mSphereMoDef,
            mState.vPos,
            mState.qRot,
            Ogre::Vector3::UNIT_SCALE,
            true,
            0.2f);
    }

    void Cell::setVelocity(Ogre::Vector3 v)
    {
        mState.vVel = v;

		//if (!mState.bActive) {
		//	mState.rPressure = abs(v.squaredLength());
		//	if (mState.rPressure > mMaxPressure)
		//		mState.rPressure = mMaxPressure;
		//	else if (mState.rPressure < 0)
		//		mState.rPressure = 0;
		//}

        //if (mState.vVel == Ogre::Vector3::ZERO)
        //    mState.vVel = Ogre::Vector3(Ogre::Math::RangeRandom(-0.2f, 0.2f), 0.0f, Ogre::Math::RangeRandom(-0.2f, 0.2f));

    }

    void Cell::setPressureGradient(Ogre::Vector3 v)
    {
        mState.vPressureGradient = v;
    }

    // Updates our transform buffer
    void Cell::updateTransforms(float timeSinceLastFrame, Ogre::uint32 currIdx, Ogre::uint32 prevIdx)
    {
        if (mVelocityArrowEntity) {
            auto qNew = mVelocityArrowEntity->mTransform[prevIdx]->qRot;

            auto q = GetRotation(Ogre::Vector3(0, 0, 1), mState.vVel.normalisedCopy(), Ogre::Vector3::UNIT_Y);

            auto vVelLen = mState.vVel.length();

            mVelocityArrowEntity->mTransform[currIdx]->vScale.x = vVelLen;
            mVelocityArrowEntity->mTransform[currIdx]->vScale.y = 0;
            mVelocityArrowEntity->mTransform[currIdx]->vScale.z = vVelLen;

            mVelocityArrowEntity->mTransform[currIdx]->qRot = q;
        }

        if (mPressureGradientArrowEntity) {

            auto qNew = mPressureGradientArrowEntity->mTransform[prevIdx]->qRot;

            auto q = GetRotation(Ogre::Vector3(0, 0, -1), mState.vPressureGradient.normalisedCopy(), Ogre::Vector3::UNIT_Y);

            auto vPressureGradientLen = mState.vPressureGradient.length();

            mPressureGradientArrowEntity->mTransform[currIdx]->vScale.x = vPressureGradientLen;
            mPressureGradientArrowEntity->mTransform[currIdx]->vScale.y = 0;
            mPressureGradientArrowEntity->mTransform[currIdx]->vScale.z = vPressureGradientLen;

            mPressureGradientArrowEntity->mTransform[currIdx]->qRot = q;
        }

        updatePressureIndicator();
    }

    void Cell::updatePressureIndicator(void) {
        if (mPlaneMoDef) {
            if (mState.bActive)
                mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, 0.4f);
            else
                mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, mState.rPressure / mMaxPressure);
        }
    }

    Ogre::Real Cell::getPressure()
    {
        return mState.rPressure;
    }

    void Cell::setPressure(Ogre::Real p)
    {
        mState.rPressure = p;
    }

    void Cell::setActive() 
    {
        mState.bActive = true;
        //mState.rPressure = 0.4f;
        mGameEntityManager->gameEntityColourChange(mPlaneEntity, Ogre::Vector3(50.0f, 50.0f, 0.0f));
    }

    void Cell::unsetActive() {
        mState.bActive = false;
        //mState.rPressure = abs(mState.vVel.squaredLength());
        mGameEntityManager->gameEntityColourChange(mPlaneEntity, Ogre::Vector3(1, 1, 1));
    }
}
