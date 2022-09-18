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
struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
}; 

RWTexture3D<float> inkTexture					: register(u0);
RWTexture3D<float> inkTexFinal				: register(u1);
RWTexture3D<float4> velocityTexture				: register(u2);
RWTexture3D<float4> velocityFinal				: register(u3);
RWTexture3D<float4> divergenceTexture				: register(u4);

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float velocityDissipationConstant; 
uniform float inkDissipationConstant;

[numthreads(1, 1, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;
		
		//inkTexture[gl_GlobalInvocationID] *= 0.992;
		//inkTexFinal[gl_GlobalInvocationID] *= 0.02;

		inkTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 0);
		inkTexFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 0);
		 
		//velocityFinal[gl_GlobalInvocationID] *= 0.04;
		//velocityTexture[gl_GlobalInvocationID] *= 0.996;

		divergenceTexture[gl_GlobalInvocationID] *= 0.96;

		//velocityFinal[gl_GlobalInvocationID] *= 1.0002;
		//velocityTexture[gl_GlobalInvocationID] *= 1.0002;

		//velocityFinal[gl_GlobalInvocationID] *= velocityDissipationConstant;
		//velocityTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 0);

	}
}