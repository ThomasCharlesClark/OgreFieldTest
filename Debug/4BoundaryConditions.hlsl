#if 0
	***	threads_per_group_x	1
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	1
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
RWTexture3D<float3> velocityTexture			: register(u0);
RWTexture3D<float3> pressureTexture			: register(u1);
RWTexture3D<float> inkTexture				: register(u2);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float reciprocalDeltaX;

[numthreads(1, 1, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x == 0) {
		float3 neighbourIdx = float3(gl_GlobalInvocationID.x + 1, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
		
	if (gl_GlobalInvocationID.x == texResolution.x - 1) {

		float3 neighbourIdx = float3(gl_GlobalInvocationID.x - 1, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
		
	if (gl_GlobalInvocationID.y == 0) {
		float3 neighbourIdx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y + 1, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
		
	if (gl_GlobalInvocationID.y == texResolution.y - 1) {

		float3 neighbourIdx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y - 1, gl_GlobalInvocationID.z);

		velocityTexture[gl_GlobalInvocationID] = -1 * velocityTexture[neighbourIdx];
		pressureTexture[gl_GlobalInvocationID] = pressureTexture[neighbourIdx];
		inkTexture[gl_GlobalInvocationID] = inkTexture[neighbourIdx];
	}
}