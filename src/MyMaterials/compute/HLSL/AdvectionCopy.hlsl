RWTexture3D<float4> velocityWrite		: register(u0);	// secondaryVelocityTexture
RWTexture3D<float4> inkWrite			: register(u1);	// secondaryInkTexture
Texture3D<float4> velocityRead			: register(t0); // primaryVelocityTexture
Texture3D<float4> inkRead				: register(t1);	// primaryInkTexture

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
		int3 idx3 = int3(gl_GlobalInvocationID);

		float width = texResolution.x;

		float4 velocity = velocityRead.Load(idx4);
		
		// to sample, or not to sample?
		//float4 ink = inkRead.Load(idx4);
		float4 ink = inkRead.SampleLevel(TextureSampler, gl_GlobalInvocationID / width, 0); 

		velocityWrite[idx3] = velocity;
		inkWrite[idx3] = ink;
	}
}