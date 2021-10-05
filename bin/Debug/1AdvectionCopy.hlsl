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
RWTexture3D<float4> velocityWrite		: register(u0);	// secondaryVelocityTexture
RWTexture3D<float4> inkWrite			: register(u1);	// secondaryInkTexture
Texture3D<float4> velocityRead			: register(t0); // primaryVelocityTexture
Texture3D<float4> inkRead				: register(t1);	// primaryInkTexture

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

uniform float reciprocalDeltaX;

[numthreads(8, 8, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		int4 idx4 = int4(gl_GlobalInvocationID, 0);
		int3 idx3 = int3(gl_GlobalInvocationID);

		float width = texResolution.x;

		float4 velocity = velocityRead.Load(idx4);
		float4 ink = inkRead.Load(idx4);
		
		// to sample, or not to sample?
		//float4 ink = inkRead.SampleLevel(TextureSampler, gl_GlobalInvocationID / width, 0); 

		velocityWrite[idx3] = velocity;
		inkWrite[idx3] += ink;
	}
}