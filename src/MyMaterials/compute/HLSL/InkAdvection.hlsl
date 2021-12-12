RWTexture3D<float> inkTexFinal			: register(u0);
Texture3D<float> inkTexture				: register(t0);
Texture3D<float4> velocityTexture		: register(t1);

SamplerState TextureSampler
{
	Filter = ANISOTROPIC;
	AddressU = Clamp;
	AddressV = Clamp;
};

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
	if (gl_GlobalInvocationID.x > 0 &&
		gl_GlobalInvocationID.x < texResolution.x &&
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y)
	{
		int3 idx3 = int3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		int4 idx4 = int4(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, 0);

		float width = texResolution.x;

		float4 velocity = velocityTexture.Load(idx4);

		float3 idxBackInTime = (idx3 - (velocity));

		float4 v = velocityTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		//float i = inkTexture.Load(int4(idxBackInTime, 0));
		float i = inkTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);
		
		inkTexFinal[idx3] = i;
	}
}