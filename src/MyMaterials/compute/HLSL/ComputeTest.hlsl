struct Particle 
{
	float4 colour;
};

RWStructuredBuffer<uint> pixelBuffer : register(u0);
RWStructuredBuffer<Particle> otherBuffer : register(u1); //changing the type from uint to float does something funky
//RWStructuredBuffer<uint> leapMotionBuffer : register(u1);

uniform uint2 texResolution;

uniform float mBlueBleedOff = 0.9997;

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
		/*uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;
		pixelBuffer[idx] = packUnorm4x8(float4(float2(gl_LocalInvocationID.xy) / 8.0f, otherBuffer[0].colour.x,
			1.0f));*/

		//	//1.0f - ((1.0f / texResolution.x) * gl_GlobalInvocationID.x)));
		//	//(1.0f / texResolution.x) * gl_GlobalInvocationID.x ));

		uint idx = gl_GlobalInvocationID.y * texResolution.x + gl_GlobalInvocationID.x;

		//otherBuffer[idx].colour.z *= mBlueBleedOff;

		pixelBuffer[idx] = packUnorm4x8(otherBuffer[idx].colour);
		//otherBuffer[idx].colour.z = 0.0;
	}
}
