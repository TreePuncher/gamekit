#pragma once

#include "MainMenu.h"

MainMenu::MainMenu(
	GameFramework*			IN_Framework,
	ConstantBufferHandle	IN_Constants, 
	VertexBufferHandle		IN_VertexBuffer, 
	VertexBufferHandle		IN_TextBuffer) : 
		FrameworkState	{IN_Framework},
		Constants		{IN_Constants},
		VertexBuffer	{IN_Constants},
		TextBuffer		{IN_TextBuffer}
{

}

bool  MainMenu::Update(EngineCore* Engine, double dT)
{
	return false;
}


bool  MainMenu::Draw(EngineCore* Engine, double dT, FrameGraph& Graph)
{
	return false;
}