/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "StringManip.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

// Integer/String manipulation
std::string int_to_hexstring(unsigned long i, bool zerox)
{
	std::stringstream stream;
	if (zerox)
		stream << "0x" << std::setfill('0') << std::setw(sizeof(i) * 2);
	stream << std::hex << i;
	return stream.str();
}

namespace
{
	std::string_view TrimAscii(std::string_view text)
	{
		while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
			text.remove_prefix(1);
		while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
			text.remove_suffix(1);
		return text;
	}

	template <typename T>
	std::optional<T> TryParseNumber(std::string_view text)
	{
		text = TrimAscii(text);
		if (text.empty())
			return std::nullopt;
		if (text.front() == '+')
			text.remove_prefix(1);
		if (text.empty())
			return std::nullopt;

		T value{};
		const char* const first = text.data();
		const char* const last = first + text.size();
		const auto [ptr, ec] = std::from_chars(first, last, value);
		if (ec != std::errc{} || ptr != last)
			return std::nullopt;
		return value;
	}
}

std::optional<int> TryParseInt(std::string_view text)
{
	return TryParseNumber<int>(text);
}

std::optional<float> TryParseFloat(std::string_view text)
{
	return TryParseNumber<float>(text);
}

namespace boost
{
	// +ve numbers + ASCII (no UTF-8)
	void to_upper(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	}
	std::string to_upper_copy(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		return str;
	}

	void to_lower(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	}
	std::string to_lower_copy(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}
}



