#include <iostream>
#include "Signal.h"
#include <fmt\format.h>

int main()
{
    {
        FlexKit::Signal<void (int)>::Slots R;

        {
            FlexKit::Signal<void (int)> signal;

            signal.Connect(R,
                [](int x)
                {
                    fmt::print("Hello World 1 : {}\n", x);
                });

            {
                FlexKit::Signal<void(int)>::Slots S;

                signal.Connect(S,
                    [](int x)
                    {
                        fmt::print("Hello World 2 : {}\n", x);
                    });

                signal(1);
            }

            signal(2);
        }

        int x = 0;
    }

    return 0;
}
