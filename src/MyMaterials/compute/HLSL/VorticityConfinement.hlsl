Texture3D<float> vorticityTexture		: register(t0);
RWTexture3D<float3> velocityTexture		: register(u0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;
uniform float halfDeltaX;
uniform float vorticityConfinementScale;
uniform float timeSinceLast;

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

		float a = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x - 1, idx.y, idx.z) / width, 0);
		float b = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x + 1, idx.y, idx.z) / width, 0);
		float c = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x, idx.y - 1, idx.z) / width, 0);
		float d = vorticityTexture.SampleLevel(TextureSampler, float3(idx.x, idx.y + 1, idx.z) / width, 0);

		float3 force = float3(abs(c) - abs(d), 0.0f, abs(b) - abs(a)) * halfDeltaX;

		force = normalize(force);

		force *= vorticityConfinementScale * vorticityTexture[idx] * float3(1, 0, -1);

		velocityTexture[idx] += force * timeSinceLast;
	}
}