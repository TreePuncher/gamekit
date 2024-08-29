#pragma once
#include <unordered_map>
#include <pxr/pxr.h>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

PXR_NAMESPACE_CLOSE_SCOPE

struct PrimitiveParserInterface_* GetPrimitiveParser(const std::string&);

struct PrimitiveParserInterface_
{
	virtual ~PrimitiveParserInterface_() = default;

	virtual void ParseObject(struct USDParseContext& ctx, pxr::UsdPrim& primitive) = 0;
	virtual const char* GetTypeName()  const = 0;

	template<typename TY>
	static bool Register()
	{
		static TY t{};
		parsers.emplace(t.GetTypeName(), &t);

		return true;
	}

private:
	inline static std::unordered_map<std::string, PrimitiveParserInterface_*> parsers;

	friend PrimitiveParserInterface_* GetPrimitiveParser(const std::string&);
};


template<typename TY>
struct PrimitiveParserInterface : PrimitiveParserInterface_
{
	inline static bool register_ = Register<TY>();
};


/**********************************************************************

Copyright (c) 2019-2024 Robert May

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
