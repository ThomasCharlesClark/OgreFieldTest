RWTexture3D<float4> previousVelocityWrite		: register(u0);
RWTexture3D<float4> previousInkWrite			: register(u1);
Texture3D<float4> velocityTextureRead			: register(t0);
Texture3D<float4> inkTextureRead				: register(t1);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

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
		int4 idx4 = int4(gl_GlobalInvocationID, 0);

		float4 velocity = velocityTextureRead.Load(idx4);
		float4 ink = inkTextureRead.Load(idx4);

		previousVelocityWrite[gl_GlobalInvocationID] = velocity;
		previousInkWrite[gl_GlobalInvocationID] = ink;
	}
}