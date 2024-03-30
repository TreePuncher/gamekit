struct InputRecord
{
	uint index;
};

[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeDispatchGrid(1, 1, 1)]
[NumThreads(1, 1, 1)]
void Main([MaxRecords(1)] NodeOutput<InputRecord> SecondNode)
{
    ThreadNodeOutputRecords<InputRecord> record = SecondNode.GetThreadNodeOutputRecords(1);
    record.Get().index = 0;
    record.OutputComplete();
}

[Shader("node")]
[NodeLaunch("broadcasting")]
[NodeDispatchGrid(1, 1, 1)]
[NumThreads(1, 1, 1)]
void SecondNode(DispatchNodeInputRecord<InputRecord> inputData)
{
}
