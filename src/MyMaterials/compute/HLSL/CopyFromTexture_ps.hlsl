RWTexture3D<float4> read : register(u1);
SamplerState samplerState[2]: register(s0);

struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
};

SamplerState TextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

uniform uint2 texResolution;

float4 unpackUnorm4x8( uint value )
{
	float4 retVal;
	retVal.x = float(value & 0xFF);
	retVal.y = float((value >>  8u) & 0xFF);
	retVal.z = float((value >> 16u) & 0xFF);
	retVal.w = float((value >> 24u) & 0xFF);

	return retVal * 0.0039215687f; // 1.0 / 255.0f;
}

float4 main(PS_INPUT inPs, float4 gl_FragCoord : SV_Position) : SV_Target
{
	//uint idx = uint(gl_FragCoord.y) * texResolution.x + uint(gl_FragCoord.x);

	float4 fragColour = read.Load(gl_FragCoord.xyz);// (TextureSampler, float3(gl_FragCoord.xyz), 1.0);

	//fragColour += float4(0.6, 0, 0, 1.0);
	
	return fragColour;
}
