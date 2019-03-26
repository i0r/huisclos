#include "Shared.h"
#include "SmallGeometryFileReader.h"
#include <fstream>

using blobMagic_t = unsigned int;

struct smallGeometryHeader_t
{
	unsigned char	versionMajor;
	unsigned char   versionMinor;
	unsigned char   versionPatch;
	unsigned char   meshFeatures;

	unsigned int	dataStartOffset;
	unsigned int	verticesSize;
	unsigned int    indiceSize;
};

struct blobHeader_t
{
	blobMagic_t		magic;
	unsigned int	size;
	uint64_t		__PADDING__;
};

const std::size_t Io_GetFileStreamSize( std::istream& stream, const std::streamoff currentStreamOffset = 0 )
{
	stream.seekg( 0, std::ios_base::end );
	const std::size_t fileSize = stream.tellg();
	stream.seekg( currentStreamOffset, std::ios_base::beg );

	return fileSize;
}

// V2.0 SGO file reader
const int Io_ReadSmallGeometryFile( const char* fileName, mesh_load_data_t& data )
{
	std::ifstream fileStream( fileName, std::ios::binary | std::ios::in );

	if ( fileStream.is_open() ) {
		const std::size_t fileSize = Io_GetFileStreamSize( fileStream );

		if ( fileSize < sizeof( smallGeometryHeader_t ) ) {
			return 2;
		}

		smallGeometryHeader_t fileHeader = {};
		fileStream.read( ( char* )&fileHeader, sizeof( smallGeometryHeader_t ) );

		while ( static_cast<unsigned int>( fileStream.tellg() ) < fileHeader.dataStartOffset ) {
			blobHeader_t header = {};
			fileStream.read( ( char* )&header, sizeof( blobHeader_t ) );

			switch ( header.magic ) {
			case 0x4C54414D: { // MATL - MATerial Library
				const std::streampos blobEndOffset = static_cast<std::size_t>( fileStream.tellg() ) + header.size;

				while ( fileStream.tellg() < blobEndOffset ) {
					blobMagic_t matMagic = 0;
					fileStream.read( ( char* )&matMagic, sizeof( blobMagic_t ) );

					std::string matFileName = {};
					std::getline( fileStream, matFileName, '\0' );

					data.materialsToLoad.push_back( std::make_pair( matMagic, matFileName ) );
				}

			} break;

			case 0x4D425553: { // SUBM - SUBMeshes
				const std::streampos blobEndOffset = static_cast<std::size_t>( fileStream.tellg() ) + header.size;

				while ( fileStream.tellg() < blobEndOffset ) {
					submeshEntry_t subMesh = {};
					fileStream.read( ( char* )&subMesh, sizeof( submeshEntry_t ) );

					data.submeshesToLoad.push_back( subMesh );
				}
			} break;

			default: // if there is no magic to read...
				continue;
			}

			int streamPos = ( fileStream.tellg() % 16 );

			if ( streamPos <= 0 ) streamPos = 16;

			fileStream.ignore( 16 - streamPos );
		}

		fileStream.seekg( fileHeader.dataStartOffset ); // seeking in io is bad, I know...

		data.vboSize	= fileHeader.verticesSize;
		data.vbo		= new float[data.vboSize / sizeof( float )]();
		fileStream.read( ( char* )data.vbo, fileHeader.verticesSize );

		data.iboSize	= fileHeader.indiceSize;
		data.ibo		= new unsigned int[data.iboSize / sizeof( unsigned int )]();
		fileStream.read( ( char* )data.ibo, fileHeader.indiceSize );
	} else {
		return 1;
	}

	fileStream.close();

	return 0;
}
