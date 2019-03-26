#include "Shared.h"
#include "DictionaryReader.h"

#include <string>
#include <fstream>

namespace
{
	void Trim( std::string &str )
	{
		size_t charPos = str.find_first_not_of( "     \n" );

		if ( charPos != std::string::npos ) {
			str.erase( 0, charPos );
		}

		charPos = str.find_last_not_of( "     \n" );

		if ( charPos != std::string::npos ) {
			str.erase( charPos + 1 );
		}
	}
}

const int Io_ReadDictionaryFile( const char* fileName, dictionary_t& dico )
{
	std::ifstream fStream( fileName );

	if ( fStream.is_open() ) {
		std::string fLine = "",
			dicoKey = "",
			dicoVal = "";

		while ( fStream.good() ) {
			std::getline( fStream, fLine );

			const size_t separator = fLine.find_first_of( ':' );
			const size_t commentSep = fLine.find_first_of( ';' );

			if ( commentSep != -1 ) {
				fLine.erase( fLine.begin() + commentSep, fLine.end() );
			}

			if ( !fLine.empty() && separator != std::string::npos ) {
				dicoKey = fLine.substr( 0, separator );
				dicoVal = fLine.substr( separator + 1 );

				Trim( dicoKey );
				Trim( dicoVal );

				if ( !dicoVal.empty() ) {
					dico.insert( std::make_pair( dicoKey, dicoVal ) );
				}
			}
		}
	} else {
		return 1;
	}

	fStream.close();
	return 0;
}
