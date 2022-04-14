#include "GamepadInput.h"
#include <Windows.h>
#include <fmt/format.h>
#include "containers.h"
#include "ois/OISInputManager.h"
#include "ois/OISJoyStick.h"
#include "ois/OISEvents.h"

namespace FlexKit
{
    const char* g_DeviceType[6] = { "OISUnknown", "OISKeyboard", "OISMouse", "OISJoyStick", "OISTablet", "OISOther" };

    OIS::JoyStick*      gamePad         = nullptr;
    OIS::InputManager*  gInputMangaer   = nullptr;

    inline uint32_t leftShoulderIdx     = 4;
    inline uint32_t rightShoulderIdx    = 5;

    inline uint32_t leftTriggerIdx  = 0;
    inline uint32_t rightTriggerIdx = 0;

    inline uint32_t AIdx = 1;
    inline uint32_t BIdx = 2;
    inline uint32_t XIdx = 0;
    inline uint32_t YIdx = 3;

    inline uint32_t StartIdx    = 8;
    inline uint32_t MenuIdx     = 9;

    inline uint32_t upIdx       = 0;
    inline uint32_t downIdx     = 0;
    inline uint32_t leftIdx     = 0;
    inline uint32_t rightIdx    = 0;

    inline uint32_t left_XAxis  = 3;
    inline uint32_t left_YAxis  = 2;

    inline uint32_t right_XAxis = 5;
    inline uint32_t right_YAxis = 4;

    struct JoyStickListener : public OIS::JoyStickListener
    {
        JoyStickListener()
        {
            for (auto& axis : axis)
                axis = 0.0f;

            for (auto& factor : factors)
                factor = 1.0f;

            for (auto& btn : button)
                btn = false;
        }

        float   axis[64]        = {};
        float   factors[64]     = {};
        bool    button[64]      = {};
        bool    dPad[4];


        bool buttonPressed(const OIS::JoyStickEvent& arg, int buttonIdx) override
        {
            button[buttonIdx] = true;

            return true;
        }

        bool buttonReleased(const OIS::JoyStickEvent& arg, int buttonIdx) override
        {
            button[buttonIdx] = false;

            return true;
        }

        bool povMoved(const OIS::JoyStickEvent& arg, int index) override
        {
            dPad[0] = arg.state.mPOV->direction & OIS::Pov::North;
            dPad[1] = arg.state.mPOV->direction & OIS::Pov::East;
            dPad[2] = arg.state.mPOV->direction & OIS::Pov::South;
            dPad[3] = arg.state.mPOV->direction & OIS::Pov::West;

            return true;
        }

        bool axisMoved(const OIS::JoyStickEvent& arg, int axisIdx) override
        {
            axis[axisIdx] = float(arg.state.mAxes[axisIdx].abs) / float(OIS::JoyStick::MAX_AXIS);

            return true;
        }

    }gamePadHandler;

    GamepadInput::GamepadInput(HWND hwnd)
    {
        OIS::ParamList pl;
        pl.insert(std::make_pair(std::string("WINDOW"), fmt::format("{}", (uint32_t)hwnd)));
        gInputMangaer = OIS::InputManager::createInputSystem(pl);
        gInputMangaer->enableAddOnFactory(OIS::InputManager::AddOn_All);

        OIS::DeviceList list = gInputMangaer->listFreeDevices();
        for (auto i = list.begin(); i != list.end(); ++i)
            fmt::print("device: {}, {}", g_DeviceType[i->first], i->second);

        auto joystickCount = gInputMangaer->getNumberOfDevices(OIS::OISJoyStick);
        if (joystickCount)
        {
            gamePad = (OIS::JoyStick*)gInputMangaer->createInputObject(OIS::OISJoyStick, true);
            gamePad->setEventCallback(&gamePadHandler);
        }
    }

    void GamepadInput::Update()
    {
        if(gamePad)
            gamePad->capture();
    }

    void GamepadInput::SetAxisMultiplier(uint32_t axisID, float f)
    {
        gamePadHandler.factors[axisID] = f;
    }

    float GamepadInput::GetAxis(uint32_t axisID)
    {
        return gamePadHandler.axis[axisID] * gamePadHandler.factors[axisID];
    }

    bool GamepadInput::GetButton(uint32_t buttonID)
    {
        return gamePadHandler.button[buttonID];
    }

    bool GamepadInput::GetButtonA()
    {
        return GetButton(AIdx);
    }

    bool GamepadInput::GetButtonB()
    {
        return GetButton(BIdx);
    }

    bool GamepadInput::GetButtonX()
    {
        return GetButton(XIdx);
    }

    bool GamepadInput::GetButtonY()
    {
        return GetButton(YIdx);
    }

    bool GamepadInput::Up()
    {
        return gamePadHandler.dPad[0];
    }

    bool GamepadInput::Down()
    {
        return gamePadHandler.dPad[2];
    }

    bool GamepadInput::Left()
    {
        return gamePadHandler.dPad[3];
    }

    bool GamepadInput::Right()
    {
        return gamePadHandler.dPad[1];
    }

    bool GamepadInput::UpRight()
    {
        return Up() && Right();
    }

    bool GamepadInput::UpLeft()
    {
        return Up() && Left();
    }

    bool GamepadInput::DownLeft()
    {
        return Down() && Left();
    }

    bool GamepadInput::DownRight()
    {
        return Down() && Right();
    }

    bool GamepadInput::GetLeftShoulder()
    {
        return GetButton(leftShoulderIdx);
    }

    bool GamepadInput::GetRightShoulder()
    {
        return GetButton(rightShoulderIdx);
    }

    float GamepadInput::GetLeftTrigger()
    {
        return GetAxis(leftTriggerIdx);
    }

    float GamepadInput::GetRightTrigger()
    {
        return GetAxis(rightTriggerIdx);
    }

    float2 GamepadInput::GetRightJoyStick()
    {
        return float2{ GetAxis(left_XAxis), GetAxis(left_YAxis) };
    }

    float2 GamepadInput::GetLeftJoyStick()
    {
        return float2{ GetAxis(right_XAxis), GetAxis(right_YAxis) };
    }
}
