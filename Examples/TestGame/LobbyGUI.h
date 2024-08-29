#pragma once

#include "BaseState.h"
#include "EngineCore.h"
#include "Gameplay.h"
#include "MultiplayerState.h"
#include "..\source\GameFramework.h"
#include "GuiUtilities.h"
#include "Fonts.h"


/************************************************************************************************/


class LobbyState : public FlexKit::FrameworkState
{
public:
	LobbyState(FlexKit::GameFramework& framework, BaseState& IN_base, NetworkState& IN_host);
	~LobbyState();

	UpdateTask* Update  (						FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) override;
	UpdateTask* Draw    (FlexKit::UpdateTask*,	FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph&)  override;

	bool		DrawChatRoom		(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT);
	void		DrawEquipmentEditor	(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT);


	void PostDrawUpdate (EngineCore&, double dT) override;

	bool EventHandler   (Event evt) override;

	void MessageRecieved(std::string& msg);

	struct Player
	{
		std::string             Name;
		MultiplayerPlayerID_t   ID;
	};


	bool host       = false;
	int  selection2         = 0;
	int  selectionEquipped  = 0;


	std::function<void   (std::string)>	OnSendMessage;
	std::function<Player (uint idx)>	GetPlayer       = [](uint idx){ return Player{}; };
	std::function<uint ()>				GetPlayerCount  = []{ return 0;};

	std::function<void ()>				OnGameStart     = [] {};
	std::function<void ()>				OnReady         = [] {};

	std::function<void()>				OnApplyEquipmentChanges    = [] {};

	struct EquipmentType
	{
		std::string gadgetName  = "!!!";
		std::string Description = "!!!";

		size_t  gadgetCount = 0;
		std::function<GadgetInterface* (iAllocator& allocator)> CreateItem =
			[](auto& allocator) -> GadgetInterface* 
			{
				return nullptr;
			};
	};

	std::vector<EquipmentType> available =
	{
		{
			"TestItem",
			"This is a test item, it does nothing!",
			1,
			[](iAllocator& allocator) -> GadgetInterface*
			{
				return &allocator.allocate<FlashLight>();
			}
		}
	};


	std::vector<GadgetInterface*>   equipped;

	std::vector<Player> players;
	std::string         chatHistory;

	BaseState&          base;
	NetworkState&       net;

	char inputBuffer[512];
};
 

/**********************************************************************

Copyright (c) 2022 Robert May

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
