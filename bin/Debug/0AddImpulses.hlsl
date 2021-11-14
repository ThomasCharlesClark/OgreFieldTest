#if 0
	***	threads_per_group_x	8
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	8
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	64
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	64
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
};

RWStructuredBuffer<Particle> inputUavBuffer		: register(u0); // inputUavBuffer
RWTexture3D<float4> velocityTexture				: register(u1); // velocityTexture
RWTexture3D<float4> inkTexture					: register(u2); // inkTexture

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

[numthreads(8, 8, 1)]
void main
(
	uint3 gl_LocalInvocationID : SV_GroupThreadID,
	uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		//float4 velocity = float4(
		//	inputUavBuffer[rwIdx].velocity.x,
		//	inputUavBuffer[rwIdx].velocity.y,
		//	inputUavBuffer[rwIdx].velocity.z,
		//	1.0);

		float4 velocity = float4(inputUavBuffer[rwIdx].velocity, 1.0);

		float width = texResolution.x;

		int4 idx4 = int4(gl_GlobalInvocationID.xyz, 0);

		velocityTexture[gl_GlobalInvocationID] += velocity;
		inkTexture[gl_GlobalInvocationID] += inputUavBuffer[rwIdx].colour;
		//inkTexture[gl_GlobalInvocationID] += inputUavBuffer[rwIdx].colour;
	}
}