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

namespace MyThirdOgre 
{
    Cell::Cell(int rowIndex, int columnIndex, int rowCount, int columnCount, GameEntityManager* geMgr) :
        mIndexX(rowIndex),
        mIndexZ(columnIndex),
        mRowCount(rowCount),
        mColumnCount(columnCount),
        mBoundary(false),
        mArrowEntity(0),
        mArrowMoDef(0),
        mPlaneEntity(0),
        mPlaneMoDef(0),
        mSphereEntity(0),
        mSphereMoDef(0),
        mSphere(0),
        mState ( mState.vVel = Ogre::Vector3::ZERO, mState.qRot = Ogre::Quaternion::IDENTITY, mState.rPressure = Ogre::Math::RangeRandom(0.0f, 0.8f) )
    {
        mBoundary = rowIndex == 0 || columnIndex == 0 || rowIndex == rowCount - 1 || columnIndex == columnCount  - 1;

        createArrow(geMgr);

        createPressureIndicator(geMgr);

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

    void Cell::createArrow(GameEntityManager* geMgr) 
    {
        mArrowMoDef = new MovableObjectDefinition();
        mArrowMoDef->meshName = "";
        mArrowMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mArrowMoDef->moType = MoTypeStaticManualLineList;

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

        Ogre::Quaternion qRot;

        if (mBoundary) {
            //outer edges:
            if (mIndexX == 0) { // x == 0 edge
                qRot = Ogre::Quaternion(0.5253, 0.0, -0.5253, 0.0);
            } else if (mIndexX == mRowCount - 1) { // opposite x edge
                qRot = Ogre::Quaternion(0.5253, 0.0, 0.5253, 0.0);
            } else if (mIndexZ == 0) { 
                qRot = Ogre::Quaternion(1.0, 0.0, 0.0, 0.0);
            } else if (mIndexZ == mColumnCount - 1) { // opposite z edge
                qRot = Ogre::Quaternion(0.0, 0.0, 1.0, 0.0);
            }

            //corners:
            if (mIndexX == 0 && mIndexZ == 0) { // x == 0, y == 0
                qRot = Ogre::Quaternion(0.9239, 0.0, -0.3827, 0.0);
            } else if (mIndexX == mRowCount - 1 && mIndexZ == 0) { // x == max, y == 0
                qRot = Ogre::Quaternion(0.9239, 0.0, 0.3827, 0.0);
            } else if (mIndexX == 0 && mIndexZ == mColumnCount - 1) { // x == 0, y == max
                qRot = Ogre::Quaternion(-0.3827, 0.0, 0.9239, 0.0);
            } else if (mIndexX == mRowCount - 1 && mIndexZ == mColumnCount - 1) { // x == max, y == max
                qRot = Ogre::Quaternion(0.3827, 0.0, 0.9239, 0.0);
            }
        }
        else {
            // set the inital orientation to some interesting random value?
            // Ogre::Real randomRotationAboutZ = Ogre::Math::RangeRandom(-Ogre::Math::PI * 2, Ogre::Math::PI * 2);
        }

        mArrowEntity = geMgr->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_STATIC,
            mArrowMoDef,
            mBoundary ? "UnlitRed" : "UnlitWhite",
            arrowLineList,
            Ogre::Vector3(mIndexX, 0, -mIndexZ),
            qRot,
            mBoundary ? Ogre::Vector3::UNIT_SCALE : mState.vVel
        );

        mState.qRot = qRot;
    }

    void Cell::createPressureIndicator(GameEntityManager* geMgr) 
    {
        mPlaneMoDef = new MovableObjectDefinition();
        mPlaneMoDef->meshName = "";
        mPlaneMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mPlaneMoDef->moType = MoTypePrefab;

        Ogre::Quaternion pRot = Ogre::Quaternion::IDENTITY;

        pRot.FromAngleAxis(Ogre::Radian(Ogre::Degree(-90)), Ogre::Vector3::UNIT_X);

        mPlaneEntity = geMgr->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_STATIC,
            mPlaneMoDef,
            Ogre::SceneManager::PrefabType::PT_PLANE,
            mBoundary ? "Red" : "Blue",
            Ogre::Vector3(mIndexX, 0, -mIndexZ),
            pRot,
            Ogre::Vector3(0.005f, 0.005f, 0.005f),
            true,
            mBoundary ? 0.4f : mState.rPressure);
    }

    void Cell::createBoundingSphere(GameEntityManager* geMgr) 
    {
        Ogre::Vector3 sPos = Ogre::Vector3(mIndexX, 0, -mIndexZ);

        mSphere = new Ogre::Sphere(sPos, 1.0f);

        mSphereMoDef = new MovableObjectDefinition();
        mSphereMoDef->meshName = "Sphere1000.mesh";
        mSphereMoDef->resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        mSphereMoDef->moType = MoTypeItem;

        mSphereEntity = geMgr->addGameEntity(Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC,
            mSphereMoDef,
            sPos,
            Ogre::Quaternion::IDENTITY,
            Ogre::Vector3::UNIT_SCALE,
            true,
            0.2f);
    }

    Ogre::Vector3 Cell::getVelocity()
    {
        //return velocity;

        return mState.vVel;
    }

    void Cell::setVelocity(Ogre::Vector3 v, float timeSinceLast) {
        //velocity = v;
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

    void Cell::restoreOriginalState()
    {
        mState = mOriginalState;
    }

    //void Cell::setActive() {
    //    planeItem->setDatablock("Active");
    //    assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock()));
    //    auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock());
    //    Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
    //    diffuseBlock.mU = Ogre::TAM_WRAP;
    //    diffuseBlock.mV = Ogre::TAM_WRAP;
    //    diffuseBlock.mW = Ogre::TAM_WRAP;
    //    datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);
    //    datablock->setTransparency(0.7f, Ogre::HlmsPbsDatablock::Transparent, true);
    //}

    //void Cell::unsetActive() {
    //    planeItem->setDatablock(personalDatablock);
    //}

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


















    // Updates our transform
    void Cell::update(Ogre::Vector3 v, Ogre::Real timeSinceLastFrame) {

        //velocity = v;

        //Cell::setScale();

        //Cell::orientArrow();
    }

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
