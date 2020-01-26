cbuffer constants : register(b0)
{
    uint2 WH;
};

Texture2D<float4> font  : register(t0);
sampler BiLinear        : register(s0); // Nearest point
sampler NearestPoint    : register(s1); // Nearest point


struct ImDrawVert
{
    float2  pos : POSITION;
    float2  uv  : TEXCOORD;
    float4  col : COLOR;
};

struct PS_Point
{
    float4  pos : SV_POSITION;
    float2  uv  : TEXCOORD;
    float4  col : COLOR;
};

PS_Point ImGui_VS(ImDrawVert vert)
{
    PS_Point output;
    output.pos  = float4(float2(-1, 1) + float2(vert.pos) / float2(WH) * float2(2, -2), 0, 1);
    output.uv   = vert.uv;
    output.col  = vert.col;
    return output;
}

float4 ImGui_PS(PS_Point fragment) : SV_TARGET
{
    float4 sample = font.Sample(NearestPoint, fragment.uv);
    return fragment.col * sample;
}
