#if 0
	***	threads_per_group_x	16
	***	fast_shader_build_hack	1
	***	glsl	635204550
	***	threads_per_group_y	16
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

RWStructuredBuffer<uint> pixelBuffer : register(u0); //changing the type from uint to float does something funky
RWStructuredBuffer<uint> leapMotionBuffer : register(u1);

uniform uint2 texResolution;

uint packUnorm4x8( float4 value )
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	uint w = uint(saturate(value.w) * 255.0f);
	
	return x | (y << 8u) | (z << 16u ) | (w << 24u);
}

[numthreads(16, 16, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y )
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;
		pixelBuffer[idx] = packUnorm4x8( float4( float2(gl_LocalInvocationID.xy) / 16.0f, 0.0f, 1.0f ) );
		//pixelBuffer[idx] = float4(255.0f, 0.0f, 0.0f, 1.0f);
	}
}
