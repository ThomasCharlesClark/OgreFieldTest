RWTexture3D<float4> target			: register(u0);
Texture3D<float4>	source			: register(t0);

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
	int4 idx4 = int4(gl_GlobalInvocationID, 0);
	int3 idx3 = int3(gl_GlobalInvocationID);

	float width = texResolution.x;

	//float4 value = source.SampleLevel(TextureSampler, gl_GlobalInvocationID / width, 0);
		
	float4 value = source.Load(idx4);

	target[idx3] = value;
	//target[idx3] = float4(-0.3f, 0.75f, 0, 0);
}