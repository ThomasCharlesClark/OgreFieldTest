#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	256
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	256
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
RWTexture3D<float> tempInkTexture				: register(u3); // tempInkTexture
Texture3D<float> velocityTextureFinal			: register(t0); // velocityFinal
Texture3D<float> inkTextureFinal				: register(t1); // inkTextureFinal

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

[numthreads(1, 1, 1)]
void main
(
	uint3 gl_LocalInvocationID : SV_GroupThreadID,
	uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float4 velocity = float4(inputUavBuffer[rwIdx].velocity, 1.0);

		float width = texResolution.x;

		int4 idx4 = int4(gl_GlobalInvocationID.xyz, 0);

		//float4 prevVelocity = velocityTextureFinal.SampleLevel(TextureSampler, gl_GlobalInvocationID / width, 0);
		//float4 prevInk = inkTextureFinal.SampleLevel(TextureSampler, gl_GlobalInvocationID / width, 0);

		float4 prevVelocity = velocityTextureFinal.Load(idx4);
		float4 prevInk = inkTextureFinal.Load(idx4);

		// being additive here might be wrong
		velocityTexture[gl_GlobalInvocationID] += velocity;// +prevVelocity;
		inkTexture[gl_GlobalInvocationID] += inputUavBuffer[rwIdx].colour;
		tempInkTexture[gl_GlobalInvocationID] = inputUavBuffer[rwIdx].ink;

		//inputUavBuffer[rwIdx].ink = 0.0;
		//inputUavBuffer[rwIdx].colour *= 0.98;
		//inputUavBuffer[rwIdx].velocity *= 0.98;
		//inputUavBuffer[rwIdx].colour = float4(0, 0, 0, 1);
		//inputUavBuffer[rwIdx].velocity = float3(0, 0, 0);
	}
}