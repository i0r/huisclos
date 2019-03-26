#pragma once

#include <vector>
#include <string>

struct submeshEntry_t
{
	unsigned int vboOffset;
	unsigned int iboOffset;
	unsigned int indiceCount;
	unsigned int matHashcode;
};

struct mesh_load_data_t
{
	float*			vbo;
	unsigned int*	ibo;
	unsigned int	vboSize;
	unsigned int	iboSize;

	std::vector<submeshEntry_t>							submeshesToLoad;
	std::vector<std::pair<unsigned int, std::string>>	materialsToLoad;
};

const int Io_ReadSmallGeometryFile( const char* fileName, mesh_load_data_t& data );
