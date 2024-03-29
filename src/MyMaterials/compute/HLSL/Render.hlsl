struct Particle 
{
	float ink;
	float4 colour;
	float3 velocity;
	float inkLifetime;
};

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

RWStructuredBuffer<uint> pixelBuffer	: register(u0);
RWTexture3D<float4> velocityTexture		: register(u1);
RWTexture3D<float4> inkTexture			: register(u2);
RWTexture3D<float4> pressureTexture		: register(u3);
RWTexture3D<float> vorticityTexture		: register(u4);

uniform float maxInk;
uniform uint2 texResolution;
uniform float timeSinceLast;

uint packUnorm4x8( float4 value )
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	uint w = uint(saturate(value.w) * 255.0f);
	
	return x | (y << 8u) | (z << 16u ) | (w << 24u);
}

float4 unpackUnorm4x8(uint value)
{
	float4 retVal;
	retVal.x = float(value & 0xFF);
	retVal.y = float((value >> 8u) & 0xFF);
	retVal.z = float((value >> 16u) & 0xFF);
	retVal.w = float((value >> 24u) & 0xFF);
	 
	return retVal * 0.0039215687f; // 1.0 / 255.0f;
}

float normaliseInkValue(float i) 
{
	return i / maxInk;
}

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if(gl_GlobalInvocationID.x > 0 && 
	   gl_GlobalInvocationID.x < texResolution.x - 1 && 
	   gl_GlobalInvocationID.y > 0 &&
	   gl_GlobalInvocationID.y < texResolution.y - 1)
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		int3 idx3 = int3(gl_GlobalInvocationID);
		int4 idx4 = int4(gl_GlobalInvocationID, 0);

		float ink = inkTexture.Load(idx4);

		float4 velocityOriginal = velocityTexture.Load(idx4);

		float4 velocity = velocityOriginal;

		float vorticity = vorticityTexture.Load(idx4);

		float4 pressure = pressureTexture.Load(idx4);
		
		float4 final = float4(0, 0, 0, 1.0);

		final.x += ink / 4;
		final.y += ink / 48.0f;

		/*
		final.x += ink;
		final.y += ink / 24.0f;
		final.z += vorticity;
		*/

		// you can't colourize using negative numbers, it doesn't work.
		// so send the components positive by whatever... means... necessary. then normalize.

		int minus = -1;
		int plus = 1;

		//velocity.x *= velocity.x < 0 ? minus : plus;
		//velocity.y *= velocity.y < 0 ? minus : plus;
		//velocity.z *= velocity.z < 0 ? minus : plus;

		//final.z += length(velocity.xyz);
		//final.xyz += normalize(velocity.xyz);
		//final.xyz += velocity.xyz;

		pixelBuffer[idx] = packUnorm4x8(final);
		 
		//velocityTexture[idx3] += float4(velocityOriginal.xyz, 0);

		//inkTexture[idx3] = float4(ink, 0, 0, 0);
	}
}