struct Particle 
{
	float ink;
	float4 colour;
	float3 velocity;
};

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

RWStructuredBuffer<uint> pixelBuffer		: register(u0);
RWTexture3D<float4> velocityTextureFinal	: register(u1);
RWTexture3D<float4> velocityTexture			: register(u2);
RWTexture3D<float> vortTex					: register(u3);
RWTexture3D<float4> pressureTexture			: register(u4);
Texture3D<float> inkTextureFinal			: register(t0);

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
	//if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y )
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		int4 idx4 = int4(gl_GlobalInvocationID, 0);

		float ink = inkTextureFinal.Load(idx4);

		float4 v = velocityTextureFinal.Load(idx4);

		float vortValue = vortTex.Load(idx4);

		float4 p = pressureTexture.Load(idx4);

		float4 final = float4(ink, 0, 0, 0.84);
		
		//final.xyz += normalize(v.xyz);

		//final.xyz = v.xyz;

		//final.xyz += normalize(abs(v.xyz));

		//final.xyz -= p.xyz;
		
		//final.xyz *= length(v);

		//final.z += vortValue;

		//if (gl_GlobalInvocationID.x == 0 && gl_GlobalInvocationID.y == 0) {
		//	final = float4(1, 0, 0, 1);
		//}
		//else {
		//	//final = float4(0, 0, 0, 1);
		//}

		//https://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio#answer-929107
		//NewValue = (((OldValue - OldMin) * (NewMax - NewMin)) / (OldMax - OldMin)) + NewMin


		//if (ink != 0)
		//	final += float4(0.85, 0.2941176562, 0, ink);


		//final.xyz = inkColour.xyz;

		//final += inkColour.xyz;

		pixelBuffer[idx] = packUnorm4x8(final);
	}
}