RWTexture3D<float4> velocityTextureWrite	: register(u0);
RWTexture3D<float4> inkTextureWrite			: register(u1);
Texture3D<float4> velocityTextureRead		: register(t0);
Texture3D<float4> inkTextureRead			: register(t1);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

uniform float timeSinceLast;
uniform float reciprocalDeltaX;

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

		float3 velocity = velocityTextureRead.SampleLevel(TextureSampler, idx / width, 0);
		
		float3 idxBackInTime = idx - (timeSinceLast * reciprocalDeltaX * velocity);

		float3 v = velocityTextureWrite[idx];
		float3 i = inkTextureWrite[idx];

		// RWTexture3D objects do not expose the Sampling functions :(
		// Fortunately I seem to be able to both read and write using the same texture but through different registers
		// Which... sounds wrong and bad but whatever

		velocityTextureWrite[idx] = velocityTextureRead.SampleLevel(TextureSampler, idxBackInTime / width, 0);
		inkTextureWrite[idx] = inkTextureRead.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		//velocityTexture[idx] = float3(i.x, v.y, v.z);
	}
}