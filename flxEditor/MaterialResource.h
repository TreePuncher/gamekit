#pragma once
#include "EditorResource.h"
#include "Serialization.hpp"
#include <string>
#include <EditorInspectorView.h>


/************************************************************************************************/


class MaterialResource : public FlexKit::Serializable<MaterialResource, FlexKit::iResource, GetTypeGUID(MaterialResource)>
{
public:
	void Serialize(auto& ar)
	{
		FlexKit::ValueStore fields;

		BindValue(fields, id);
		BindValue(fields, guid);

		ar& fields;
	}

	FlexKit::ResourceBlob	CreateBlob() const override;
	const std::string&		GetResourceID() const noexcept override;

	uint64_t		GetResourceGUID()	const noexcept override;
	ResourceID_t	GetResourceTypeID() const noexcept;

	void			SetResourceID	(const std::string& IN_id)	noexcept;
	void			SetResourceGUID	(uint64_t IN_guid) noexcept;

	std::string			id		= fmt::format("Material_{}", rand());
	FlexKit::GUID_t		guid	= 0xfffffffffffffff;
};


void RegisterMaterialInspector();


/************************************************************************************************/


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
