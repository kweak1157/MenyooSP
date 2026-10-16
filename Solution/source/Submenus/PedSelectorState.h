#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace PedSelectorUI
{
	inline std::string Lower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return text;
	}

	inline int FirstVisibleOption(int selected, int total, int maximum = 14, int scroll = 8, int below = 6)
	{
		if (selected < scroll || total <= maximum) return 1;
		return selected > total - below ? (std::max)(1, total - maximum + 1) : selected - scroll + 1;
	}

}
