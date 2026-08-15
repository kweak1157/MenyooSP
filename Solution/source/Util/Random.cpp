/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#include "Random.h"

#include <random>

int RandomInt(int exclusiveMax)
{
	if (exclusiveMax <= 0)
		return 0;

	static thread_local std::mt19937 engine{ std::random_device{}() };
	std::uniform_int_distribution<int> dist(0, exclusiveMax - 1);
	return dist(engine);
}
