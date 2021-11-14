//RWTexture3D<float4> inkTextureFinal		: register(u0);
RWTexture3D<float4> inkTexture			: register(u0);
Texture3D<float4> inkTextureSampler		: register(t0);
Texture3D<float4> velocityTexture		: register(t1);

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
	if (gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx3 = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;

		//float4 velocity = velocityTexture.SampleLevel(TextureSampler, idx3 / width, 0);

		float4 velocity = velocityTexture.Load(int4(idx3, 0));

		//float3 idxBackInTime = (idx3 - 1 / (timeSinceLast * reciprocalDeltaX * velocity.xyz));

		//float3 idxBackInTime = (idx3 - (timeSinceLast * reciprocalDeltaX * velocity.xyz));

		//float3 idxBackInTime = (idx3 - (reciprocalDeltaX * velocity.xyz));

		float3 idxBackInTime = (idx3 - (velocity.xyz));

		// at this point in time, inkTexture should contain the previous
		// state of affairs
		float4 i = inkTextureSampler.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		//float4 i = inkTexture.Load(int4(idxBackInTime, 0));

		//float4 i2 = inkTextureFinal.Load(int4(idxBackInTime, 0));

		inkTexture[idxBackInTime] = i;// float4(0, 0, 0, 1.0);

		//inkTexture[idx3] = float4(0, 0, 0, 1.0);

		//inkTextureFinal[idxBackInTime] = float4(0, 0, 0, 1.0);
	}
}