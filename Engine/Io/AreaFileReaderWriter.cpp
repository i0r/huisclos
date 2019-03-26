#include "Shared.h"
#include "AreaFileReaderWriter.h"

#include <Engine/Game/World.h>

#include <fstream>

const int Io_WriteAreaFile( const char* fileName, const worldArea_t* area )
{
	std::ofstream fileStream( fileName, std::ios::binary | std::ios::out );

	if ( fileStream.good() ) {
		areaHeader_t header = {
			0,
			0,
			0xFFFF,
			0x0,
			0,
			0xFFFFFFFF,
		};

		fileStream.write( ( const char* )&header, sizeof( areaHeader_t ) );

		constexpr unsigned int pad = 0xFFFFFFFF;
		for ( const areaNode_t* node : area->nodes->children ) {
			fileStream.write( ( const char* )&node->hash, sizeof( unsigned int ) );
			fileStream.write( ( const char* )&node->flags, sizeof( uint64_t ) );
			fileStream.write( ( const char* )&pad, sizeof( unsigned int ) );
		}

		fileStream.close();
	}

	return 0;
}
