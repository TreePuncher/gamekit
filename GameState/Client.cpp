/**********************************************************************

Copyright (c) 2018 Robert May

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

#include "Client.h"


/************************************************************************************************/


ClientLobbyState::ClientLobbyState(
	FlexKit::GameFramework* IN_framework,
	GameClientState*		IN_client,
	NetworkState*			IN_network, 
	char*					IN_localPlayerName) :
		FrameworkState	{IN_framework},
		network			{IN_network},
		packetHandlers	{IN_framework->Core->GetBlockMemory()},
		localPlayerName {IN_localPlayerName},
		client			{IN_client},
		Ready			{false}
{
	packetHandlers.push_back(
		CreatePacketHandler(
			LobbyMessage, 
			[&](auto, auto, auto)
			{
				std::cout << "Message Received!\n";
			}, 
			IN_framework->Core->GetBlockMemory()));


	packetHandlers.push_back(
		CreatePacketHandler(
			ClientDataRequest,
			[&](UserPacketHeader* incomingPacket, RakNet::Packet* P, NetworkState* network)
			{
				auto				request = reinterpret_cast<RequestClientDataPacket*>(incomingPacket);
				ClientDataPacket	responsePacket(request->playerID, localPlayerName);

				client->localID = request->playerID;

				FK_LOG_INFO("Sending Client Info");
				std::cout << "playerID set to: " << request->playerID << "\n";
				network->SendPacket(responsePacket.GetRawPacket(), P->systemAddress);
			},
			IN_framework->Core->GetBlockMemory()));

	network->PushHandler(&packetHandlers);
}


/************************************************************************************************/


bool ClientLobbyState::EventHandler(FlexKit::Event evt)
{
	std::cout << "event recieved!\n";

	if (evt.InputSource == FlexKit::Event::InputType::Keyboard)
	{
		if (evt.Action == FlexKit::Event::InputAction::Release)
		{
			switch (evt.mData1.mKC[0])
			{
			case FlexKit::KC_R:
			{
				Ready = !Ready;
				ClientReady packet(client->localID, Ready);
				network->SendPacket(packet.GetRawPacket(), client->ServerAddress);
			}
			}
		}
	}
	return true;
}