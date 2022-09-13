#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	512
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	512
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
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

[numthreads(1, 1, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		int3 idx3 = int3(gl_GlobalInvocationID);
		int4 idx4 = int4(gl_GlobalInvocationID, 0);

		float ink = inkTextureFinal.Load(idx4);

		float4 velocity = velocityTextureFinal.Load(idx4);

		float vorticityValue = vortTex.Load(idx4);

		float4 pressure = pressureTexture.Load(idx4);
		
		//float4 final = float4(ink, 0.0, vorticityValue, 0.84);
		//float4 final = float4(0.0, 0.0, 0.0, 0.84);
		float4 final = float4(0.0, 0.0, 0.0, 0.84);
		
		final.xyz += velocity.xyz;
		//final.xyz += normalize(velocity.xyz);

		pixelBuffer[idx] = packUnorm4x8(final);

		velocityTextureFinal[idx3] += float4(velocity.xyz, 0);
	}
}