#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>

enum class TaskState
{
	Running,
	Error,
	Completed
};

class EditorTask
{
public:
	TaskState state;

	void AddChildTask(std::shared_ptr<EditorTask> task)
	{
		std::scoped_lock sl{ m };
		childTasks.push_back(task);
	}

	void SetName(std::string&& newName)
	{
		std::scoped_lock sl{ m };
		name = newName;
	}

	void SetDescription(std::string&& newDescription)
	{
		std::scoped_lock sl{ m };
		name = newDescription;
	}

	std::string GetName() const
	{
		std::scoped_lock sl{ const_cast<std::mutex&>(m) };
		return name;
	}

	std::string GetDescription() const
	{
		std::scoped_lock sl{ const_cast<std::mutex&>(m) };
		return description;
	}

private:
	uint32_t	id				= rand();
	std::string name			= "Un-named task";
	std::string description		= "Describe me please!";

	std::vector<std::shared_ptr<EditorTask>> childTasks;
	std::mutex m;
};

namespace FlexKit
{
	class iWork;
}

using EditorTask_ptr = std::shared_ptr<EditorTask>;

void PostTask(FlexKit::iWork&, EditorTask_ptr);


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
