#if 0
	***	threads_per_group_x	8
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	8
	***	threads_per_group_z	1
	***	hlms_high_quality	0
	***	typed_uav_load	1
	***	num_thread_groups_y	4
	***	glsles	1070293233
	***	hlslvk	1841745752
	***	syntax	-334286542
	***	metal	-1698855755
	***	num_thread_groups_z	1
	***	glslvk	-338983575
	***	hlsl	-334286542
	***	num_thread_groups_x	4
	DONE DUMPING PROPERTIES
	DONE DUMPING PIECES
#endif
struct Particle
{
	float ink;
	float4 colour;
	float3 velocity;
}; 

RWTexture3D<float4> inkTexture					: register(u0);
RWTexture3D<float4> inkTextureFinal				: register(u1);
RWTexture3D<float4> velocityTexture				: register(u2);
RWTexture3D<float4> velocityFinal				: register(u3);

uniform uint2 texResolution;

[numthreads(8, 8, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		//handInputBuffer[rwIdx].ink = 0.0;
		//handInputBuffer[rwIdx].colour *= 0.98;// float4(0, 0, 0, 1);
		//handInputBuffer[rwIdx].velocity *= 0.98;// float3(0, 0, 0);

		//inkTextureFinal[gl_GlobalInvocationID] = float4(inkTextureFinal[gl_GlobalInvocationID].xyz * 0.97, 1.0);

		//inkTexture[gl_GlobalInvocationID] *= 0.98; // float4(0, 0, 0, 1);

		inkTexture[gl_GlobalInvocationID] *= 0.998;
		//inkTextureFinal[gl_GlobalInvocationID] *= 0.98;

		//inkTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 1);
		inkTextureFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 1);

		velocityFinal[gl_GlobalInvocationID] *= 0.99;
		velocityTexture[gl_GlobalInvocationID] *= 0.99;

		//velocityFinal[gl_GlobalInvocationID] *= 0.9997;
		//velocityTexture[gl_GlobalInvocationID] *= 0.9997;

		//velocityFinal[gl_GlobalInvocationID] = float4(0, 0, 0, 1);
		//velocityTexture[gl_GlobalInvocationID] = float4(0, 0, 0, 1);
	}
}