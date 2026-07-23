#include "core/terminal.h"
#include <cstdio>
#include <cstdlib>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace kommando
{
	struct PosixTermData
	{
		termios OriginalTermios{};
	};

	Terminal::Terminal()
	{
		if (isatty(STDOUT_FILENO) != 0)
		{
			_platformData = new PosixTermData();
			tcgetattr(STDOUT_FILENO, &((PosixTermData*)_platformData)->OriginalTermios);
		}
	}
	Terminal::~Terminal()
	{
		if (IsTty())
		{
			tcsetattr(
				STDOUT_FILENO,
				TCSANOW,
				&((PosixTermData*)_platformData)->OriginalTermios
			);
			delete (PosixTermData*)_platformData;
		}
	}
	Terminal::Terminal(Terminal&&) noexcept = default;
	auto Terminal::operator=(Terminal&&) noexcept -> Terminal& = default;

	[[nodiscard]] auto Terminal::IsTty() const -> bool
	{
		return _platformData == nullptr || isatty(STDOUT_FILENO) != 0;
	}

	[[nodiscard]] auto Terminal::Size() const -> TerminalSize
	{
		if (!IsTty())
		{
			return {.Rows = 24, .Cols = 80};
		}
		winsize ws{};
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
		{
			return {.Rows = ws.ws_row, .Cols = ws.ws_col};
		}
		return {.Rows = 24, .Cols = 80};
	}

	auto Terminal::DetectColorDepth() const -> ColorDepth
	{
		if (std::getenv("NO_COLOR") != nullptr)
		{
			return ColorDepth::None;
		}

		const char* colorterm = std::getenv("COLORTERM");
		if ((colorterm != nullptr) &&
			(std::string_view(colorterm) == "truecolor" ||
			 std::string_view(colorterm) == "24bit"))
		{
			return ColorDepth::Truecolor;
		}

		const char* term = std::getenv("TERM");
		if (term != nullptr)
		{
			std::string_view term_sv(term);
			if (term_sv.find("256color") != std::string_view::npos)
			{
				return ColorDepth::Ansi256;
			}
			if (term_sv.find("color") != std::string_view::npos)
			{
				return ColorDepth::Ansi8;
			}
			if (term_sv != "dumb")
			{
				return ColorDepth::Ansi8;
			}
		}

		return ColorDepth::None;
	}

	void Terminal::Write(std::string_view text)
	{
		if (!text.empty())
		{
			std::fwrite(text.data(), 1, text.size(), stdout);
		}
	}

	void Terminal::Flush()
	{
		std::fflush(stdout);
	}

	void Terminal::CursorMove(int row, int col)
	{
		char buf[32];
		int len =
			std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row + 1, col + 1);
		Write(std::string_view(buf, len));
	}

	void Terminal::CursorUp(int n)
	{
		char buf[16];
		int len = std::snprintf(buf, sizeof(buf), "\x1b[%dA", n);
		Write(std::string_view(buf, len));
	}

	void Terminal::CursorDown(int n)
	{
		char buf[16];
		int len = std::snprintf(buf, sizeof(buf), "\x1b[%dB", n);
		Write(std::string_view(buf, len));
	}

	void Terminal::CursorHide()
	{
		Write("\x1b[?25l");
	}
	void Terminal::CursorShow()
	{
		Write("\x1b[?25h");
	}

	void Terminal::Clear()
	{
		Write("\x1b[2J\x1b[H");
	}
	void Terminal::ClearLine()
	{
		Write("\x1b[2K");
	}
}