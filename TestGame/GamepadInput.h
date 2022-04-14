#pragma once
#include "buildsettings.h"
#include "MathUtils.h"

namespace FlexKit
{
    class GamepadInput
    {
    public:
        GamepadInput(HWND hwnd);

        void Update();

        void    SetAxisMultiplier   (uint32_t axisID, float f);
        float   GetAxis             (uint32_t axisID);
        bool    GetButton           (uint32_t btnID);

        bool    GetButtonA();
        bool    GetButtonB();
        bool    GetButtonX();
        bool    GetButtonY();

        bool    Up();
        bool    Down();
        bool    Left();
        bool    Right();

        bool    UpRight();
        bool    UpLeft();
        bool    DownLeft();
        bool    DownRight();

        bool    GetLeftShoulder();
        bool    GetRightShoulder();

        float   GetLeftTrigger();
        float   GetRightTrigger();

        float2  GetLeftJoyStick();
        float2  GetRightJoyStick();
    };
}
