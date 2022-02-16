Texture2D<float4> SourceBuffer     : register(t0);
Texture2D<float4> NormalBuffer     : register(t1); // metallic, roughness, IOR, anisotropic
Texture2D<float>  DepthBuffer      : register(t2);
Texture2D<float2> SobelBuffer      : register(t3);
Texture2D<float4> testImage        : register(t4);

cbuffer LocalConstants : register(b0)
{
}

struct Blur_PS_IN
{
    float4 Position   : SV_Position;
    float2 PixelCoord : PIXELCOORD;
    float2 UV         : UVCOORD;
};

struct Sobel_PS_IN
{
    float4 Position   : SV_Position;
    float2 PixelCoord : PIXELCOORD;
    float2 UV         : UVCOORD;
};

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in float3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

float Gaussian2D(in float2 p, in float sigma)
{
    float sigma2 = sigma*sigma;
    return (1.0/(2.0 * 3.14159 * sigma2)) * exp(-dot(p,p)/(2.0*sigma2));
}

#define KERNEL_SIZE 8
#define SIGMA 0.7
#define BSIGMA 0.1
/*
float4 SobelFilterHorizontal_PS(Sobel_PS_IN input): SV_TARGET
{
    
}

float4 SobelFilterVertical_PS(Sobel_PS_IN input): SV_TARGET
{
    
}*/

struct HorizontalOut
{
    float4 Colour : SV_Target0;
    float2 Sobel  : SV_Target1;
};

float ColorStrength(in float3 col)
{
    return (col.r + col.g + col.b) * 0.33333;
}

HorizontalOut BilateralBlurHorizontal_PS(float4 position : SV_Position)
{
    HorizontalOut output;

    uint3 coord = uint3(position.xy, 0);
    float depth = DepthBuffer.Load(coord).r;

    float dp0 = DepthBuffer.Load(uint3(coord.x - 1, coord.y, 0)).x;
    float dp1 = DepthBuffer.Load(uint3(coord.x, coord.y, 0)).x;
    float dp2 = DepthBuffer.Load(uint3(coord.x + 1, coord.y, 0)).x;
    float3 nm0 = NormalBuffer.Load(uint3(coord.x - 1, coord.y, 0)).xyz;
    float3 nm1 = NormalBuffer.Load(uint3(coord.x, coord.y, 0)).xyz;
    float3 nm2 = NormalBuffer.Load(uint3(coord.x + 1, coord.y, 0)).xyz;
    //float dnm0 = length(nm1)

    output.Sobel.x = dp0 - dp2;

    output.Sobel.y = dp0 + 2.0 * dp1 + dp2; /*ColorStrength(SourceBuffer.Load(uint3(coord.x - 1, coord.y, 0)).rgb)
        + ColorStrength(2.0 * SourceBuffer.Load(uint3(coord.x,coord.y,0)).rgb)
        + ColorStrength(SourceBuffer.Load(uint3(coord.x+1, coord.y, 0)).rgb);*/
    /*
    float3 color     = float3(0, 0, 0);
    uint2 pixelCoord = uint2(input.PixelCoord);
    
    float3 c = float3(0, 0, 0);
    const float HALF_KERNEL_SIZE = float(KERNEL_SIZE) * 0.5;
    //float depthDiff = 0.0;
    float3 normalDiff = float3(0.0, 0.0, 0.0);

    for(uint i = 0; i < KERNEL_SIZE; ++i)
    {
        uint2 sampleCoord = pixelCoord + uint2(i - HALF_KERNEL_SIZE, 0);
        float k = i/HALF_KERNEL_SIZE - 1.0;
        //float di = DepthBuffer.Load(uint3(sampleCoord, 0)).x;
        float3 ni = NormalBuffer.Load(uint3(sampleCoord, 0)).xyz;
        //depthDiff += di * k;
        normalDiff += ni * k;
    }

    float diff = length(normalDiff);
    c = float3(diff, diff, diff);*/

    float3 c = 0;
    if(depth == 1.0f)
    {
        c = SourceBuffer.Load(coord).rgb;
    }
    else
    {
        
        float k[] = { 0.0545, 0.2442, 0.4026, 0.2442, 0.0545 };
        for (uint x = 0; x < 5; x++)
        {
            uint2 pos = coord.xy + uint2(x - 2, 0);
            c += k[x] * SourceBuffer.Load(uint3(pos, 0)).rgb;
        }
    }

    output.Colour  = float4(c, 1);
    return output;//float4(c, 1.0);
}


float4 BilateralBlurVertical_PS(float4 position : SV_Position) : SV_TARGET
{   
    #if 0
    uint3 coord = uint3(position.xy, 0);
    float depth = DepthBuffer.Load(coord).x;


    float2 vs0 = SobelBuffer.Load(uint3(coord.x, coord.y - 1, 0)).xy;
    float2 vs1 = SobelBuffer.Load(uint3(coord.x, coord.y, 0)).xy;
    float2 vs2 = SobelBuffer.Load(uint3(coord.x, coord.y + 1, 0)).xy;

    float Gx = vs0.x + 2 * vs1.x + vs2.x;
    float Gy = vs0.y - vs2.y;

    float G = sqrt(Gx * Gx + Gy * Gy);
    float A = 1.0 - max(G, float(depth == 1.0));

    return float4(SourceBuffer.Load(coord).xyz, 1);

    #else

    float3 color = float3(0, 0, 0);
    uint2 pixelCoord = uint2(position.xy);

    float depth = DepthBuffer.Load(uint3(pixelCoord, 0)).r;

    float k[] = { 0.0545, 0.2442, 0.4026, 0.2442, 0.0545 };
    
    float3 c = float3(0, 0, 0);
    if (depth == 1.0f)
    {
        c = SourceBuffer.Load(uint3(pixelCoord, 0)).rgb;
    }
    else
    {
        for (uint y = 0; y < 5; y++)
        {
            uint2 pos = pixelCoord + uint2(0, y - 2);
            c += k[y] * SourceBuffer.Load(uint3(pos, 0)).rgb;
        }
    }
    
    return float4(c, 1.0);// * testImage.Load(uint3(pixelCoord % uint2(2048, 2048), 0));
    //return testImage.Load(uint3((pixelCoord) % uint2(2048, 2048), 0));

    #endif
}
