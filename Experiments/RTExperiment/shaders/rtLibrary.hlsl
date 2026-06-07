[[fk::BeginRootSignatureDef(id=rootsig1)]]
[[fk::RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)]]
[[fk::DescriptorSet(SRV(num=1))]]


struct Attributes
{
    
};

struct Payload
{
    
};

[shader("anyhit")]
void anyhit_main(inout Payload payload, in Attributes attr)
{
}


[shader("closesthit")]
void closesthit_main(inout Payload payload, in Attributes attr)
{
}

[shader("miss")]
void miss_main(inout Payload payload)
{
}


ConstantBuffer<SceneConstantStructure> SceneConstants;
[[fk::AccelerationStructure(binding = 0, set = 2, id = AccelerationStructure)]]

[shader("raygeneration")]
void raygen_main()
{
    
}
