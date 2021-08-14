#include <sstream>

#include "Cell.h"

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
#include "MyMath.h"

#include "OgreEntity.h"

namespace MyThirdOgre 
{
    Cell::Cell(
        float scale,
        int rowIndex,
        int columnIndex,
        int layerIndex,
        int rowCount,
        int columnCount,
        float maxPressure,
        float maxVelocitySquared,
        bool velocityArrowVisible,
        bool pressureGradientArrowVisible,
        float maxInk,
        GameEntityManager* geMgr
    ) :
        mScale(scale),
        mRowCount(rowCount),
        mColumnCount(columnCount),
        mBoundary(rowIndex == 0 || columnIndex == 0 || rowIndex == rowCount - 1 || columnIndex == columnCount - 1),
        mVelocityArrowEntity(0),
        mVelocityArrowMoDef(0),
        mPressureGradientArrowEntity(0),
        mPressureGradientArrowMoDef(0),
        mVelocityArrowVisible(velocityArrowVisible),
        mPressureGradientArrowVisible(pressureGradientArrowVisible),
        mPlaneEntity(0),
        mPlaneMoDef(0),
        mSphereEntity(0),
        mSphereMoDef(0),
        mSphere(0),
        mMaxVelocitySquared(maxVelocitySquared),
        mMaxPressure(maxPressure),
        mGameEntityManager(geMgr)
        // interesting pressure - based surfaces:
        // (0.2 * ((rowIndex * rowIndex) - (2 * (rowIndex * columnIndex))) + 3)
        //pow(2.71828, -((rowIndex*rowIndex) + (columnIndex * columnIndex)))
    {
        float adjustmentX = -(mColumnCount / 2);
        float adjustmentZ = -(mRowCount / 2);

        mCellCoords = CellCoord(adjustmentX + columnIndex, layerIndex, adjustmentZ + rowIndex);

        // Set state. Can no longer do this in the default parameter constructor list, because of dependence on mCellCoord
        mState =
        {
            mState.bIsBoundary = mBoundary,
            mState.vPos = Ogre::Vector3(mCellCoords.mIndexX, 0, mCellCoords.mIndexZ) * mScale,
            mState.vVel = Ogre::Vector3::ZERO,
            mState.rPressure = 0.0f, //0.5 * (mCellCoords.mIndexX + mCellCoords.mIndexZ),//  4x^2 + y - 6
            mState.vPressureGradient = Ogre::Vector3::ZERO,
            mState.rDivergence = 0,
            mState.qRot = Ogre::Quaternion::IDENTITY,
            mState.bActive = false,
            mState.rVorticity = 0,
            mState.rInk = 0.0f,//mCellCoords.mIndexX < (adjustmentX + 3) ? 1 : 0,// (float)1 / (float)(mCellCoords.mIndexX - 10),
            mState.rMaxInk = maxInk,
            mState.vInkColour = Ogre::Vector3(0.968, 0.529, 0.094)
            //mState.vInkColour = Ogre::Vector3(1.0f, 0.0f, 0.0f)
        };

        mOriginalState = CellState(mState);

        createVelocityArrow();

        if (!mBoundary)
            createPressureGradientArrow();

        createPressureIndicator();

        createBoundingSphere();
        //createBoundingSphereDisplay();

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

        if (mSphere) {
            delete mSphere;
            mSphere = 0;
        }
    }
    
    void Cell::resetState(void) {
        mState.vPos = mOriginalState.vPos;
        mState.vVel = mOriginalState.vVel;
        mState.qRot = mOriginalState.qRot;
        mState.rPressure = mOriginalState.rPressure;
        mState.rInk = mOriginalState.rInk;
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

        Ogre::String name = "vA_";

        name += Ogre::StringConverter::toString(mCellCoords.mIndexX);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexY);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexZ);

        mVelocityArrowEntity = mGameEntityManager->addGameEntity(
            name,
            Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mVelocityArrowMoDef,
            mBoundary ? "UnlitRed" : "UnlitWhite",
            arrowLineList,
            Ogre::Vector3(mState.vPos.x, 0.01f, mState.vPos.z),
            mState.qRot,
            Ogre::Vector3::UNIT_SCALE * mScale,
            false,
            1.0f, 
            mVelocityArrowVisible
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

        Ogre::String name = "pGA_";

        name += Ogre::StringConverter::toString(mCellCoords.mIndexX);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexY);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexZ);

        mPressureGradientArrowEntity = mGameEntityManager->addGameEntity(
            name,
            Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mPressureGradientArrowMoDef,
            "UnlitGreen",
            arrowLineList,
            Ogre::Vector3(mState.vPos.x, 0.02f, mState.vPos.z),
            Ogre::Quaternion(1, 0, 0, 0),
            Ogre::Vector3(0.5f, 0.0f, 0.5f) * mScale,
            false,
            1.0f,
            mPressureGradientArrowVisible
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

        Ogre::String name = "pIP_";

        name += Ogre::StringConverter::toString(mCellCoords.mIndexX);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexY);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexZ);

        mPlaneEntity = mGameEntityManager->addGameEntity(
            name,
            Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mPlaneMoDef,
            Ogre::SceneManager::PrefabType::PT_PLANE,
            mBoundary ? "Red" : "White", //"Blue",
            mState.vPos,
            pRot,
            Ogre::Vector3(0.005f, 0.005f, 0.005f) * mScale,
            true,
            mBoundary ? 0.4 : mState.rInk,
            true,
            mBoundary ? Ogre::Vector3::ZERO : mState.vInkColour);
    }

    void Cell::createBoundingSphere(void)
    {
        mSphere = new Ogre::Sphere(mState.vPos, 1.0f);
    }

    void Cell::createBoundingSphereDisplay(void)
    {
        mSphereMoDef = new MovableObjectDefinition();
        mSphereMoDef->meshName = "Sphere1000.mesh";
        mSphereMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mSphereMoDef->moType = MoTypeItem;

        Ogre::String name = "sph_";

        name += Ogre::StringConverter::toString(mCellCoords.mIndexX);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexY);
        name += "_";
        name += Ogre::StringConverter::toString(mCellCoords.mIndexZ);

        mSphereEntity = mGameEntityManager->addGameEntity(
            name,
            Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
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
    }

    void Cell::setInkColour(Ogre::Vector3 v)
    {
        mState.vInkColour = v;
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

            if (isnan(vVelLen) || isinf(vVelLen))
                vVelLen = 0;

            mVelocityArrowEntity->mTransform[currIdx]->vScale.x = vVelLen;
            mVelocityArrowEntity->mTransform[currIdx]->vScale.y = 0;
            mVelocityArrowEntity->mTransform[currIdx]->vScale.z = vVelLen;

            //mPlaneEntity->mTransform[currIdx]->vPos.y = mState.rPressure;
            //mPlaneEntity->mTransform[currIdx]->vPos.y = 1 / (mState.rMaxInk / (mState.rInk == 0 ? 1 : mState.rInk));
            
            q.normalise();

            if (isnan(q.w) && isnan(q.x) && isnan(q.y) && isnan(q.z))
                q = Ogre::Quaternion::IDENTITY;

            mVelocityArrowEntity->mTransform[currIdx]->qRot = q;
        }

        if (mPressureGradientArrowEntity) {

            auto qNew = mPressureGradientArrowEntity->mTransform[prevIdx]->qRot;

            auto q = GetRotation(Ogre::Vector3(0, 0, -1), mState.vPressureGradient, Ogre::Vector3::UNIT_Y);

            auto vPressureGradientLen = mState.vPressureGradient.length();

            if (isnan(vPressureGradientLen) || isinf(vPressureGradientLen))
                vPressureGradientLen = 0;

            mPressureGradientArrowEntity->mTransform[currIdx]->vScale.x = vPressureGradientLen;
            mPressureGradientArrowEntity->mTransform[currIdx]->vScale.y = 0;
            mPressureGradientArrowEntity->mTransform[currIdx]->vScale.z = vPressureGradientLen;

            q.normalise();

            if (isnan(q.w) && isnan(q.x) && isnan(q.y) && isnan(q.z))
                q = Ogre::Quaternion::IDENTITY;

            mPressureGradientArrowEntity->mTransform[currIdx]->qRot = q;
        }

        updatePlaneEntity();
    }

    void Cell::updatePlaneEntity(void) {
        if (mPlaneMoDef) {
            if (mState.bIsBoundary) {
                //mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, mState.rPressure / mMaxPressure);
            }
            else {
                //if (mState.bActive) {
                //}
                //else {


                    //mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, mState.rPressure / mMaxPressure);

                // 

                //mState.vInkColour = Ogre::Math::lerp(Ogre::Vector3(0.968, 0.529, 0.094), Ogre::Vector3(0.988, 0.631, 0.631), (mState.rMaxInk / mState.rInk == 0 ? 1 : mState.rInk));

                //if (mState.rInk > mState.rMaxInk / 2) {
                //    //mState.vInkColour = Ogre::Vector3(1.0f, 0.0f, 0.0f);
                //    mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, ((mState.rMaxInk) / mState.rInk == 0 ? 1 : mState.rInk / (mState.rMaxInk / 2)));
                //}
                //else {
                    //mState.vInkColour = Ogre::Vector3(0.968, 0.529, 0.094);
                    mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, (mState.rMaxInk / mState.rInk == 0 ? 1 : mState.rInk / 2));
                //}

                //mGameEntityManager->gameEntityColourChange(mPlaneEntity, mState.vInkColour);



                //}
            }
        }
        //if (mPlaneMoDef) {
        //    //mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, mState.rInk);
        //    //mGameEntityManager->gameEntityColourChange(mPlaneEntity, mState.vInkColour);
        //}
    }

    Ogre::Real Cell::getPressure()
    {
        return mState.rPressure;
    }

    void Cell::setPressure(Ogre::Real p)
    {
        mState.rPressure = p;

        //if (mState.rPressure == 0) {
        //    mGameEntityManager->toggleGameEntityVisibility(mVelocityArrowEntity, false);
        //}
        //else {
        //    mGameEntityManager->toggleGameEntityVisibility(mVelocityArrowEntity, true);
        //}
    }

    void Cell::setActive() 
    {
        mState.bActive = true;
        //mState.rPressure = 0.4f;
        mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, 0.4f);
        mGameEntityManager->gameEntityColourChange(mPlaneEntity, Ogre::Vector3(50.0f, 50.0f, 0.0f));
    }

    void Cell::unsetActive() {
        mState.bActive = false;
        //mState.rPressure = abs(mState.vVel.squaredLength());
        //mGameEntityManager->gameEntityColourChange(mPlaneEntity, Ogre::Vector3(1, 1, 1));
        mGameEntityManager->gameEntityColourChange(mPlaneEntity, mState.vInkColour);
    }
}
