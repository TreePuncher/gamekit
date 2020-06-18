const float4 Vertices[] = 
 {
    float4(-1,  1, 0, 1),
    float4( 1,  1, 0, 1),
    float4(-1, -1, 0, 1),

    float4(-1, -1, 0, 1),
    float4( 1,  1, 0, 1),
    float4( 1, -1, 0, 1),
};

float4 FullscreenQuad(uint vIdx : SV_VERTEXID) : SV_POSITION
{
    return Vertices[vIdx % 6];
}