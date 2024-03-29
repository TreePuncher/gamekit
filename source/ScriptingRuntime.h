#pragma once

#include <filesystem>

class asIScriptContext;
class asIScriptEngine;
class asIScriptFunction;
class asIScriptObject;
class asIScriptModule;

namespace FlexKit
{
	class iAllocator;

	struct ScriptContext
	{
		asIScriptContext*   ctx;
		void*               scriptState;
		asIScriptModule*    scriptModule;
	};

	struct ScriptResourceBlob : public Resource
	{
		ScriptResourceBlob(size_t byteCodeSize);

		size_t blobSize;
	};

	enum RegisterFlags
	{
		REGISTER_ALL,
		EXCLUDE_LOADANIMATION,
	};

	void RegisterRuntimeAPI	(asIScriptEngine*, RegisterFlags flags = REGISTER_ALL);
	void RegisterMathTypes	(asIScriptEngine*, iAllocator* allocator);

	void InitiateScriptRuntime();
	void ReleaseScriptRuntime();

	void AddGlobal		(const char* str, void*);
	void ReleaseGlobal	(const char* str);

	[[nodiscard]] asIScriptModule* LoadScriptFile		(const char* moduleName, std::filesystem::path, iAllocator&);
	[[nodiscard]] asIScriptModule* LoadByteCode			(const char* moduleName, const char* byteCode, size_t);
	[[nodiscard]] asIScriptModule* LoadByteCodeAsset	(uint64_t assetID);

	asIScriptEngine*	GetScriptEngine();
	asIScriptContext*	GetContext();
	void				ReleaseContext(asIScriptContext*);
}


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
