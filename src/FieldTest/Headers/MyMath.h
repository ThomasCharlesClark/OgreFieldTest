#pragma once

#include "OgreVector3.h"

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

		// we need... we need the ability to avoid dividing by zero.

		Ogre::Real  xLength			= xExtremis - xMinima,
					zLength			= zExtremis - zMinima;

		if (xLength == 0)
			xLength = 1;

		if (zLength == 0)
			zLength = 1;

		float fxz1 = ((xExtremis - pX) / (xLength) * q11x) + ((pX - xMinima) / (xLength) * q21x);
		float fxz2 = ((xExtremis - pX) / (xLength) * q12x) + ((pX - xMinima) / (xLength) * q22x);

		float    x = ((zExtremis - pZ) / (zLength) * fxz1) + ((pZ - zMinima) / (zLength) * fxz2);

		float fzz1 = ((xExtremis - pX) / (xLength) * q11z) + ((pX - xMinima) / (xLength) * q21z);
		float fzz2 = ((xExtremis - pX) / (xLength) * q12z) + ((pX - xMinima) / (xLength) * q22z);

		float    z = ((zExtremis - pZ) / (zLength) * fzz1) + ((pZ - zMinima) / (zLength) * fzz2);


		return Ogre::Vector3(x, 0, z);
	}
}