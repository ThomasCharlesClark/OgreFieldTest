RWTexture3D<float3> divergenceTexture : register(u0);
Texture3D<float3> velocityTexture : register(t0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float timeSinceLast;
uniform float halfDeltaX;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x > 0 &&
		gl_GlobalInvocationID.x < texResolution.x - 1 &&
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y - 1)
	{
		float3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float width = texResolution.x;

		float3 a = velocityTexture.SampleLevel(TextureSampler, float3(idx.x - 1, idx.y,	 idx.z) / width, 0);
		float3 b = velocityTexture.SampleLevel(TextureSampler, float3(idx.x + 1, idx.y,	 idx.z) / width, 0);
		float3 c = velocityTexture.SampleLevel(TextureSampler, float3(idx.x,	  idx.y - 1, idx.z) / width, 0);
		float3 d = velocityTexture.SampleLevel(TextureSampler, float3(idx.x,	  idx.y + 1, idx.z) / width, 0);

		float3 div = float3(
			((a.x - b.x) + (c.y - d.y)) * halfDeltaX,
			0,
			0);

		divergenceTexture[idx] = div;
	}
}