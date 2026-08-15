/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "Clock.h"

#include <chrono>

std::uint64_t GetTickMs()
{
	using clock = std::chrono::steady_clock;
	static const clock::time_point start = clock::now();
	return static_cast<std::uint64_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count());
}
