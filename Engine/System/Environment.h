#pragma once

enum wellKnownPath_t
{
	WKP_CONFIGURATION_STORAGE	= 0,
	WKP_SAVEGAME_STORAGE		= 1,

	WKP_COUNT,
};

const unsigned short		Env_GetCacheLineSizeL2();
const int					Env_GetCPUCoreCount();
const unsigned long long	Env_GetRAMTotalSize();
const char*					Env_GetCPUName();
const char*					Env_GetOSName();
