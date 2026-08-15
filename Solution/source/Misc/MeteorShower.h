/*
* Menyoo PC - Grand Theft Auto V single-player trainer mod
* Copyright (C) 2019  MAFINS
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/
#pragma once

#include "GenericLoopedMode.h"

#include "..\Scripting\Game.h" //GameSound

#include <cstdint>
#include <vector>

typedef unsigned long DWORD;

namespace GTAmodel {
	class Model;
}
class GTAprop;

namespace _MeteorShower_
{
	class MeteorShower final : public GenericLoopedMode
	{
	private:
		std::uint64_t timer1;
		std::uint64_t timer2;
		std::vector<GTAprop> rockArray;
		std::vector<GTAmodel::Model> rockModels;
		static const std::vector<std::string> rockModelNames;
		static Game::Sound::GameSound crateLandSound;
	public:
		MeteorShower();

		void TurnOn() override;
		void TurnOff() override;

		void Tick() override;
		inline void DoMeteorShowerTick();

	};
	
	
	extern MeteorShower g_meteorShower;

	void ToggleOnOff();
}


