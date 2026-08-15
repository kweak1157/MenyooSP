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

#ifdef max
#pragma push_macro("max")
#undef max
#define MENYOO_RESTORE_MAX
#endif
#ifdef min
#pragma push_macro("min")
#undef min
#define MENYOO_RESTORE_MIN
#endif
#include <simpleini\SimpleIni.h>
#ifdef MENYOO_RESTORE_MIN
#pragma pop_macro("min")
#undef MENYOO_RESTORE_MIN
#endif
#ifdef MENYOO_RESTORE_MAX
#pragma pop_macro("max")
#undef MENYOO_RESTORE_MAX
#endif

namespace MenuConfig
{
	extern CSimpleIniA iniFile;
	extern bool bSaveAtIntervals;

	void ConfigInit();
	void ConfigRead();
	void ConfigSave();
	void ConfigResetHaxValues();
}



