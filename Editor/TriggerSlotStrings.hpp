#include <unordered_map>
#include <string_view>
#include <TriggerSlotIds.hpp>


/************************************************************************************************/


inline const std::unordered_map<uint32_t, std::string_view>  TriggerIDStringMappings =
{
	{ FlexKit::ParentChangeSignalID,	"Parent Change"			},
	{ FlexKit::TranslationSignalID,		"Translation Change"	},
	{ FlexKit::SetOrientationmSignalID, "Orientation Change"	},
	{ FlexKit::SetScaleSignalID,		"Scale Change"			},
	{ FlexKit::SetTransformSignalID,	"Transform Change"		},
	{ FlexKit::ActivateTrigger,			"User Acivate"			},
};


/************************************************************************************************/


inline const std::unordered_map<uint32_t, std::string_view>  SlotIDStringMappings =
{
	{ FlexKit::LightSetRaidusSlotID,			"Set Light Radius"				},
	{ FlexKit::ChangePositionStaticBodySlot,	"Set Static Body Position"		},
	{ FlexKit::ChangeOrientationStaticBodySlot,	"Set Static Body Orientation"	},
	{ FlexKit::PortalSlot,						"Activate Portal"				},
};


/**********************************************************************

Copyright (c) 2023 Robert May

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
