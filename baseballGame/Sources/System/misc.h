#pragma once

#include <windows.h>
#include <crtdbg.h>
#include <cstdlib>

#if defined( DEBUG ) || defined( _DEBUG )
#define _ASSERT_EXPR_A(expr, msg) \
	(void)((!!(expr)) || \
	(1 != _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, "%s", msg)) || \
	(_CrtDbgBreak(), 0))
#else
#define  _ASSERT_EXPR_A(expr, expr_str) ((void)0)
#endif

// Keep assertion-based pointer/resource validation active in Release builds.
#if !defined(DEBUG) && !defined(_DEBUG)
inline void baseball_release_check_failed(const char* file, int line)
{
	char message[512]{};
	sprintf_s(message, "A runtime validation failed.\n%s (%d)", file, line);
	MessageBoxA(nullptr, message, "BASEBALL runtime error", MB_OK | MB_ICONERROR);
	std::abort();
}

#undef _ASSERT_EXPR
#define _ASSERT_EXPR(expression, message) \
	do { if (!(expression)) baseball_release_check_failed(__FILE__, __LINE__); } while (false)

#undef _ASSERT_EXPR_A
#define _ASSERT_EXPR_A(expression, message) \
	do { if (!(expression)) baseball_release_check_failed(__FILE__, __LINE__); } while (false)
#endif

inline LPWSTR hr_trace(HRESULT hr)
{
	LPWSTR msg{ 0 };
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&msg), 0, NULL);
	return msg;
}

class benchmark
{
	LARGE_INTEGER ticks_per_second;
	LARGE_INTEGER start_ticks;
	LARGE_INTEGER current_ticks;

public:
	benchmark()
	{
		QueryPerformanceFrequency(&ticks_per_second);
		QueryPerformanceCounter(&start_ticks);
		QueryPerformanceCounter(&current_ticks);
	}
	~benchmark() = default;
	benchmark(const benchmark&) = delete;
	benchmark& operator=(const benchmark&) = delete;
	benchmark(benchmark&&) noexcept = delete;
	benchmark& operator=(benchmark&&) noexcept = delete;

	void begin()
	{
		QueryPerformanceCounter(&start_ticks);
	}
	float end()
	{
		QueryPerformanceCounter(&current_ticks);
		return static_cast<float>(current_ticks.QuadPart - start_ticks.QuadPart) / static_cast<float>(ticks_per_second.QuadPart);
	}
};

