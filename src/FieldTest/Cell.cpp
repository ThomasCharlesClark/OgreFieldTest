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
        mArrowEntity(0),
        mArrowMoDef(0),
        mPlaneEntity(0),
        mPlaneMoDef(0),
        mSphereEntity(0),
        mSphereMoDef(0),
        mSphere(0),
        mMaxPressure(maxPressure),
        mGameEntityManager(geMgr),
        mActive(false)
        // interesting pressure - based surfaces:
        // (0.2 * ((rowIndex * rowIndex) - (2 * (rowIndex * columnIndex))) + 3)
        //pow(2.71828, -((rowIndex*rowIndex) + (columnIndex * columnIndex)))
    {
        // Set state. Can no longer do this in the default parameter constructor list, because of mCellCoord
        mState = 
        {
            mState.vPos = Ogre::Vector3(mCellCoords.mIndexX, 0, -mCellCoords.mIndexZ),
            mState.vVel = mBoundary ? Ogre::Vector3::ZERO :
                                      Ogre::Vector3(mCellCoords.mIndexX, mCellCoords.mIndexY, -mCellCoords.mIndexZ),
                                      //Ogre::Vector3(mCellCoords.mIndexX, 0, -mCellCoords.mIndexZ),
            mState.qRot = Ogre::Quaternion::IDENTITY,
            mState.rPressure = 0.5f 
        };

        mOriginalState = CellState(mState);

        createArrow();

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

        if (mArrowMoDef) {
            delete mArrowMoDef;
            mArrowMoDef = 0;
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
    
    // well why the fuck does this cause a huge FPS drop?!
    void Cell::resetState(void) {
        mState.vPos = mOriginalState.vPos;
        mState.vVel = mOriginalState.vVel;
        mState.qRot = mOriginalState.qRot;
        mState.rPressure = mOriginalState.rPressure;
    }

    void Cell::createArrow(void) 
    {
        mArrowMoDef = new MovableObjectDefinition();
        mArrowMoDef->meshName = "";
        mArrowMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mArrowMoDef->moType = MoTypeDynamicManualLineList;

        ManualObjectLineListDefinition arrowLineList;

        arrowLineList.points = std::vector<Ogre::Vector3>
        {
            Ogre::Vector3(0.0f, 0.0f, 0.25f),
            Ogre::Vector3(0.0f, 0.f, -0.25f),
            Ogre::Vector3(0.0f, 0.0f, -0.25f),
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
                mState.qRot = Ogre::Quaternion(0.5253, 0.0, -0.5253, 0.0);
            } else if (mCellCoords.mIndexX == mRowCount - 1) { // opposite x edge
                mState.qRot = Ogre::Quaternion(0.5253, 0.0, 0.5253, 0.0);
            } else if (mCellCoords.mIndexZ == 0) {
                mState.qRot = Ogre::Quaternion(1.0, 0.0, 0.0, 0.0);
            } else if (mCellCoords.mIndexZ == mColumnCount - 1) { // opposite z edge
                mState.qRot = Ogre::Quaternion(0.0, 0.0, 1.0, 0.0);
            }

            //corners:
            if (mCellCoords.mIndexX == 0 && mCellCoords.mIndexZ == 0) { // x == 0, y == 0
                mState.qRot = Ogre::Quaternion(0.9239, 0.0, -0.3827, 0.0);
            } else if (mCellCoords.mIndexX == mRowCount - 1 && mCellCoords.mIndexZ == 0) { // x == max, y == 0
                mState.qRot = Ogre::Quaternion(0.9239, 0.0, 0.3827, 0.0);
            } else if (mCellCoords.mIndexX == 0 && mCellCoords.mIndexZ == mColumnCount - 1) { // x == 0, y == max
                mState.qRot = Ogre::Quaternion(-0.3827, 0.0, 0.9239, 0.0);
            } else if (mCellCoords.mIndexX == mRowCount - 1 && mCellCoords.mIndexZ == mColumnCount - 1) { // x == max, y == max
                mState.qRot = Ogre::Quaternion(0.3827, 0.0, 0.9239, 0.0);
            }
        }
        else {
            Ogre::Quaternion q;

            Ogre::Vector3 veln = mState.vVel.normalisedCopy();
            Ogre::Vector3 a = veln.crossProduct(Ogre::Vector3(0, 1, 0));

            a.normalise();

            q.x = a.x;
            q.z = a.y;
            q.y = a.z;

            q.w = sqrt((pow(veln.length(), 2) * (pow(veln.length(), 2)))) + veln.dotProduct(a);
            q.normalise();
        }

        mArrowEntity = mGameEntityManager->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mArrowMoDef,
            mBoundary ? "UnlitRed" : "UnlitWhite",
            arrowLineList,
            Ogre::Vector3(mState.vPos.x, 0.01f, mState.vPos.z),
            mState.qRot,
            Ogre::Vector3::UNIT_SCALE
            //Ogre::Vector3(1.0f, 0.0f, mState.vVel.length())
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
            //Ogre::Vector3(mIndexX, mState.rPressure * 0.5f, -mIndexZ),
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

    CellState Cell::getState(void) 
    {
        return mState;
    }

    Ogre::Vector3 Cell::getVelocity()
    {
        return mState.vVel;
    }

    void Cell::setVelocity(Ogre::Vector3 v) 
    {
        mState.vVel = v;
        if (!mActive) {
            mState.rPressure = abs(v.squaredLength());
            if (mState.rPressure > mMaxPressure)
                mState.rPressure = mMaxPressure;
            else if (mState.rPressure < 0)
                mState.rPressure = 0;
        }
    }

    // Updates our transform buffer
    void Cell::updateTransforms(float timeSinceLastFrame, Ogre::uint32 currIdx, Ogre::uint32 prevIdx)
    {
        auto qNew = mArrowEntity->mTransform[prevIdx]->qRot;

        auto q = GetRotation(Ogre::Vector3(0, 0, 1), mState.vVel.normalisedCopy(), Ogre::Vector3::UNIT_Y);

        //mArrowEntity->mTransform[currIdx]->vScale.z = mState.vVel.length();
        mArrowEntity->mTransform[currIdx]->qRot = q;

        updatePressureIndicator();
    }

    void Cell::updatePressureIndicator(void) {
        mGameEntityManager->gameEntityAlphaChange(mPlaneEntity, mState.rPressure / mMaxPressure);
    }

    //
    //void Cell::setVelocity(Ogre::Vector3 v, float timeSinceLast)
    //{
    //    //arrowNode->scale(0.5f, v.length(), 0.5f);
    //
    //    velocity.y = 0;
    //    v.y = 0;
    //
    //    Ogre::Quaternion q;
    //
    //    auto vn = v.normalisedCopy();
    //    auto veln = velocity.normalisedCopy();
    //
    //    Ogre::Vector3 a = -vn.crossProduct(veln);
    //
    //    q.x = a.x;
    //    q.z = a.y;
    //    q.y = a.z;
    //
    //    q.w = sqrt((pow(veln.length(), 2) * (pow(vn.length(), 2)))) + veln.dotProduct(a);
    //
    //    if (q.x != 0 || q.y != 0 || q.z != 0 || q.w != 0) {
    //        q.normalise();
    //        arrowNode->setOrientation(Ogre::Quaternion::Slerp(timeSinceLast, arrowNode->getOrientation(), arrowNode->getOrientation() * q));//();
    //    }
    //
    //    //velocity = vn;
    //
    //    velocity = v;
    //    updateVelocity();
    //}

    void Cell::updatePressureAlpha()
    {
        //personalDatablock->setTransparency(pressure, Ogre::HlmsPbsDatablock::Transparent, true);
    }

    void Cell::setPressure(Ogre::Real p)
    {
        //pressure = p;
    }

    Ogre::Real Cell::getPressure()
    {
        //return pressure;
        return mState.rPressure;
    }

    //void Cell::updateVelocity() {
        //velocity = originalVelocity;
        //velocity = arrowNode->getOrientation() * velocity;
    //}


    void Cell::setActive() 
    {
        mActive = true;
        mState.rPressure = 0.5f;
        mGameEntityManager->gameEntityColourChange(mPlaneEntity, Ogre::Vector3(50.0f, 50.0f, 0.0f));

        //Ogre::v1::Entity* pEnt = static_cast<Ogre::v1::Entity*>(mPlaneEntity->mMovableObject);
        ////pEnt->setDatablock("Active");
        //auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(pEnt->getSubEntity(0)->getDatablock());
        //auto c = datablock->getDiffuseColour();
        //datablock->setDiffuse(Ogre::Vector3(50.0f, 50.0f, 0.0f));
        //datablock->setTransparency(0.15f, Ogre::HlmsPbsDatablock::Transparent, true);
        //updatePressureIndicator();

        //Ogre::StringVector materialNames = mPlaneEntity->mMoDefinition->submeshMaterials;
        //size_t minMaterials = std::min(materialNames.size(), item->getNumSubItems());

        //for (size_t i = 0; i < minMaterials; ++i)
        //{
        //    item->getSubItem(i)->setDatablockOrMaterialName(materialNames[i],
        //        cge->gameEntity->mMoDefinition->
        //        resourceGroup);

        //    if (cge->useAlpha) {

        //        auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());

        //        Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
        //        diffuseBlock.mU = Ogre::TAM_WRAP;
        //        diffuseBlock.mV = Ogre::TAM_WRAP;
        //        diffuseBlock.mW = Ogre::TAM_WRAP;
        //        datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

        //        datablock->setTransparency(cge->gameEntity->mTransparency, Ogre::HlmsPbsDatablock::Transparent, true);
        //    }
        //}

        //planeItem->setDatablock("Active");
        //assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock()));
        //auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock());
        //Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
        //diffuseBlock.mU = Ogre::TAM_WRAP;
        //diffuseBlock.mV = Ogre::TAM_WRAP;
        //diffuseBlock.mW = Ogre::TAM_WRAP;
        //datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);
        //datablock->setTransparency(0.7f, Ogre::HlmsPbsDatablock::Transparent, true);
    }

    void Cell::unsetActive() {
        mActive = false;
        mState.rPressure = abs(mState.vVel.squaredLength());
        mGameEntityManager->gameEntityColourChange(mPlaneEntity, Ogre::Vector3(1, 1, 1));
    }

    //void Cell::setNeighbourly()
    //{
    //    planeItem->setDatablock("Neighbourly");
    //    assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock()));
    //    auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock());
    //    Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
    //    diffuseBlock.mU = Ogre::TAM_WRAP;
    //    diffuseBlock.mV = Ogre::TAM_WRAP;
    //    diffuseBlock.mW = Ogre::TAM_WRAP;
    //    datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);
    //    datablock->setTransparency(0.7f, Ogre::HlmsPbsDatablock::Transparent, true);
    //}

    //void Cell::unsetNeighbourly()
    //{
    //    planeItem->setDatablock(personalDatablock);
    //}

    //Ogre::Sphere* Cell::getSphere()
    //{
    //    return mSphere;
    //}
















    void Cell::warpBackInTime(float timeSinceLast)
    {
        //mSphere->setCenter(mSphere->getCenter() + (velocity * -timeSinceLast));
    }

    void Cell::warpForwardInTime(Ogre::Vector3 v, float timeSinceLast)
    {
        //mSphere->setCenter(mSphere->getCenter() + (v * timeSinceLast));
    }

    void Cell::undoTimewarp()
    {
        //mSphere->setCenter(arrowNode->getPosition());
    }

    void Cell::setScale() {
        //arrowNode->setScale(velocity.length(), 0.0f, velocity.length());
    }

    void Cell::orientArrow() {
  /*      Ogre::Quaternion q;

        auto normal = velocity.normalisedCopy();

        auto radians = normal.angleBetween(Ogre::Vector3(0, 0, 1));*/

        //arrowNode->setOrientation(Ogre::Quaternion(Ogre::Math::Cos(radians), 0, Ogre::Math::Sin(radians), 0));
    }
}
