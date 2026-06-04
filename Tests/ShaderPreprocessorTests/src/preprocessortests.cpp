#include <BuildSettings.hpp>
#include <expected>
#include <string>
#include <format>
#include <ShaderPreprocessor.hpp>
#include <fmt/format.h>
#include <scn/scan.h>
#include <scn/xchar.h>
#include <scn/scan.h>
#include <scn/regex.h>
#include <cctype>

using namespace FlexKit;
using namespace std;


static bool RootSignatureDescriptorSetTest()
{
    std::string exampleShader = R"(
[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(CBV(num=unbounded) CBV( num = 10, flags = static, visibility=pixel|vertex) CBV(num=3, flags=ASDF))]]
[[fk::DescriptorSet(CBV(num=10, flags=static , visibility = pixel | vertex) SRV(num=3) UAV(num=4))]]

[[fk::PushConstants(num=4)]] 
{
    float time;
};

struct VIN
{
	[[vk::location(0)]] float3 xyz : POSITION;
};

struct VOut
{
	[[vk::location(0)]] float4 position : SV_Position;
	[[vk::location(1)]] float3 uvw : UVW;
};

VOut VMain(VIN vin, uint vertexID : SV_VertexID)
{
	VOut OUT;
	OUT.position	= float4(vin.xyz, 1);
	OUT.uvw			= vin.xyz / 2.0f + 0.5f;

	return OUT;
}

[[fk::RootSignature(id=rootsig1)]]
float4 PMain() : SV_Target
{
	return
        float4(
            sin(time) / 2.0f + 0.5f,
            cos(time) / 2.0f + 0.5f,
            vin.position.y / 553.0f,
            1.0f);
}

)";

    auto preprocessResults = DXShaderProprocessor(exampleShader, SHADER_TYPE::Unknown);

    return true;
}

int main(const int args, const char* argv[])
{
    FK_ASSERT(RootSignatureDescriptorSetTest());
    return 0;
}
