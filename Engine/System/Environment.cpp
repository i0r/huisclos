#include "Shared.h"
#include "Environment.h"

#include <Shlobj.h>
#include <intrin.h>
#include <VersionHelpers.h>

#define CPU_ID( x, y ) __cpuid( x, static_cast<int>( y ) )

const unsigned short Env_GetCacheLineSizeL2()
{
	DWORD buffLength = 0;
	GetLogicalProcessorInformationEx( RelationCache, nullptr, &buffLength );

	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* sysInfos = new SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX[buffLength];
	BOOL result = GetLogicalProcessorInformationEx( RelationCache, sysInfos, &buffLength );

	if ( result == FALSE ) {
		delete sysInfos;
		return 0;
	}

	unsigned short cacheSize = sysInfos->Cache.LineSize;
	delete sysInfos;

	return cacheSize;
}

const int Env_GetCPUCoreCount()
{
	SYSTEM_INFO systemInfo = {};
	GetSystemInfo( &systemInfo );

	return systemInfo.dwNumberOfProcessors;
}

const unsigned long long Env_GetRAMTotalSize()
{
	MEMORYSTATUSEX memStatusEx = {};
	memStatusEx.dwLength = sizeof( memStatusEx );

	BOOL opResult = GlobalMemoryStatusEx( &memStatusEx );

	if ( opResult == FALSE ) {
		return 0;
	}

	return static_cast< const unsigned long long >( memStatusEx.ullTotalPhys >> 20 );
}

const char* Env_GetCPUName()
{
	static char cpuName[128] = {};
	int cpuInfos[4] = {};
	int cpuLetter = -1;

	CPU_ID( cpuInfos, 0x80000000 );

	if ( cpuInfos[0] >= 0x80000004 ) {
		for ( unsigned int i = 0x80000002; i < 0x80000005; ++i ) {
			CPU_ID( cpuInfos, i );

			for ( const int info : cpuInfos ) {
				cpuName[++cpuLetter] = static_cast<char>( info >> ( 8 * 0 ) ) & 0xFF;
				cpuName[++cpuLetter] = static_cast<char>( info >> ( 8 * 1 ) ) & 0xFF;
				cpuName[++cpuLetter] = static_cast<char>( info >> ( 8 * 2 ) ) & 0xFF;
				cpuName[++cpuLetter] = static_cast<char>( info >> ( 8 * 3 ) ) & 0xFF;
			}
		}
	}

	return cpuName;
}

const char* Env_GetOSName()
{
	static char osName[64] = {};

	if ( IsWindows10OrGreater() ) {
		strcpy_s( osName, "Windows 10" );
	} else if ( IsWindows8Point1OrGreater() ) {
		strcpy_s( osName, "Windows 8.1" );
	} else if ( IsWindows8OrGreater() ) {
		strcpy_s( osName, "Windows 8" );
	} else if ( IsWindows7SP1OrGreater() ) {
		strcpy_s( osName, "Windows 7 SP1" );
	} else if ( IsWindows7OrGreater() ) {
		strcpy_s( osName, "Windows 7" );
	} else if ( IsWindowsVistaSP2OrGreater() ) {
		strcpy_s( osName, "Windows Vista SP2" );
	} else if ( IsWindowsVistaSP1OrGreater() ) {
		strcpy_s( osName, "Windows Vista SP1" );
	} else if ( IsWindowsVistaOrGreater() ) {
		strcpy_s( osName, "Windows Vista" );
	} else if ( IsWindowsXPSP3OrGreater() ) {
		strcpy_s( osName, "Windows XP SP3" );
	} else if ( IsWindowsXPSP2OrGreater() ) {
		strcpy_s( osName, "Windows XP SP2" );
	} else if ( IsWindowsXPSP1OrGreater() ) {
		strcpy_s( osName, "Windows XP SP1" );
	} else if ( IsWindowsXPOrGreater() ) {
		strcpy_s( osName, "Windows XP" );
	} else {
		strcpy_s( osName, "Windows ???" );
	}

	return osName;
}
