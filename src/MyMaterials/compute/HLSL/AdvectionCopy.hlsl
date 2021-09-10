RWTexture3D<float4> previousVelocityWrite		: register(u0);
Texture3D<float4> velocityTextureRead			: register(t0);

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
	float tsl = timeSinceLast * 2;

	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, 0);

		float4 velocity = velocityTextureRead.Load(int4(idx, 0));

		previousVelocityWrite[idx] = velocity;

		//velocityTexture[idx] = float3(i.x, v.y, v.z);
	}
}