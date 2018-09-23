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


#ifndef PACKETS_H_INCLUDED
#define PACKETS_H_INCLUDED

#include "MultiplayerState.h"
#include "..\coreutilities\type.h"


typedef uint64_t MultiplayerPlayerID_t;


enum LobbyPacketIDs : PacketID_t
{
	_LobbyPacketID_Begin		= UserPacketIDCount,
	RequestPlayerList			= GetCRCGUID(RequestPlayerList),
	RequestPlayerListResponse	= GetCRCGUID(PlayerList),
};


class RequestClientDataPacket
{
public:
	RequestClientDataPacket(MultiplayerPlayerID_t	IN_playerID) :
		header	{	{sizeof(RequestClientDataPacket)},
					{UserPacketIDs::ClientDataRequest} },
		playerID{IN_playerID}
	{}

	UserPacketHeader* GetRawPacket()
	{
		return &header;
	}

	UserPacketHeader		header;
	MultiplayerPlayerID_t	playerID;
};


class ClientReady
{
public:
	ClientReady(MultiplayerPlayerID_t	IN_playerID, bool IN_ready) :
		header{		{sizeof(ClientReady)},
					{UserPacketIDs::ClientReadyEvent} },
		playerID{	IN_playerID},
		ready	{	IN_ready}
	{}

	UserPacketHeader* GetRawPacket()
	{
		return &header;
	}

	UserPacketHeader		header;
	MultiplayerPlayerID_t	playerID;
	bool					ready;
};

class ClientDataPacket
{
public:
	ClientDataPacket(MultiplayerPlayerID_t IN_id, char* PlayerName) :
		Header			{	{sizeof(ClientDataPacket)},
							{UserPacketIDs::ClientDataRequestResponse} },
		playerNameLength{ uint16_t(strnlen(playerName, 128)) },
		playerID		{ IN_id }
	{
		strncpy(playerName, PlayerName, 32);
	}

	UserPacketHeader* GetRawPacket()
	{
		return &Header;
	}

	UserPacketHeader			Header;
	const MultiplayerPlayerID_t	playerID;
	char						playerName[32];
	uint16_t					playerNameLength;
};

#endif