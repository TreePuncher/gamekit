struct NodeInput
{
	uint3 X : SV_DispatchGrid;
};

[Shader("node")]
[NodeLaunch("broadcasting")]
[NumThreads(1,1,1)]
[NodeMaxDispatchGrid(1,1,1)]
void Main(
	uint3 DTid : SV_DispatchThreadID,
	 DispatchNodeInputRecord<NodeInput> input)
{

}
