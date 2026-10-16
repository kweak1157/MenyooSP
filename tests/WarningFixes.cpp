#include "../Solution/source/Util/ConfigValue.h"
#include "../Solution/source/Util/StringManip.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

int main()
{
	CSimpleIniA ini;
	assert(ini.LoadData("[test]\nbyte=255\noverflow=256\nnegative=-1\nword=65535\nword_overflow=65536\nreal=123.75\nlarge=1e100\n") >= 0);
	assert(ConfigValue::ReadInteger(ini, "test", "byte", std::uint8_t{7}) == 255);
	assert(ConfigValue::ReadInteger(ini, "test", "overflow", std::uint8_t{7}) == 7);
	assert(ConfigValue::ReadInteger(ini, "test", "negative", std::uint8_t{7}) == 7);
	assert(ConfigValue::ReadInteger(ini, "test", "negative", std::int8_t{7}) == -1);
	assert(ConfigValue::ReadInteger(ini, "test", "word", std::uint16_t{7}) == 65535);
	assert(ConfigValue::ReadInteger(ini, "test", "word_overflow", std::uint16_t{7}) == 7);
	assert(ConfigValue::ReadInteger(ini, "test", "missing", std::uint8_t{7}) == 7);
	assert(ConfigValue::ReadReal(ini, "test", "real", 0.0f) == 123.75f);
	assert(ConfigValue::ReadReal(ini, "test", "real", 0) == 123);
	assert(ConfigValue::ReadReal(ini, "test", "large", 7.0f) == 7.0f);
	assert(ConfigValue::CheckedNumber((std::numeric_limits<double>::infinity)(), 7) == 7);
	assert(ConfigValue::CheckedNumber((std::numeric_limits<double>::quiet_NaN)(), 7) == 7);
	assert(ConfigValue::CheckedNumber(2147483647.0, 7) == 2147483647);
	assert(ConfigValue::CheckedNumber(2147483648.0, 7) == 7);
	assert(ConfigValue::CheckedNumber(-2147483648.0, 7) == (-2147483647 - 1));
	assert(ConfigValue::CheckedNumber(-2147483649.0, 7) == 7);

	const std::wstring wide = L"caf\u00e9 \u0928\u092e\u0938\u094d\u0924\u0947 \U0001f600";
	const std::string utf8 = "caf\xc3\xa9 \xe0\xa4\xa8\xe0\xa4\xae\xe0\xa4\xb8\xe0\xa5\x8d\xe0\xa4\xa4\xe0\xa5\x87 \xf0\x9f\x98\x80";
	assert(WideToUtf8(wide) == utf8);
	assert(Utf8ToWide(utf8) == wide);
	assert(WideToUtf8(L"ASCII") == "ASCII");
	assert(Utf8ToWide("ASCII") == L"ASCII");
	assert(WideToUtf8(L"").empty());
	assert(Utf8ToWide("").empty());
	const std::wstring embeddedNull(L"a\0b", 3);
	assert(WideToUtf8(embeddedNull) == std::string("a\0b", 3));
	assert(Utf8ToWide(WideToUtf8(embeddedNull)) == embeddedNull);
	std::cout << "Configuration bounds and Unicode conversion checks passed.\n";
}
