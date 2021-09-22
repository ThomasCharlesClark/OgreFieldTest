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
RWTexture3D<float4> velocityWrite			: register(u0); // primaryVelocityTexture
RWTexture3D<float4> inkWrite				: register(u1); // primaryInkTexture
Texture3D<float4> velocityRead				: register(t0); // secondaryVelocityTexture
Texture3D<float4> inkRead					: register(t1); // secondaryInkTexture

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uint packUnorm4x8(float4 value)
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	uint w = uint(saturate(value.w) * 255.0f);

	return x | (y << 8u) | (z << 16u) | (w << 24u);
}

uniform uint2 texResolution;

uniform float timeSinceLast;
uniform float reciprocalDeltaX;
uniform float velocityDissipationConstant;
uniform float inkDissipationConstant;

[numthreads(8, 8, 1)]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		uint rwIdx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float width = texResolution.x;

		float4 velocity = velocityRead.SampleLevel(TextureSampler, idx / width, 1.0);
		float4 ink = inkRead.SampleLevel(TextureSampler, idx / width, 1.0);

		float3 idxBackInTime = (idx - (timeSinceLast * reciprocalDeltaX * velocity.xyz));

		float4 v = float4(float3(velocityRead.SampleLevel(TextureSampler, idxBackInTime / width, 1.0).xyz) * velocityDissipationConstant, 1.0);
		float4 i = float4(float3(inkRead.SampleLevel(TextureSampler, idxBackInTime / width, 1.0).xyz) * inkDissipationConstant, 1.0);

		velocityWrite[idx] = v;
		inkWrite[idx] = i;
	}
}