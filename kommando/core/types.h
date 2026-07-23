#pragma once
#include <cstdint>

namespace kommando
{
	struct Color
	{
		uint8_t R = 0;
		uint8_t G = 0;
		uint8_t B = 0;
	};

	constexpr auto hex(uint32_t h) -> Color
	{
		return {
			.R = static_cast<uint8_t>((h >> 16) & 0xFF),
			.G = static_cast<uint8_t>((h >> 8) & 0xFF),
			.B = static_cast<uint8_t>(h & 0xFF),
		};
	}

	constexpr auto gray(uint8_t v) -> Color
	{
		return {.R = v, .G = v, .B = v};
	}

	enum class ColorDepth : uint8_t
	{
		None = 0,
		Ansi8 = 8,
		Ansi256 = 16,
		Truecolor = 24,
	};

	constexpr auto colorDepthValue(ColorDepth d) -> int
	{
		switch (d)
		{
		case ColorDepth::None:
			return 0;
		case ColorDepth::Ansi8:
			return 8;
		case ColorDepth::Ansi256:
			return 256;
		case ColorDepth::Truecolor:
			return 16777216;
		}
		return 0;
	}

	constexpr auto supportsColor(ColorDepth terminal, ColorDepth required)
		-> bool
	{
		return colorDepthValue(terminal) >= colorDepthValue(required);
	}

	struct TerminalSize
	{
		int Rows = 0;
		int Cols = 0;
	};

}