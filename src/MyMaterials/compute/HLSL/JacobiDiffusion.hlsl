RWTexture3D<float4> velocityTexture : register(u0);

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
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y)
	{
		int3 idx = float3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		float4 a = velocityTexture.Load(float4(idx.x - 1, idx.y, idx.z, 0));
		float4 b = velocityTexture.Load(float4(idx.x + 1, idx.y, idx.z, 0));
		float4 c = velocityTexture.Load(float4(idx.x, idx.y - 1, idx.z, 0));
		float4 d = velocityTexture.Load(float4(idx.x, idx.y + 1, idx.z, 0));

		float alpha = halfDeltaX * halfDeltaX / (2.865 * timeSinceLast);
		float rBeta = 1 / (4 + alpha);
		float4 beta = velocityTexture.Load(float4(idx.xyz, 0));


		float4 vVel = (a + b + c + d + alpha * beta) * rBeta;

		velocityTexture[idx] = vVel;
	}
}