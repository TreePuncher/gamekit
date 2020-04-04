/**********************************************************************

Copyright (c) 2019 Robert May

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


#ifdef _WIN32
#pragma once
#endif

#ifndef LOGGING_H_INCLUDED
#define LOGGING_H_INCLUDED


#ifndef FK_LOG_LEVEL

#if _DEBUG
#define FK_LOG_LEVEL 0
#else
#define FK_LOG_LEVEL 9
#endif

#endif


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

		// Do not use higher verbosity levels, as that will make grepping log files harder.
		Verbosity_MAX     = +9,
	};


	typedef void (*FNLOG_Callback)(void* User, const char* Str, size_t StrLen);

	struct LogCallback
	{
		char*			ID;
		void*			User;
		FNLOG_Callback Callback;
	};

	void InitLog(int argc, char* argv[]);

	void AddLogCallback(LogCallback* LogCallback, int Verbosity);
	void ClearLogCallbacks();

	void SetShellVerbocity(Verbosity verbosity);
	void AddLogFile(char* file, Verbosity verbosity, bool Append = true);
	Verbosity VerbosityCutof();

	void LogEvent(Verbosity verbosity, const char * file, unsigned line, const char *format, ...);
	void LogEventAndAbort(const char * file, unsigned line, const char *test, const char *format, ...);
	void LogEventAndAbort(const char * file, unsigned line, const char *test);
}


#define FK_VLOG(verbosity, ...) ((verbosity) > FlexKit::VerbosityCutof())					\
	? (void)0																				\
	: FlexKit::LogEvent(verbosity, __FILE__, __LINE__, __VA_ARGS__)


// Assert
#if _DEBUG

#define FK_CHECK(test, ...) ((bool)(test))												\
	? (void)0																				\
	: FlexKit::LogEventAndAbort(__FILE__, __LINE__, "" #test)

#define FK_CHECK_WITH(test, ...) ((bool)(test))												\
	? (void)0																				\
	: FlexKit::LogEventAndAbort(__FILE__, __LINE__, "" #test, __VA_ARGS__)

#else

#define FK_CHECK(test, ...)
#define FK_CHECK_WITH(test, ...)

#endif


#if 0
#define FK_LOG(verbosity_name, ...) FK_VLOG(FlexKit::Verbosity_ ## verbosity_name, __VA_ARGS__)
#endif


// Error levels
#define FK_LOG_FATAL(...)		FK_VLOG(FlexKit::Verbosity_FATAL, __VA_ARGS__)
#define FK_LOG_ERROR(...)		FK_VLOG(FlexKit::Verbosity_ERROR, __VA_ARGS__)
#define FK_LOG_WARNING(...)		FK_VLOG(FlexKit::Verbosity_WARNING, __VA_ARGS__)


// Level 0 (Info)
#if FK_LOG_LEVEL>=0
#define FK_LOG_INFO(...)		FK_VLOG(FlexKit::Verbosity_INFO, __VA_ARGS__)
#define FK_LOG_0(...)			FK_VLOG(FlexKit::Verbosity_0, __VA_ARGS__)
#else
#define FK_LOG_INFO(...)
#define FK_LOG_0(...)
#endif


// Level 1
#if FK_LOG_LEVEL>=1
#define FK_LOG_1(...)			FK_VLOG(FlexKit::Verbosity_1, __VA_ARGS__)
#else
#define FK_LOG_1(...)
#endif


// Level 2
#if FK_LOG_LEVEL>=2
#define FK_LOG_2(...)			FK_VLOG(FlexKit::Verbosity_2, __VA_ARGS__)
#else
#define FK_LOG_2(...)
#endif


// Level 3
#if FK_LOG_LEVEL>=3
#define FK_LOG_3(...)			FK_VLOG(FlexKit::Verbosity_3, __VA_ARGS__)
#else
#define FK_LOG_3(...)
#endif


// Level 4
#if FK_LOG_LEVEL>=4
#define FK_LOG_4(...)			FK_VLOG(FlexKit::Verbosity_4, __VA_ARGS__)
#else
#define FK_LOG_4(...)
#endif


// Level 5
#if FK_LOG_LEVEL>=5
#define FK_LOG_5(...)			FK_VLOG(FlexKit::Verbosity_5, __VA_ARGS__)
#else
#define FK_LOG_5(...)
#endif


// Level 6
#if FK_LOG_LEVEL>=6
#define FK_LOG_6(...)			FK_VLOG(FlexKit::Verbosity_6, __VA_ARGS__)
#else
#define FK_LOG_6(...)
#endif


// Level 7
#if FK_LOG_LEVEL>=7
#define FK_LOG_7(...)			FK_VLOG(FlexKit::Verbosity_7, __VA_ARGS__)
#else
#define FK_LOG_7(...)
#endif


// Level 8
#if FK_LOG_LEVEL>=8
#define FK_LOG_8(...)			FK_VLOG(FlexKit::Verbosity_8, __VA_ARGS__)
#else
#define FK_LOG_8(...)
#endif


// Level 9
#if FK_LOG_LEVEL>=9
#define FK_LOG_9(...)			FK_VLOG(FlexKit::Verbosity_9, __VA_ARGS__)
#else
#define FK_LOG_9(...)
#endif


#endif // LOGGING_H_INCLUDED
