#pragma once

#include "type.h"
#include <stdint.h>

namespace FlexKit
{
	// Transforms Triggers

	constexpr uint32_t ParentChangeSignalID		= GetCRCGUID(ParentChangeSignalID);
	constexpr uint32_t TranslationSignalID		= GetCRCGUID(TranslationSignalID);
	constexpr uint32_t SetOrientationmSignalID	= GetCRCGUID(SetOrientationmSignalID);
	constexpr uint32_t SetScaleSignalID			= GetCRCGUID(SetScaleSignalID);
	constexpr uint32_t SetTransformSignalID		= GetCRCGUID(SetTransformSignalID);

	// Light Slot IDs
	constexpr uint32_t LightSetRaidusSlotID		= GetTypeGUID(LightSetRaidusSlotID);


	// Gamplay Triggers
	constexpr uint32_t ActivateTrigger			= GetTypeGUID(ActivateTrigger);

	// Gamplay Slots
	constexpr uint32_t PortalSlot				= GetTypeGUID(PortalSlot);


	// Collider Slots
	constexpr uint32_t	ChangePositionStaticBodySlot	= GetTypeGUID(ChangePositionStaticBodySlot);
	constexpr uint32_t	ChangeOrientationStaticBodySlot	= GetTypeGUID(ChangeOrientationStaticBodySlot);
}
