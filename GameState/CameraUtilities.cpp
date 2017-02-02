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

#include "CameraUtilities.h"

using namespace FlexKit;

void InitiateCamera3rdPersonContoller(SceneNodes* Nodes, Camera* C, Camera3rdPersonContoller* Out)
{
	ReleaseNode(Nodes, C->Node);

	auto Yaw	= GetZeroedNode(Nodes);
	auto Pitch	= GetZeroedNode(Nodes);
	auto Roll	= GetZeroedNode(Nodes);

	SetParentNode(Nodes, Pitch, Roll);
	SetParentNode(Nodes, Yaw, Pitch);


	C->Node		= Pitch;
	Out->C		= C;
	Out->Nodes	= Nodes;

	Out->Pitch	= 0;
	Out->Roll	= 0;
	Out->Yaw	= 0;

	Out->Pitch_Node = Pitch;
	Out->Roll_Node  = Roll;
	Out->Yaw_Node   = Yaw;
}

void UpdateCameraController(SceneNodes* Nodes, Camera3rdPersonContoller* Controller, double dT)
{
	auto Temp2 = GetPositionW(Nodes, Controller->Yaw_Node);
	auto Temp = GetPositionW(Nodes, Controller->Pitch_Node);
	Quaternion Q;
	GetOrientation(Nodes, Controller->Roll_Node, &Q);
	SetOrientationL(Nodes, Controller->Yaw_Node,	Quaternion(0, Controller->Yaw, 0));
	SetOrientationL(Nodes, Controller->Pitch_Node,	Quaternion(Controller->Pitch, 0, 0));
	//SetOrientationL(Nodes, Controller->Roll_Node,	Quaternion(0, 0, Controller->Roll));
}

void SetCameraOffset(Camera3rdPersonContoller* Controller, float3 xyz)
{
	TranslateWorld(Controller->Nodes, Controller->Pitch_Node, {xyz});
}


void TranslateCamera(Camera3rdPersonContoller* Controller, float3 xyz)
{
	TranslateWorld(Controller->Nodes, Controller->Yaw_Node, xyz);
}

void YawCamera(Camera3rdPersonContoller* Controller, float Degree)
{
	Controller->Yaw += Degree;
}

void PitchCamera(Camera3rdPersonContoller* Controller, float Degree)
{
	Controller->Pitch += Degree;
}

void RollCamera(Camera3rdPersonContoller* Controller, float Degree)
{
	Controller->Roll += Degree;
}

float3 GetForwardVector(Camera3rdPersonContoller* Controller)
{
	Quaternion Q;
	GetOrientation(Controller->Nodes, Controller->Yaw_Node, &Q);
	return -(Q * float3(0, 0, 1)).normal();
}


float3 GetRightVector(Camera3rdPersonContoller* Controller)
{
	Quaternion Q;
	GetOrientation(Controller->Nodes, Controller->Yaw_Node, &Q);
	return -(Q * float3(-1, 0, 0)).normal();
}
