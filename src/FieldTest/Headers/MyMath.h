#pragma once

#include "OgreVector3.h"
#include "OgreQuaternion.h"

namespace MyThirdOgre {
	// Ooooh no it isn't :(
	static float derivative(float a, float b) {
		return (a*a) - (b*b);
	}

	// Thanks, Brian! 
	// https://forum.unity.com/threads/vector-bilinear-interpolation-of-a-square-grid.205644/

	static Ogre::Vector3 velocityBiLerp(
		Ogre::Vector3 a,  // point A (bottom-left)	: x in minima,   z in minima    // POSITION
		Ogre::Vector3 b,  // point B (top-left)		: x in minima,   z in extremis  // POSITION
		Ogre::Vector3 c,  // point C (top-right)	: x in extremis, z in extremis  // POSITION
		Ogre::Vector3 d,  // point D (bottom-right) : x in extremis, z in minima    // POSITION
		Ogre::Vector3 vA, // point A				:								// VELOCITY
		Ogre::Vector3 vB, // point B				:								// VELOCITY
		Ogre::Vector3 vC, // point C				:								// VELOCITY
		Ogre::Vector3 vD, // point D				:								// VELOCITY
		float pX,		  // P.x					: Point we want a velocity average for - X
		float pZ)		  // P.z					: Point we want a velocity average for - Z
	{

		//  xMinima,   <-----pX----->  xExtremis,
		//	zExtremis, b ----------- c zExtremis
		//			   |             |   
		//			   |             |    
		//			   |             |    
		//			   |  *(pX,pZ)   |    
		//			   |             |  
		//	xMinima,   a------------ d xExtremis,
		//  zMinima					   zMinima			
		
		Ogre::Real	xMinima			= a.x, 
					xExtremis		= c.x,
					zMinima			= a.z, 
					zExtremis		= c.z;

		Ogre::Real  q11x			= vA.x,
					q12x			= vB.x,
					q21x			= vC.x,
					q22x			= vD.x,

					q11z			= vA.z,
					q12z			= vB.z,
					q21z			= vC.z,
					q22z			= vD.z;

		Ogre::Real  xLength			= xExtremis - xMinima,
					zLength			= zExtremis - zMinima;

		float fxz1 = ((xExtremis - pX) / (xLength) * q11x) + ((pX - xMinima) / (xLength) * q21x);
		float fxz2 = ((xExtremis - pX) / (xLength) * q12x) + ((pX - xMinima) / (xLength) * q22x);

		float    x = ((zExtremis - pZ) / (zLength) * fxz1) + ((pZ - zMinima) / (zLength) * fxz2);

		float fzz1 = ((xExtremis - pX) / (xLength) * q11z) + ((pX - xMinima) / (xLength) * q21z);
		float fzz2 = ((xExtremis - pX) / (xLength) * q12z) + ((pX - xMinima) / (xLength) * q22z);

		float    z = ((zExtremis - pZ) / (zLength) * fzz1) + ((pZ - zMinima) / (zLength) * fzz2);


		return Ogre::Vector3(x, 0, z);
	}

	// Thanks, random StackOverflow dude!
	// https://gamedev.stackexchange.com/questions/15070/orienting-a-model-to-face-a-target#answer-15078
	static Ogre::Quaternion GetRotation(Ogre::Vector3 source, Ogre::Vector3 dest, Ogre::Vector3 up)
	{
		float dot = source.dotProduct(dest);

		if (abs(dot - (-1.0f)) < 0.000001f)
		{
			// vector a and b point exactly in the opposite direction, 
			// so it is a 180 degrees turn around the up-axis
			return Ogre::Quaternion(Ogre::Radian(Ogre::Math::DegreesToRadians(180.0f)), up);
		}
		if (abs(dot - (1.0f)) < 0.000001f)
		{
			// vector a and b point exactly in the same direction
			// so we return the identity quaternion
			return Ogre::Quaternion::IDENTITY;
		}

		float rotAngle = (float)acos(dot);
		Ogre::Vector3 rotAxis = source.crossProduct(dest);
		rotAxis.normalise();
		Ogre::Quaternion result;
		result.FromAngleAxis(Ogre::Radian(rotAngle), rotAxis);
		result.normalise();
		return result;
	}
}