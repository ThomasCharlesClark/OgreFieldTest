struct Particle 
{
	float4 colour;
	float3 velocity;
	float pressure;
	float3 pressureGradient;
};

RWStructuredBuffer<uint> pixelBuffer : register(u0);
RWStructuredBuffer<Particle> otherBuffer : register(u1);
RWTexture3D<float4> velocityTexture : register(u2);
RWTexture3D<float4> inkTexture : register(u3);

uniform uint2 texResolution;

uniform float timeSinceLast;

uint packUnorm4x8( float4 value )
{
	uint x = uint(saturate(value.x) * 255.0f);
	uint y = uint(saturate(value.y) * 255.0f);
	uint z = uint(saturate(value.z) * 255.0f);
	//uint z = uint(value.z);
	uint w = uint(saturate(value.w) * 255.0f);
	
	return x | (y << 8u) | (z << 16u ) | (w << 24u);
}

[numthreads(@value( threads_per_group_x ), @value( threads_per_group_y ), @value( threads_per_group_z ))]
void main
(
    uint3 gl_LocalInvocationID : SV_GroupThreadID,
    uint3 gl_GlobalInvocationID : SV_DispatchThreadId
)
{
	if( gl_GlobalInvocationID.x < texResolution.x && gl_GlobalInvocationID.y < texResolution.y )
	{
		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		float3 i = inkTexture.Load(gl_GlobalInvocationID);

		float3 v = velocityTexture.Load(gl_GlobalInvocationID);

		//pixelBuffer[idx] = packUnorm4x8(float4(i, 1.0f));

		//pixelBuffer[idx] = packUnorm4x8(float4(v, 1.0f));

		pixelBuffer[idx] = packUnorm4x8(float4(i + v, 1.0f));
	}
}