#pragma once

#include <cmath>
#include <limits>
#include <type_traits>
#include <simpleini/SimpleIni.h>

namespace ConfigValue
{
	template <typename T, typename U>
	T CheckedNumber(U value, T fallback)
	{
		static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>);
		const long double number = static_cast<long double>(value);
		if (!std::isfinite(number) || number < (std::numeric_limits<T>::lowest)() ||
			number > (std::numeric_limits<T>::max)())
			return fallback;
		return static_cast<T>(value);
	}

	template <typename T>
	T ReadInteger(const CSimpleIniA& ini, const char* section, const char* key, T fallback)
	{
		static_assert(std::is_integral_v<T>);
		return CheckedNumber(ini.GetLongValue(section, key, fallback), fallback);
	}

	template <typename T>
	T ReadReal(const CSimpleIniA& ini, const char* section, const char* key, T fallback)
	{
		return CheckedNumber(ini.GetDoubleValue(section, key, fallback), fallback);
	}
}
