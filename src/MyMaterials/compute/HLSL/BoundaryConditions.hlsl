RWTexture3D<float3> velocityWrite : register(u0);
RWTexture3D<float3> inkWrite : register(u1);
Texture2D<float3> velocityRead : register(t0);
Texture2D<float3> inkRead : register(t1);

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
		
	}
}