#pragma once

#include <map>

using dictionary_t = std::map<std::string, std::string>;

const int Io_ReadDictionaryFile( const char* fileName, dictionary_t& dico );
