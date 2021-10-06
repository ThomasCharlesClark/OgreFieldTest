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
RWTexture3D<float4> inkWrite		: register(u0); // primaryInkTexture
RWTexture3D<float4> inkRead			: register(u1); // secondaryInkTexture
RWTexture3D<float> inkTemp			: register(u2); // tempInkTexture

Texture3D<float4> velocityRead		: register(t0); // primaryVelocityTexture

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

float4 unpackUnorm4x8(uint value)
{
	float4 retVal;
	retVal.x = float(value & 0xFF);
	retVal.y = float((value >> 8u) & 0xFF);
	retVal.z = float((value >> 16u) & 0xFF);
	retVal.w = float((value >> 24u) & 0xFF);

	return retVal * 0.0039215687f;
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
		float3 idx3 = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;	

		float4 velocity = velocityRead.SampleLevel(TextureSampler, idx3 / width, 0) * 100;
		//float4 velocity = velocityRead.Load(int4(idx3, 0));// *100;

		float3 idxBackInTime = (idx3 - (reciprocalDeltaX * velocity.xyz));
		
		//float4 inkColour = inkRead.SampleLevel(TextureSampler, idxBackInTime / width, 0);
		////float4 inkColour = inkRead.Load(idxBackInTime);

		//inkWrite[idx3] = float4(inkColour.xyz * inkDissipationConstant, 1.0);

		////inkRead[idxBackInTime] = inkWrite[idx3];
		////inkRead[idxBackInTime] = float4(0,0,0,1);

		//float i = inkTemp.Load(float4(idxBackInTime, 1));

		//inkTemp[idx3] = i;


		float4 i = inkRead.Load(idxBackInTime);

		inkWrite[idx3] = float4(i.xyz, 1.0) * 0.64;

		//inkRead[idxBackInTime] = float4(0, 0, 0, 1);// inkWrite[idx3];
	}
}