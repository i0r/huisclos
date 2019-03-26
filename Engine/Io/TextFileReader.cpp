#include "Shared.h"
#include "SmallGeometryFileReader.h"

#include <string>
#include <fstream>

const int Io_ReadTextFile( const char* fileName, char*& fileContent, unsigned int& contentLength )
{
	std::ifstream fileStream( fileName );

	if ( fileStream.is_open() ) {
		// get file size
		fileStream.seekg( 0, std::ios_base::end );
		contentLength = static_cast< unsigned int >( fileStream.tellg() );
		fileStream.seekg( 0, std::ios_base::beg );

		fileContent = new char[contentLength * sizeof( char )]{ '\0' };
		fileStream.read( fileContent, contentLength * sizeof( char ) );
	} else {
		return 1;
	}

	fileStream.close();
	return 0;
}
