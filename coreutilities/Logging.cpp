#include "Logging.h"

#define LOGURU_IMPLEMENTATION 1
#include "..\ThirdParty\loguru\loguru.hpp"
#include <string>

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
		auto buff = loguru::vtextprintf(format, vlist);
		loguru::log_to_everywhere(1, verbosity, file, line, "", buff.c_str());
		va_end(vlist);
	}


	/************************************************************************************************/


	void LogEventAndAbort(const char * file, unsigned line, const char * test, const char * format, ...)
	{
		va_list vlist;
		va_start(vlist, format);
		auto buff				= loguru::vtextprintf(format, vlist);
		std::string message		= "CHECK \"";
		message					+= test;
		message					+= "\" FAILED: ";
		message					+= buff.c_str();

		loguru::log_to_everywhere(1, loguru::Verbosity_FATAL, file, line, "", message.c_str());
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

		loguru::log_to_everywhere(1, loguru::Verbosity_FATAL, file, line, "", message.c_str());

		// Redundant
		abort();
	}


	/************************************************************************************************/


	Verbosity VerbosityCutof() {
		return loguru::current_verbosity_cutoff();
	}


}	/************************************************************************************************/
