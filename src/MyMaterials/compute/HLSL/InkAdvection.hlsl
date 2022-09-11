Texture3D<float4> velocityTexture			: register(t0);
Texture3D<float4> inkTexture				: register(t1);
RWTexture3D<float4> inkTexFinal				: register(u0);

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
		gl_GlobalInvocationID.x < texResolution.x - 1 &&
		gl_GlobalInvocationID.y > 0 &&
		gl_GlobalInvocationID.y < texResolution.y - 1)
	{
		int3 idx3 = int3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);

		int4 idx4 = int4(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z, 0);

		float width = texResolution.x;

		//float4 velocity = velocityTexture.Load(idx4);
		float4 velocity = velocityTexture.SampleLevel(TextureSampler, idx4 / width, 0) * 100;

		//velocity.x = 0;

		//float3 idxBackInTime = (idx3 - velocity);
		//float3 idxBackInTime = (idx3 - (timeSinceLast * reciprocalDeltaX * velocity));
		float3 idxBackInTime = (idx3 - (timeSinceLast * reciprocalDeltaX * velocity));

		float4 v = velocityTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);

		//float i = inkTexture.Load(int4(idxBackInTime, 0));
		float4 i = inkTexture.SampleLevel(TextureSampler, idxBackInTime / width, 0);
		
		//inkTexFinal[idx3] = normalize(v.x);
		inkTexFinal[idx3] = i;
	}
}