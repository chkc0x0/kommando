#pragma once
#include "core/types.h"
#include <string_view>

namespace kommando
{
	class Terminal
	{
	public:
		Terminal();
		~Terminal();

		Terminal(const Terminal&) = delete;
		auto operator=(const Terminal&) -> Terminal& = delete;
		Terminal(Terminal&&) noexcept;
		auto operator=(Terminal&&) noexcept -> Terminal&;

		[[nodiscard]] auto IsTty() const -> bool;

		[[nodiscard]] auto DetectColorDepth() const -> ColorDepth;

		[[nodiscard]] auto Size() const -> TerminalSize;

		void Write(std::string_view text);
		void Flush();

		void CursorMove(int row, int col);

		void CursorUp(int n = 1);
		void CursorDown(int n = 1);

		void CursorHide();
		void CursorShow();

		void Clear();
		void ClearLine();

	private:
		void* _platformData;
	};
}