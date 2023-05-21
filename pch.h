#pragma once

/**********************************************************************
Copyright (c) 2015 - 2019 Robert May

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



#include "buildsettings.h"
#include "containers.h"

#include <algorithm>
#include <filesystem>
#include <fmt\format.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <ranges>
#include <chrono>
#include <stdint.h>
#include <thread>
#include <utility>

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <DirectXMath/DirectXMath.h>
#include <dxgi1_6.h>
#include <concepts>
#include <expected>
#include <tuple>
#include <variant>
#include <optional>
#include <directx-dxc/dxcapi.h>

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <DirectXMath/DirectXMath.h>

#include <Windows.h>
#include "MathUtils.h"

#include <PxPhysicsAPI.h>
#include <characterkinematic/PxController.h>
#include <extensions/PxDefaultAllocator.h>
#include <pvd/PxPvd.h>
#include <pvd/PxPvdTransport.h>
#include <characterkinematic/PxControllerManager.h>
#include <PxQueryReport.h>
