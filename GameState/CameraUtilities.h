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

#ifndef CAMERAUTILITIES
#define CAMERAUTILITIES

#include "../graphicsutilities/graphics.h"

using FlexKit::NodeHandle;
using FlexKit::SceneNodes;
using FlexKit::Camera;

struct Camera3rdPersonContoller
{
	NodeHandle Yaw_Node;
	NodeHandle Pitch_Node;
	NodeHandle Roll_Node;

	float Yaw, Pitch, Roll;

	SceneNodes*	Nodes;
	Camera*		C;
};

void InitiateCamera3rdPersonContoller( SceneNodes* Nodes, Camera* C, Camera3rdPersonContoller* Out );
void UpdateCameraController( SceneNodes* Nodes, Camera3rdPersonContoller* Controller, double dT );

void SetCameraOffset	(Camera3rdPersonContoller* Controller, float3 xyz);
void SetCameraPosition	(Camera3rdPersonContoller* Controller, float3 xyz);


void TranslateCamera	(Camera3rdPersonContoller* Controller, float3 xyz);
void YawCamera			(Camera3rdPersonContoller* Controller, float Degree);
void PitchCamera		(Camera3rdPersonContoller* Controller, float Degree);
void RollCamera			(Camera3rdPersonContoller* Controller, float Degree);

float3 GetForwardVector	(Camera3rdPersonContoller* Controller);
float3 GetRightVector	(Camera3rdPersonContoller* Controller);


#endif