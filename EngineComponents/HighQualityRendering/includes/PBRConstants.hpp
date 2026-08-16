#pragma once
#include <FrameGraph.hpp>

namespace FlexKit
{
	struct UpdatePBRBrushConstantsResults
	{
		FrameGraphNodeHandle				node;
		FrameResourceHandle					constants;
		UploadReservation					upload;
		size_t								reservationSize = (size_t)-1;

		//Vector<uint32_t>				entityTable;
		//CBPushBuffer&					GetConstantBuffer(size_t IN_reservationSize);
		//CBPushBuffer&					GetConstantBuffer();
	};
}
