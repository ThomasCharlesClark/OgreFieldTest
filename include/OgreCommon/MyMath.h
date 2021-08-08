#pragma once

#include "OgreVector3.h"
#include "OgreQuaternion.h"

namespace MyThirdOgre {
	// Ooooh no it isn't :(
	static float derivative(float a, float b) {
		return (a*a) - (b*b);
	}

	// Thanks to the Omni calculator for helping me verify that working this through by hand was correct.
	// https://www.omnicalculator.com/math/bilinear-interpolation#bilinear-interpolation-formula

	static Ogre::Vector3 vectorBiLerp(
		Ogre::Vector3 a,  // point A (bottom-left)	: x in minima,   z in minima    // POSITION
		Ogre::Vector3 b,  // point B (top-left)		: x in minima,   z in extremis  // POSITION
		Ogre::Vector3 c,  // point C (top-right)	: x in extremis, z in extremis  // POSITION
		Ogre::Vector3 d,  // point D (bottom-right) : x in extremis, z in minima    // POSITION
		Ogre::Vector3 vA, // point A				:								// VELOCITY
		Ogre::Vector3 vB, // point B				:								// VELOCITY
		Ogre::Vector3 vC, // point C				:								// VELOCITY
		Ogre::Vector3 vD, // point D				:								// VELOCITY
		float pX,		  // P.x					: Point we want a velocity average for - X
		float pZ,		  // P.z					: Point we want a velocity average for - Z
		bool crossFormation = false)	// this is BS	 
	{
		//             q12         q21
		//  xMinima,   <-----pX----->  xExtremis,
		//	zExtremis, b ----------- c zExtremis
		//			   |             |   
		//			   |             |    
		//			   |             |    
		//			   |  *(pX,pZ)   |    
		//			   |             |  
		//	xMinima,   a------------ d xExtremis,
		//  zMinima	   q11		   q22 zMinima			


		// Okay, but sometimes we don't have a square to deal with.
		// Sometimes we only have a cross:
		//         
		//         _____
		//        |     |
		//     <--|  b  |  ^
		//   _____|_____|__|__
		//  |     |     |     |
		//	|  a  |	 p  |  c  |
		//  |_____|_____|_____|
		//     |  |     |
		//     v  |  d  |-->
		//        |_____|
		//
		//  to force this configuration to fit the above algorithm, what do we need to do?
		//  well, as I know I'm working with unit grid spacing this becomes trivial:
		//  adjust a.z, b.x, c.z and d.x!
		
		if (crossFormation) 
		{
			a.z += 1.0f; // bringing a back towards us is +ve because we're working with -ve Z axis going INTO the screen
			b.x -= 1.0f;
			c.z -= 1.0f;
			d.x += 1.0f;
		}

		Ogre::Real	xMinima = a.x,
					xExtremis = c.x,
					zMinima = a.z, 
					zExtremis = c.z;

		Ogre::Real  q11x = vA.x,
					q12x = vB.x,
					q21x = vC.x,
					q22x = vD.x,

					q11z = vA.z,
					q12z = vB.z,
					q21z = vC.z,
					q22z = vD.z;

		Ogre::Real  xLength = xExtremis - xMinima,
			zLength = zExtremis - zMinima;

		float fxz1 = ((xExtremis - pX) / (xLength)*q11x) + ((pX - xMinima) / (xLength)*q21x);
		float fxz2 = ((xExtremis - pX) / (xLength)*q12x) + ((pX - xMinima) / (xLength)*q22x);

		float    x = ((zExtremis - pZ) / (zLength)*fxz1) + ((pZ - zMinima) / (zLength)*fxz2);

		float fzz1 = ((xExtremis - pX) / (xLength)*q11z) + ((pX - xMinima) / (xLength)*q21z);
		float fzz2 = ((xExtremis - pX) / (xLength)*q12z) + ((pX - xMinima) / (xLength)*q22z);

		float    z = ((zExtremis - pZ) / (zLength)*fzz1) + ((pZ - zMinima) / (zLength)*fzz2);

		if (isnan(x) || isinf(x))
			x = 0;

		if (isnan(z) || isinf(z))
			z = 0;

		return Ogre::Vector3(x, 0, z);
	}

	static Ogre::Real scalarBiLerp(
		Ogre::Vector3 a,  // point A (bottom-left)	: x in minima,   z in minima    // POSITION
		Ogre::Vector3 b,  // point B (top-left)		: x in minima,   z in extremis  // POSITION
		Ogre::Vector3 c,  // point C (top-right)	: x in extremis, z in extremis  // POSITION
		Ogre::Vector3 d,  // point D (bottom-right) : x in extremis, z in minima    // POSITION
		Ogre::Real rA,    // point A				:								// SCALAR
		Ogre::Real rB,    // point B				:								// SCALAR
		Ogre::Real rC,	  // point C				:								// SCALAR
		Ogre::Real rD,	  // point D				:								// SCALAR
		float pX,		  // P.x					: Point we want a vector average for - X
		float pZ,	      // P.z					: Point we want a vector average for - Z
		bool crossFormation)	// this is BS	  
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

		if (crossFormation) { // this is BS
			a.z += 1.0f; // bringing a back towards us is +ve because we're working with -ve Z axis going INTO the screen
			b.x -= 1.0f;
			c.z -= 1.0f;
			d.x += 1.0f;
		}

		Ogre::Real	xMinima = a.x,
			xExtremis = c.x,
			zMinima = a.z,
			zExtremis = c.z;

		Ogre::Real  q11 = rA,
					q12 = rB,
					q21 = rC,
					q22 = rD;

		Ogre::Real  xLength = xExtremis - xMinima,
			zLength = zExtremis - zMinima;

		float fxz1 = ((xExtremis - pX) / (xLength)*q11) + ((pX - xMinima) / (xLength)*q21);
		float fxz2 = ((xExtremis - pX) / (xLength)*q12) + ((pX - xMinima) / (xLength)*q22);

		float    r = ((zExtremis - pZ) / (zLength)*fxz1) + ((pZ - zMinima) / (zLength)*fxz2);

		if (isnan(r) || isinf(r))
			r = 0;

		return r;
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
			auto result = Ogre::Quaternion(Ogre::Radian(Ogre::Math::DegreesToRadians(180.0f)), up);
			result.normalise();
			return result;
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