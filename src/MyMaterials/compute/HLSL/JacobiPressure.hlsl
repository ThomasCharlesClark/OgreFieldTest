RWTexture3D<float4> pressureTexture : register(u0);
Texture3D<float4> divergenceRead : register(t0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float halfDeltaX;

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if (gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		int3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float alpha = -halfDeltaX * halfDeltaX;
		float rBeta = 0.25;
		float width = texResolution.x;
		float3 beta = divergenceRead.SampleLevel(TextureSampler, idx / width, 1.0);

		float4 a = pressureTexture.Load(float4(idx.x - 1, idx.y, idx.z, 0));
		float4 b = pressureTexture.Load(float4(idx.x + 1, idx.y, idx.z, 0));
		float4 c = pressureTexture.Load(float4(idx.x, idx.y - 1, idx.z, 0));
		float4 d = pressureTexture.Load(float4(idx.x, idx.y + 1, idx.z, 0));

		float4 p = float4(
			(a.x + b.x + c.x + d.x + alpha * beta.x) * rBeta, 
			(a.x + b.x + c.x + d.x + alpha * beta.x) * rBeta, 
			(a.x + b.x + c.x + d.x + alpha * beta.x) * rBeta, 
			0);

		pressureTexture[idx] = p; // float3(0.0, 0.4, 0.0);
	}
}