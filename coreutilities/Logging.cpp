#include "Logging.h"

#define LOGURU_IMPLEMENTATION 1
#include "..\ThirdParty\loguru\loguru.hpp"

namespace FlexKit
{


	/************************************************************************************************/


	void InitLog(int argc, char * argv[])
	{
		loguru::init(argc, argv);
	}


	/************************************************************************************************/


	void SetShellVerbocity(Verbosity verbosity)
	{
		loguru::g_stderr_verbosity = verbosity;
	}


	/************************************************************************************************/


	void AddLogFile(char * file, Verbosity verbosity)
	{
		loguru::add_file(file, loguru::Append, verbosity);
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


	Verbosity VerbosityCutof() {
		return loguru::current_verbosity_cutoff();
	}

}