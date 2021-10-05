RWTexture3D<float4> inkWrite		: register(u0); // primaryInkTexture
RWTexture3D<float4> inkRead			: register(u1); // secondaryInkTexture
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

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
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

		float3 idxBackInTime = (idx3 - (reciprocalDeltaX * velocity.xyz));
		
		float4 i = inkRead.Load(idxBackInTime);

		inkWrite[idx3] = float4(i.xyz * inkDissipationConstant, 1.0);

		inkRead[idxBackInTime] = inkWrite[idx3];
	}
}