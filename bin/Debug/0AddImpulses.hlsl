#if 0
	***	threads_per_group_x	8
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	8
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	32
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	32
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
};

RWStructuredBuffer<Particle> handInputBuffer	: register(u0); // inputUavBuffer (leapMotion input)
RWTexture3D<float4> velocityWrite				: register(u1);	// primaryVelocityTexture
RWTexture3D<float4> inkWrite					: register(u2); // primaryInkTexture
RWTexture3D<float> inkTemp						: register(u3); // tempInkTexture (temporary uav)

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

		float4 velocity = float4(handInputBuffer[rwIdx].velocity, 1.0);

		// being additive here might be wrong
		velocityWrite[gl_GlobalInvocationID] = velocity;
		inkWrite[gl_GlobalInvocationID] += handInputBuffer[rwIdx].colour;
		inkTemp[gl_GlobalInvocationID] += handInputBuffer[rwIdx].ink;
		
		// clearing the input buffer here == slowtown
		/*handInputBuffer[rwIdx].colour = float4(0, 0, 0, 1.0);
		handInputBuffer[rwIdx].velocity = float3(0, 0, 0);
		handInputBuffer[rwIdx].ink = 0.0;*/
		// but not clearing it at all == bleh
	}
}