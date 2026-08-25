[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(SRVTexture(num=3))]]
[[fk::DescriptorSet(SRVTexture(num=1))]]


[[fk::Texture2D(id=MRIA,	binding=0, set=0, type=float4)]]
[[fk::Texture2D(id=Albedo,	binding=1, set=0, type=float4)]]
[[fk::Texture2D(id=Normal,	binding=2, set=0, type=float2)]]
[[fk::Texture2D(id=Depth,	binding=0, set=1, type=float)]]

sampler BiLinear		: register(s0); 
sampler NearestPoint	: register(s1);


float2 SignNotZero(float2 v)
{
	return float2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

float3 Decode(float2 e)
{
	float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
	if (v.z < 0)
		v.xy = (1.0 - abs(v.yx)) * SignNotZero(v.xy);

	return normalize(v);
}

[[fk::RootSignature(id=rootsig1)]]
float4 GBufferDebug(const float4 XY : SV_POSITION) : SV_TARGET
{
	const float d = Depth.Sample(NearestPoint, XY.xy / float2(800, 600));

	if ( d == 1.0f)
		return 0.0f;

	const float3 n		= Decode(Normal.Sample(NearestPoint, XY.xy / float2(800, 600))) * 2.0f + 0.5f;
	const float4 sample = Albedo.Sample(NearestPoint, XY.xy / float2(800, 600));
	return float4(float3(1.0f, 1.0f, 1.0f) * pow(saturate(d * 500.0f), 2.2f), 1.0f) + float4(n, 0.0f);
	
    //return float4(n * 2 + 0.5f, saturate(d * 1000.0f));
	//return float4(XY.x / 800.0f, XY.y / 600.0f, 0.0f, 0.0f);
	//return pow(float4(float(0xFC) / 255.0f, float(0x8E) / 255.0f, float(0xAC) / 255.0f, 0), 2.1f);
}
