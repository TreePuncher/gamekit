#ifndef PACKETS_H_INCLUDED
#define PACKETS_H_INCLUDED

#include "MultiplayerState.h"
#include "MultiplayerGameState.h"
#include "type.h"


/************************************************************************************************/


class LobbyMessagePacket
{
public:
    LobbyMessagePacket(MultiplayerPlayerID_t IN_id, std::string msg) :
        Header  {   { sizeof(LobbyMessagePacket) },
                    { LobbyMessage} },
        playerID{ IN_id }
    {
        strncpy(message, msg.c_str(), 64);
    }

    UserPacketHeader* GetRawPacket()
    {
        return &Header;
    }

    UserPacketHeader			Header;
    const MultiplayerPlayerID_t	playerID;
    char                        message[64];
};


/************************************************************************************************/


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


/************************************************************************************************/


class ClientReady
{
public:
	ClientReady(MultiplayerPlayerID_t	IN_playerID, bool IN_ready) :
		header      {	{sizeof(ClientReady)},
					    {UserPacketIDs::ClientReadyEvent} },
		playerID    { IN_playerID},
		ready	    { IN_ready}
	{}

	UserPacketHeader* GetRawPacket()
	{
		return &header;
	}

	UserPacketHeader		header;
	MultiplayerPlayerID_t	playerID;
	bool					ready;
};


/************************************************************************************************/


class ClientDataPacket
{
public:
    ClientDataPacket() = default;

	ClientDataPacket(MultiplayerPlayerID_t IN_id, const char* PlayerName) :
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
	const MultiplayerPlayerID_t	playerID = 0;
	char						playerName[32];
	uint16_t					playerNameLength;
};


/************************************************************************************************/


class PlayerJoinEventPacket
{
public:
    PlayerJoinEventPacket(MultiplayerPlayerID_t IN_id) :
        Header{ { sizeof(PlayerJoinEventPacket) },
                { PlayerJoin } },
        playerID{ IN_id }{}

    UserPacketHeader* GetRawPacket() { return &Header; }

    UserPacketHeader			Header;
    char                        PlayerName[64];
    const MultiplayerPlayerID_t	playerID = 0;
}; 


/************************************************************************************************/


class RequestPlayerListPacket
{
public:
	RequestPlayerListPacket(MultiplayerPlayerID_t IN_id) :
		Header      {	{ sizeof(RequestPlayerListPacket) },
				        { RequestPlayerList } },
		playerID    { IN_id }{}

	UserPacketHeader* GetRawPacket()
	{
		return &Header;
	}

    static PacketID_t PacketID() { return RequestPlayerList; }

	UserPacketHeader			Header;
	const MultiplayerPlayerID_t	playerID;
};


/************************************************************************************************/


class PlayerListPacket : public UserPacketHeader
{
public:
	PlayerListPacket(size_t playerCount) :
        UserPacketHeader{
            { GetPacketSize(playerCount)},
			{ RequestPlayerListResponse } },
		playerCount	{ playerCount	}{}

	uint64_t playerCount;
    
	struct entry
	{
		MultiplayerPlayerID_t	ID;
		char					name[32];
		bool					ready;
	}Players[];


    static PlayerListPacket* Create(size_t playerCount, iAllocator* allocator)
    {
        void* _ptr = allocator->malloc(GetPacketSize(playerCount));

        return new(_ptr) PlayerListPacket{ playerCount };
    }


	static size_t GetPacketSize(size_t playerCount)
	{
		return 
			sizeof(entry) * playerCount + 
			sizeof(UserPacketHeader) + 
			sizeof(uint32_t);
	}
};


/************************************************************************************************/


class StartGamePacket : public UserPacketHeader
{
public:
	StartGamePacket() :
        UserPacketHeader
        {   sizeof(StartGamePacket),
		    { BeginGame } } {}
};


/************************************************************************************************/


class ClientGameLoadedPacket : public UserPacketHeader
{
    ClientGameLoadedPacket() :
        UserPacketHeader
        {   sizeof(ClientGameLoadedPacket),
            { ClientGameLoaded } } {}
};


/************************************************************************************************/

#endif
