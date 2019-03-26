#pragma once

#include <vector>
#include <string>

struct areaHeader_t
{
	unsigned char indexX;
	unsigned char indexY;
	unsigned short __PADDING__;
	unsigned int  areaFlags;
	unsigned int  nodeCount;
	unsigned int  hashcode;
};

struct worldArea_t;

const int Io_WriteAreaFile( const char* fileName, const worldArea_t* area );
