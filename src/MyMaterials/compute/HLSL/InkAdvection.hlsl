RWTexture3D<float4> velocityWrite	: register(u0);
RWTexture3D<float4> inkWrite		: register(u1);
Texture3D<float4> velocityRead		: register(t0);
Texture3D<float4> inkRead			: register(t1);

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

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;

		float4 velocity = velocityRead.Load(float4(idx, 0));

		float3 idxBackInTime = (idx - (timeSinceLast * reciprocalDeltaX * velocity.xyz));
		
		//float4 i = inkWrite[idxBackInTime];// inkRead.Load(float4(idxBackInTime, 0));
		float4 i = inkRead.Load(float4(idxBackInTime, 0));

		inkWrite[idx] = float4(i.xyz * inkDissipationConstant, 1.0);
	}
}