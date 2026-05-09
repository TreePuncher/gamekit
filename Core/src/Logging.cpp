#define __STDC_WANT_LIB_EXT1__ 1

#include "BuildSettings.hpp"
#include "Logging.hpp"
#include <cstring>
#include <loguru/loguru.hpp>
#include <stdarg.h>
#include <string>

#ifdef WIN32
#include <stacktrace>
#endif

namespace FlexKit
{	/************************************************************************************************/


	void InitLog(int argc, char * argv[])
	{
		loguru::init(argc, argv);
	}


	/************************************************************************************************/

	
	void AddLogCallback(LogCallback* CB_Data, int Verbosity)
	{
		auto CallbackWrapper = [](void* user_data, const loguru::Message& message)
		{
			LogCallback* Data = reinterpret_cast<LogCallback*>(user_data);

			Data->Callback(Data->User, message.message, strnlen_s(message.message, 1024));
		};

		loguru::add_callback(CB_Data->ID, CallbackWrapper, CB_Data, Verbosity);
	}


	/************************************************************************************************/


	void ClearLogCallbacks()
	{
		loguru::remove_all_callbacks();
	}


	/************************************************************************************************/


	void SetShellVerbocity(Verbosity verbosity)
	{
		loguru::g_stderr_verbosity = verbosity;
	}


	/************************************************************************************************/


	void AddLogFile(const char * file, Verbosity verbosity, bool Append )
	{
		loguru::add_file(file, Append ? loguru::Append : loguru::Truncate, verbosity);
	}


	/************************************************************************************************/


	void LogEvent(Verbosity verbosity, const char * file, unsigned line, const char *format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff = loguru::textprintf(format, vlist);
		loguru::log(verbosity, file, line, "%s", buff.c_str());
		va_end(vlist);
	}


	/************************************************************************************************/


	void LogEventAndAbort(const char * file, unsigned line, const char * test, const char * format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff				= loguru::textprintf(format, vlist);
		std::string message		= "CHECK \"";
		message					+= test;
		message					+= "\" FAILED: ";
		message					+= buff.c_str();

		loguru::log(loguru::Verbosity_FATAL, file, line, "%s", message.c_str());
		va_end(vlist);

		// Redundant
		abort();
	}


	/************************************************************************************************/


	void LogEventAndAbort(const char * file, unsigned line, const char * test)
	{
		std::string message		= "CHECK \"";
		message					+= test;
		message					+= "\" FAILED";

		loguru::log(loguru::Verbosity_FATAL, file, line, "%s", message.c_str());

		// Redundant
		abort();
	}


	/************************************************************************************************/


	Verbosity VerbosityCutof() {
		return loguru::current_verbosity_cutoff();
	}


	/************************************************************************************************/


	std::string GetCallStackString()
	{
		std::string traceMessage;
#ifdef WIN32
		auto stackTrace = std::stacktrace::current();

		for (const auto& frame : std::ranges::subrange(stackTrace.begin() + 1, stackTrace.end()))
		{
			auto description = frame.description();

			const size_t strackDescriptionMaxLength = 128;
			if (description.size() <= strackDescriptionMaxLength)
			{
				traceMessage += " \t" + description + "\n";
			}
			else if (description.size() >= strackDescriptionMaxLength)
			{
				traceMessage += " \t" + description.substr(0, strackDescriptionMaxLength / 2) + " ... " + description.substr(description.length() - strackDescriptionMaxLength / 2, strackDescriptionMaxLength / 2) + "\n";
			}
			else if (description.size() <= strackDescriptionMaxLength)
				traceMessage += " \t..." + description.substr(description.size() - strackDescriptionMaxLength, strackDescriptionMaxLength) + "\n ";
		}
#else
		traceMessage += "Trace not available!";
#endif

		return traceMessage;
	}

}	/************************************************************************************************/


/**********************************************************************
Copyright (c) 2015 - 2024 Robert May

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
