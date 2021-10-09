RWTexture3D<float4> inkTexture				: register(u0);
Texture3D<float4>	inkTextureFinal			: register(t0);

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
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
		int3 idx3 = int3(gl_GlobalInvocationID);

		float width = texResolution.x;

		float4 ink = inkTextureFinal.SampleLevel(TextureSampler, gl_GlobalInvocationID / width, 0); 

		inkTexture[idx3] = ink;
	}
}