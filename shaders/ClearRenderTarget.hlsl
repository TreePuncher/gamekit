cbuffer ClearConstants : register(b1)
{
    uint4 clearValues;
}

uint4 ClearRenderTargetUINT2() : sv_target
{
    return clearValues;
}