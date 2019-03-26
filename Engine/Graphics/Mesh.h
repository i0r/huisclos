#pragma once

struct renderContext_t;

class TextureManager;

#include "Material.h"
#include <vector>

struct transform_t 
{
	DirectX::BoundingSphere	boundingSphere;
	DirectX::XMFLOAT3	translation;
	DirectX::XMFLOAT4	rotation;
	DirectX::XMFLOAT3	scale;
	DirectX::XMMATRIX	modelMatrix;
};

struct submesh_t
{
	material_t*			material;		// 8 // TMP: this should be a pointer instead (still need to create a material manager)
	
	transform_t*		transformation; // 8
	
	unsigned int		vboOffset;		// 4
	unsigned int		iboOffset;		// 4

	unsigned int		indiceCount;	// 4
	unsigned int		__PADDING__;	// 4
};

struct mesh_t
{
	ID3D11Buffer*			vertexBuffer;	// 8
	ID3D11Buffer*			indiceBuffer;	// 8

	int						vertexCount;	// 4
	int						indiceCount;	// 4

	transform_t*			transformation; // 8

	std::vector<submesh_t>	subMeshes;
};

void	Render_BindMesh( const renderContext_t* context, mesh_t* mesh );
int		Render_CreateMeshFromFile( const renderContext_t* context, MaterialManager* matMan, mesh_t* mesh, const char* fileName );
void	Render_ReleaseMesh( mesh_t* mesh );
