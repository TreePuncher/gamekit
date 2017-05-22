/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef FORWARDRENDERING_H
#define FORWARDRENDERING_H

#include "..\buildsettings.h"
#include "graphics.h"


namespace FlexKit
{
	/************************************************************************************************/


	struct ForwardRender
	{
		RenderWindow*				RenderTarget;
		DepthBuffer*				DepthTarget;
		ID3D12GraphicsCommandList*	CommandList;
		ID3D12CommandAllocator*		CommandAllocator;
		ID3D12PipelineState*		PSO;
		ID3D12RootSignature*		PassRTSig;
		ID3D12DescriptorHeap*		CBDescHeap;

		Shader VShader;
		Shader PShader;
	};


	struct ForwardPass_DESC
	{
		DepthBuffer*	DepthBuffer;
		RenderWindow*	OutputTarget;
	};


	FLEXKITAPI void InitiateForwardPass	( RenderSystem* RenderSystem, ForwardPass_DESC* GBdesc, ForwardRender* out );
	FLEXKITAPI void ForwardPass			( PVS* _PVS, ForwardRender* Pass, RenderSystem* RS, Camera* C, float4& ClearColor, PointLightBuffer* PLB, GeometryTable* GT );
	FLEXKITAPI void ReleaseForwardPass	( ForwardRender* FP );


	/************************************************************************************************/
}

#endif