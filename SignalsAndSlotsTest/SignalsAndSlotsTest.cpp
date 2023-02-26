#include <iostream>
#include "Signals.h"
#include <fmt\format.h>

int main()
{
	{
		FlexKit::Signal<void (int)>::Slot R;

		{
			FlexKit::Signal<void (int)> signal;

			signal.Connect(R,
				[](int x)
				{
					fmt::print("Hello World 1 : {}\n", x);
				});

			{
				FlexKit::Signal<void(int)>::Slot s;

				s.Bind(
					[](int x)
					{
						fmt::print("Hello World 2 : {}\n", x);
					});

				signal.Connect(s);

				signal(1);
			}

			signal(2);
		}

		int x = 0;
	}

	return 0;
}
