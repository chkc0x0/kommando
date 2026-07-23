#include "core/terminal.h"
#include "core/types.h"
#include <cstdio>

auto main(int argc, const char** argv) -> int
{
	kommando::Terminal term;

	auto size = term.Size();
	auto depth = term.DetectColorDepth();

	std::printf(
		"%s %dx%d %hhu\n",
		term.IsTty() ? "tty" : "no tty",
		size.Rows,
		size.Cols,
		static_cast<uint8_t>(depth)
	);

	term.Write("hello");

	return 0;
}