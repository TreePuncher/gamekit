#include "LobbyGUI.h"


/************************************************************************************************/


LobbyState::LobbyState(GameFramework& framework, BaseState& IN_base, NetworkState& IN_host) :
    FrameworkState{ framework },
    base{ IN_base },
    net{ IN_host }
{
    memset(inputBuffer, '\0', 512);
    chatHistory += "Lobby Created\n";
}

LobbyState::~LobbyState()
{
    for (auto card : equipped)
        base.framework.core.GetBlockMemory().release_allocation(*card);
}



/************************************************************************************************/


UpdateTask* LobbyState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
    base.Update(core, dispatcher, dT);
    base.debugUI.Update(base.renderWindow, core, dispatcher, dT);

    net.Update(core, dispatcher, dT);

    ImGui::NewFrame();

    if (DrawChatRoom(core, dispatcher, dT))
        return nullptr;

    DrawEquipmentEditor(core, dispatcher, dT);

    return nullptr;
}

/************************************************************************************************/


bool LobbyState::DrawChatRoom(EngineCore&, UpdateDispatcher&, double dT)
{
    // Player List
    ImGui::Begin("Lobby");
    EXITSCOPE(ImGui::End());

    int selected = 0;

    std::string playerStrings[16];
    const char* playerNames[16];
    const uint playerCount = GetPlayerCount();

    for (uint I = 0; I < playerCount; I++) {
        auto player         = GetPlayer(I);
        playerStrings[I]    = player.Name;
        playerNames[I]      = playerStrings[I].c_str();
    }

    ImGui::ListBox("Players", &selected, playerNames, (int)playerCount);

    // Chat
    ImGui::InputTextMultiline("Chat Log", const_cast<char*>(chatHistory.c_str()), chatHistory.size(), ImVec2(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputText("type here...", inputBuffer, 512);

    if (ImGui::Button("Send"))
    {
        std::string msg = inputBuffer;
        chatHistory += inputBuffer;
        chatHistory += "\n";

        memset(inputBuffer, '\0', 512);

        if (OnSendMessage)
            OnSendMessage(msg);
    }

    if (host)
    {
        if (ImGui::Button("StartGame")) {
            OnGameStart();
            return true;
        }
    }
    else if (ImGui::Button("Ready"))
        OnReady();

    

    return false;
}


/************************************************************************************************/


void LobbyState::DrawEquipmentEditor(EngineCore& core, UpdateDispatcher&, double dT)
{
    ImGui::Begin("Equipment");
    
    Vector<const char*> availableNames  { core.GetTempMemory() };
    Vector<const char*> equippedNames   { core.GetTempMemory() };

    for (auto& gadget : available)
        availableNames.emplace_back(gadget.gadgetName.c_str());

    for (auto& gadget : equipped)
        equippedNames.emplace_back(gadget->cardName);

    ImGui::ListBox("Available Equipment list", &selection2, availableNames.data(), (uint)availableNames.size());

    if (available.size() > selection2)
    {
        auto str = const_cast<char*>(available[selection2].Description.c_str());

        ImGui::InputTextMultiline(
            "Gadget Description", str, strnlen(str, 64),
            { 0, 0 },
            ImGuiInputTextFlags_ReadOnly);


        if (ImGui::Button("Add"))
            equipped.push_back(available[selection2].CreateItem(core.GetBlockMemory()));
    }

    if (!equipped.size())
        ImGui::Text("Nothing Equipped");
    else
        ImGui::ListBox("Equipped list", &selectionEquipped, equippedNames.data(), (uint)equippedNames.size());

    if (equipped.size() > selectionEquipped)
    {
        auto str = const_cast<char*>(equipped[selectionEquipped]->description);

        ImGui::InputTextMultiline(
            "Gadget Description", str, strnlen(str, 64),
            { 0, 0 },
            ImGuiInputTextFlags_ReadOnly);
    }

    if (ImGui::Button("Apply"))
        OnApplyEquipmentChanges();

    ImGui::End();
}


/************************************************************************************************/


UpdateTask* LobbyState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
    auto renderTarget = base.renderWindow.GetBackBuffer();

    ClearVertexBuffer(frameGraph, base.vertexBuffer);
    ClearBackBuffer(frameGraph, renderTarget, 0.0f);

    auto reserveVB = FlexKit::CreateVertexBufferReserveObject(base.vertexBuffer, core.RenderSystem, core.GetTempMemory());
    auto reserveCB = FlexKit::CreateConstantBufferReserveObject(base.constantBuffer, core.RenderSystem, core.GetTempMemory());

    ImGui::Render();
    base.debugUI.DrawImGui(dT, dispatcher, frameGraph, reserveVB, reserveCB, renderTarget);

    FlexKit::PresentBackBuffer(frameGraph, renderTarget);

    return nullptr;
}


/************************************************************************************************/


void LobbyState::PostDrawUpdate(EngineCore& core, double dT)
{
    base.PostDrawUpdate(core, dT);
}


/************************************************************************************************/


bool LobbyState::EventHandler(Event evt)
{
    switch (evt.InputSource)
    {
    case Event::E_SystemEvent:
    {
        switch (evt.Action)
        {
        case Event::InputAction::Resized:
        {
            const auto width    = (uint32_t)evt.mData1.mINT[0];
            const auto height   = (uint32_t)evt.mData2.mINT[0];

            base.Resize({ width, height });
        }   break;

        case Event::InputAction::Exit:
            framework.quit = true;
            break;
        default:
            break;
        }
    }   break;
    };

    return base.debugUI.HandleInput(evt);
}


/************************************************************************************************/


void LobbyState::MessageRecieved(std::string& msg)
{
    chatHistory += msg + '\n';
}



/************************************************************************************************/
