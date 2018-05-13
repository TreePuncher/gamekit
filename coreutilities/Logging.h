#ifdef _WIN32
#pragma once
#endif

#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED

namespace FlexKit
{
	using Verbosity = int;

	enum NamedVerbosity: int
	{
		Verbosity_OFF     = -9, // Only for shell verbosity

		Verbosity_FATAL   = -3,
		Verbosity_ERROR   = -2,
		Verbosity_WARNING = -1,

		// Normal messages.
		Verbosity_INFO    =  0,
		Verbosity_0       =  0,

		// Verbosity levels 1-9 are generally written to file.
		Verbosity_1       = +1,
		Verbosity_2       = +2,
		Verbosity_3       = +3,
		Verbosity_4       = +4,
		Verbosity_5       = +5,
		Verbosity_6       = +6,
		Verbosity_7       = +7,
		Verbosity_8       = +8,
		Verbosity_9       = +9,

		// Don not use higher verbosity levels, as that will make grepping log files harder.
		Verbosity_MAX     = +9,
	};

	void InitLog(int argc, char* argv[]);
	void SetShellVerbocity(Verbosity verbosity);
	void AddLogFile(char* file, Verbosity verbosity);
	Verbosity VerbosityCutof();

	void LogEvent(Verbosity verbosity, const char * file, unsigned line, const char *format, ...);

}

#define FK_VLOG(verbosity, ...) ((verbosity) > FlexKit::VerbosityCutof())					\
	? (void)0																						\
	: FlexKit::LogEvent(verbosity, __FILE__, __LINE__, __VA_ARGS__)

#define FK_LOG(verbosity_name, ...) FK_VLOG(FlexKit::Verbosity_ ## verbosity_name, __VA_ARGS__)

#endif // LOGGING_H_INCLUDED