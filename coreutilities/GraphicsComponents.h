#ifndef GRAPHICSCOMPONENTS_H
#define GRAPHICSCOMPONENTS_H

/**********************************************************************

Copyright (c) 2017 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/


namespace FlexKit
{
	struct FLEXKITAPI SceneNodeComponentSystem
	{
	public:
		void Initiate(byte* Memory, size_t BufferSize)
		{
			InitiateSceneNodeBuffer(Memory, BufferSize);
			Root = FlexKit::GetZeroedNode();
		}

		// Interface Methods
		void Release();

		NodeHandle GetRoot();
		NodeHandle GetNewNode();
		NodeHandle GetZeroedNode();

		NodeHandle Root;

		void SetParentNode(NodeHandle Parent, NodeHandle Node)
		{
			FlexKit::SetParentNode(Parent, Node);
		}

		float3 GetLocalScale(NodeHandle Node)
		{
			return FlexKit::GetLocalScale(Node);
		}

		float3 GetPositionW(NodeHandle Node)
		{
			return FlexKit::GetPositionW(Node);
		}

		float3 GetPositionL(NodeHandle Node)
		{
			return FlexKit::GetPositionL(Node);
		}

		void SetPositionW(NodeHandle Node, float3 xyz)
		{
			FlexKit::SetPositionW(Node, xyz);
		}

		void SetPositionL(NodeHandle Node, float3 xyz)
		{
			FlexKit::SetPositionL(Node, xyz);
		}

		Quaternion GetOrientation(NodeHandle Node)
		{
			return FlexKit::GetOrientation(Node);
		}

		operator SceneNodeComponentSystem* () { return this; }
	};

	SceneNodeComponentSystem	SceneNodes;
}// FlexKit

#endif // GRAPHICSCOMPONENTS_H