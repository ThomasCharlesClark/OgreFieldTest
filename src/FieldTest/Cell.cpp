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


Cell::Cell(int rowIndex, int rowCount, int columnIndex, int columnCount, float s, bool onTheBoundary, Ogre::SceneManager* scnmgr)
{
    xIndex = rowIndex;
    yIndex = columnIndex;
    boundary = onTheBoundary;

    bool centerBoundary = false;

    if (centerBoundary) {
        if ((rowIndex == 10 && columnIndex == 10) ||
            (rowIndex == 11 && columnIndex == 11) ||
            (rowIndex == 11 && columnIndex == 10) ||
            (rowIndex == 10 && columnIndex == 11)) {
            boundary = true;
        }
    }

    //originalVelocity = Ogre::Vector3::ZERO;
    originalVelocity = Ogre::Vector3(Ogre::Math::RangeRandom(-2.0f, 2.0f), 0.0f, -Ogre::Math::RangeRandom(-2.0f, 2.0f));

    velocity = originalVelocity;

    gridReference = Ogre::Vector2(rowIndex, -columnIndex);

    arrowNode = scnmgr->getRootSceneNode(Ogre::SCENE_DYNAMIC)->
        createChildSceneNode(Ogre::SCENE_DYNAMIC);

    arrowObject = scnmgr->createManualObject();

    arrowObject->begin(boundary ? "UnlitRed" : "UnlitWhite", Ogre::OT_LINE_LIST);

    // Back
    arrowObject->position(0.0f, 0.0f, 0.25f);
    arrowObject->position(0.0f, 0.f, -0.25f);
    arrowObject->line(0, 1);

    arrowObject->position(0.0f, 0.0f, -0.25f);
    arrowObject->position(0.2f, 0.0f, 0.0f);
    arrowObject->line(2, 3);

    arrowObject->position(0.0f, 0.0f, -0.25f);
    arrowObject->position(-0.2f, 0.0f, 0.0f);
    arrowObject->line(4, 5);

    arrowObject->end();

    arrowNode->attachObject(arrowObject);

    arrowNode->translate(rowIndex, 0.0f, columnIndex, Ogre::SceneNode::TS_LOCAL);


    // Quaternion Cheatsheet:
    // Take the degrees you want to rotate e.g. 135°
    // halve it = 67.5°
    // convert it to radians = 1 degree is equivalent to (π/180) radians
    // therefore multiply by (π/180)
    // == 0.3827rad
    // for W, cosine it
    // for x,y,z, sine it


    if (boundary) {

        velocity = Ogre::Vector3::ZERO;
        originalVelocity = Ogre::Vector3::ZERO;

        //outer edges:
        if (rowIndex == 0) { // x == 0 edge
            arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.5253, 0.0, -0.5253, 0.0));
            velocity = Ogre::Vector3(1.0f, 0, 0); // boundary gives x+ve velocity
        }

        if (rowIndex == rowCount - 1) { // opposite x edge
            arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.5253, 0.0, 0.5253, 0.0));
            velocity = Ogre::Vector3(-1.0f, 0, 0); // boundary gives x-ve velocity
        
        }
    
        if (columnIndex == 0) { // z == 0 edge
            arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(1.0, 0.0, 0.0, 0.0));
            //velocity = Ogre::Vector3(0.0f, 0, -1.0f); // boundary gives z-ve velocitye
        
        }

        if (columnIndex == -columnCount + 1) { // opposite z edge
            arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.0, 0.0, 1.0, 0.0));
            velocity = Ogre::Vector3(0.0f, 0, 1.0f); // boundary gives z+ve velocity
        
        }

        //  
        //corners:
        if (rowIndex == 0 && columnIndex == 0) { // x == 0, y == 0
            arrowNode->setOrientation(Ogre::Quaternion(0.9239, 0.0, -0.3827, 0.0));
            velocity = Ogre::Vector3(0.5f, 0, 0.5f); // corner gives 1/2 x, 1/2 y + ve
        }

        if (rowIndex == rowCount - 1 && columnIndex == 0) { // x == max, y == 0
            arrowNode->setOrientation(Ogre::Quaternion(0.9239, 0.0, 0.3827, 0.0));
            velocity = Ogre::Vector3(-0.5f, 0, 0.5f); // corner gives -1/2 x, 1/2 y + ve

        }

        if (rowIndex == 0 && columnIndex == -columnCount + 1) { // x == 0, y == max
            arrowNode->setOrientation(Ogre::Quaternion(-0.3827, 0.0, 0.9239, 0.0));
            velocity = Ogre::Vector3(-0.5f, 0, 0.5f); // corner gives 1/2 x, -1/2 y + ve
        }

        if (rowIndex == rowCount - 1 && columnIndex == -columnCount + 1) { // x == max, y == max
            arrowNode->setOrientation(Ogre::Quaternion(0.3827, 0.0, 0.9239, 0.0));
            velocity = Ogre::Vector3(-0.5f, 0, -0.5f); // corner gives -1/2 x, -1/2 y + ve

        }

        // center:

        //if (centerBoundary) {
        //    if (rowIndex == 10 && columnIndex == 10) {
        //        arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.9239, 0.0, 0.0, 0.3827));
        //    }

        //    if (rowIndex == 11 && columnIndex == 11) {
        //        arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.3827, 0.0, 0.0, -0.923879));
        //    }

        //    if (rowIndex == 11 && columnIndex == 10) {
        //        arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.9239, 0.0, 0.0, -0.3827));
        //    }

        //    if (rowIndex == 10 && columnIndex == 11) {
        //        arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(0.3827, 0.0, 0.0, 0.923879));
        //    }
        //}

        //velocity = arrowNode->getOrientation() * velocity;

        //velocity *= 0.000005f; // less

        //originalVelocity = velocity;

        //if (columnIndex == columnCount - 1)
            //arrowNode->setOrientation(Ogre::Quaternion(0.707, 0.707, 0.0, 0.0));
    }
    else {
        // set the inital orientation to some interesting random value
        //Ogre::Real randomRotationAboutZ = Ogre::Math::RangeRandom(0, Ogre::Math::PI);

       /* Ogre::Real randomRotationAboutZ = Ogre::Math::RangeRandom(0, Ogre::Math::PI * 2);
        randomRotationAboutZ /= 2;
        arrowNode->setOrientation(arrowNode->getOrientation() * Ogre::Quaternion(Ogre::Math::Cos(randomRotationAboutZ), 0.0f, 0.0f, Ogre::Math::Sin(randomRotationAboutZ)));*/
    }

    if (boundary)
        arrowNode->scale(0.5f, 0.0f, 0.5f);
    else
        arrowNode->scale(0.0f, 0.0f, 0.0f);

    #pragma region Plane
    std::stringstream ss;
    ss << "Plane " << rowIndex << ", " << columnIndex;
    const std::string tmp = std::string{ ss.str() };
    const char* planeName = tmp.c_str();

    Ogre::v1::MeshPtr planeMeshV1 = Ogre::v1::MeshManager::getSingleton().createPlane(planeName,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f), 1.0f, 1.0f,
        1, 1, true, 1, 1.0f, 1.0f, Ogre::Vector3::UNIT_Z,
        Ogre::v1::HardwareBuffer::HBU_STATIC,
        Ogre::v1::HardwareBuffer::HBU_STATIC);

    Ogre::MeshPtr planeMesh = Ogre::MeshManager::getSingleton().createManual(
        planeName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    planeMesh->importV1(planeMeshV1.get(), true, true, true);
    {
        planeItem = scnmgr->createItem(planeMesh, Ogre::SCENE_DYNAMIC);

        if (boundary) {
            pressure = 0.05f;
            planeItem->setDatablock("Transparent");
        }
        else {
            //pressure = Ogre::Math::RangeRandom(0.05f, 0.15f);
            pressure = 0.05f;
            planeItem->setDatablock("Red");
        }

        originalPressure = pressure;

        Ogre::SceneNode* sceneNode = scnmgr->getRootSceneNode(Ogre::SCENE_DYNAMIC)->
            createChildSceneNode(Ogre::SCENE_DYNAMIC);
        sceneNode->setPosition(rowIndex, 0.0f, columnIndex);
        sceneNode->attachObject(planeItem);

        auto originalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock());
        auto originalDataBlockName = *originalDatablock->getNameStr();

        personalDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(originalDatablock->clone(originalDataBlockName
            + Ogre::StringConverter::toString(planeItem->getId())));

        planeItem->getSubItem(0)->setDatablock(personalDatablock);

        //Change the addressing mode of the roughness map to wrap via code.
        //Detail maps default to wrap, but the rest to clamp.
        assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock()));

        //Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(
        //    item->getSubItem(0)->getDatablock());

        ////Make a hard copy of the sampler block
        //Ogre::HlmsSamplerblock roughnessBlock(*datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS));
        //roughnessBlock.mU = Ogre::TAM_WRAP;
        //roughnessBlock.mV = Ogre::TAM_WRAP;
        //roughnessBlock.mW = Ogre::TAM_WRAP;
        //datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, roughnessBlock);

        Ogre::HlmsSamplerblock diffuseBlock(*personalDatablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
        diffuseBlock.mU = Ogre::TAM_WRAP;
        diffuseBlock.mV = Ogre::TAM_WRAP;
        diffuseBlock.mW = Ogre::TAM_WRAP;
        personalDatablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);

        if (!boundary)
            //personalDatablock->setTransparency(0, Ogre::HlmsPbsDatablock::Transparent, true);
            personalDatablock->setTransparency(pressure, Ogre::HlmsPbsDatablock::Transparent, true);
        else
            personalDatablock->setTransparency(0, Ogre::HlmsPbsDatablock::Transparent, true);
    }
    #pragma endregion

    if (!boundary) {
        originalOrientation = arrowNode->getOrientation();

    }

    originalPosition = arrowNode->getPosition();

    sphere = new Ogre::Sphere(originalPosition, 1.0f);
}

Cell::~Cell()
{
    delete sphere;
    sphere = 0;
}

Ogre::Vector3 Cell::getVelocity()
{
	return velocity;
}

void Cell::setVelocity(Ogre::Vector3 v, float timeSinceLast) {
    velocity = v;
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
    personalDatablock->setTransparency(pressure, Ogre::HlmsPbsDatablock::Transparent, true);
}

void Cell::setPressure(Ogre::Real p) 
{
    pressure = p;
}

Ogre::Real Cell::getPressure()
{
    return pressure;
}

void Cell::updateVelocity() {
    //velocity = originalVelocity;
    //velocity = arrowNode->getOrientation() * velocity;
}

void Cell::restoreOriginalState()
{
    pressure = originalPressure;

    this->updatePressureAlpha();

    //arrowNode->setOrientation(originalOrientation);
    //arrowNode->setPosition(originalPosition);

    planeItem->setDatablock(personalDatablock);

    if (!boundary)
        update(originalVelocity, 0.0f);
}

void Cell::randomiseVelocity() {
    if (!boundary)
        originalVelocity = Ogre::Vector3(Ogre::Math::RangeRandom(-2.0f, 2.0f), 0.0f, -Ogre::Math::RangeRandom(-2.0f, 2.0f));
    restoreOriginalState();
}

void Cell::setActive() {
    planeItem->setDatablock("Active");
    assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock()));
    auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock());
    Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
    diffuseBlock.mU = Ogre::TAM_WRAP;
    diffuseBlock.mV = Ogre::TAM_WRAP;
    diffuseBlock.mW = Ogre::TAM_WRAP;
    datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);
    datablock->setTransparency(0.7f, Ogre::HlmsPbsDatablock::Transparent, true);
}

void Cell::unsetActive() {
    planeItem->setDatablock(personalDatablock);
}

void Cell::setNeighbourly()
{
    planeItem->setDatablock("Neighbourly");
    assert(dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock()));
    auto datablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(planeItem->getSubItem(0)->getDatablock());
    Ogre::HlmsSamplerblock diffuseBlock(*datablock->getSamplerblock(Ogre::PBSM_DIFFUSE));
    diffuseBlock.mU = Ogre::TAM_WRAP;
    diffuseBlock.mV = Ogre::TAM_WRAP;
    diffuseBlock.mW = Ogre::TAM_WRAP;
    datablock->setSamplerblock(Ogre::PBSM_DIFFUSE, diffuseBlock);
    datablock->setTransparency(0.7f, Ogre::HlmsPbsDatablock::Transparent, true);
}

void Cell::unsetNeighbourly()
{
    planeItem->setDatablock(personalDatablock);
}

Ogre::ManualObject* Cell::getArrowObject()
{
    return arrowObject;
}











Ogre::Sphere* Cell::getSphere()
{
    return sphere;
}


















// This function simply updates our "arrow" node to point in the direction our velocity is pointing.
void Cell::update(Ogre::Vector3 v, Ogre::Real timeSinceLastFrame) {

    velocity = v;

    Cell::setScale();

    Cell::orientArrow();
}

void Cell::warpBackInTime(float timeSinceLast)
{
    sphere->setCenter(sphere->getCenter() + (velocity * -timeSinceLast));
}

void Cell::warpForwardInTime(Ogre::Vector3 v, float timeSinceLast)
{
    sphere->setCenter(sphere->getCenter() + (v * timeSinceLast));
}

void Cell::undoTimewarp()
{
    sphere->setCenter(arrowNode->getPosition());
}

void Cell:: setScale() {
    arrowNode->setScale(velocity.length(), 0.0f, velocity.length());
}

void Cell::orientArrow() {
    Ogre::Quaternion q;

    auto normal = velocity.normalisedCopy();

    auto radians = normal.angleBetween(Ogre::Vector3(0, 0, 1));
    
    arrowNode->setOrientation(Ogre::Quaternion(Ogre::Math::Cos(radians), 0, Ogre::Math::Sin(radians), 0));

    //auto up = Ogre::Vector3(0, 1, 0);

    //Ogre::Vector3 a = normal.crossProduct(up);

    //q.x = a.x;
    //q.z = a.y;
    //q.y = a.z;

    //q.w = sqrt((pow(normal.length(), 2) * (pow(up.length(), 2)))) + normal.dotProduct(a);

    //if (q.x != 0 || q.y != 0 || q.z != 0 || q.w != 0) {
    //    q.normalise();
    //    arrowNode->setOrientation(q);
    //}
}