#include "Shared.h"
#include <d3d11.h>
#include "Mesh.h"

#include "RenderContext.h"

#include <Engine/ThirdParty/DirectXTK/Inc/SimpleMath.h>
#include <Engine/Io/SmallGeometryFileReader.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Material.h>

struct defaultVertexLayout_t
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uvCoord;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 bitangent;
};

void Render_BindMesh( const renderContext_t* context, mesh_t* mesh )
{
	unsigned int stride = sizeof( defaultVertexLayout_t );
	unsigned int offset = 0;

	context->deviceContext->IASetVertexBuffers( 0, 1, &mesh->vertexBuffer, &stride, &offset );
	context->deviceContext->IASetIndexBuffer( mesh->indiceBuffer, DXGI_FORMAT_R32_UINT, 0 );
	context->deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
}

int Render_CreateMeshFromFile( const renderContext_t* context, MaterialManager* matMan, mesh_t* mesh, const char* fileName )
{
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	
	mesh_load_data_t data = {};
	if ( Io_ReadSmallGeometryFile( fileName, data ) != 0 ) {
		return 1;
	}

	vertexBufferDesc.Usage					= D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth				= data.vboSize;
	vertexBufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags			= 0;
	vertexBufferDesc.MiscFlags				= 0;
	vertexBufferDesc.StructureByteStride	= 0;

	vertexData.pSysMem			= data.vbo;
	vertexData.SysMemPitch		= 0;
	vertexData.SysMemSlicePitch = 0;

	if ( FAILED( context->device->CreateBuffer( &vertexBufferDesc, &vertexData, &mesh->vertexBuffer ) ) ) {
		return 2;
	}

	indexBufferDesc.Usage				= D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth			= data.iboSize;
	indexBufferDesc.BindFlags			= D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags		= 0;
	indexBufferDesc.MiscFlags			= 0;
	indexBufferDesc.StructureByteStride	= 0;

	indexData.pSysMem			= data.ibo;
	indexData.SysMemPitch		= 0;
	indexData.SysMemSlicePitch	= 0;

	if ( FAILED( context->device->CreateBuffer( &indexBufferDesc, &indexData, &mesh->indiceBuffer ) ) ) {
		return 3;
	}

	mesh->indiceCount = data.iboSize / sizeof( unsigned int );
	mesh->vertexCount = data.vboSize / sizeof( defaultVertexLayout_t );

	mesh->transformation = new transform_t();
	mesh->transformation->modelMatrix = DirectX::XMMatrixIdentity();
	DirectX::BoundingSphere::CreateFromPoints( mesh->transformation->boundingSphere, mesh->indiceCount, ( DirectX::XMFLOAT3* )data.vbo, sizeof( defaultVertexLayout_t ) );

	for ( const std::pair<unsigned int, std::string>& matToLoad : data.materialsToLoad ) {
		const std::string fullMatPath = "base_data/materials/" + matToLoad.second;

		material_t* mat = matMan->GetMaterial( fullMatPath.c_str() );
		if ( mat == nullptr ) {
			continue; // cant load material (incomplete or smthing like that)
		}

		for ( submeshEntry_t& sme : data.submeshesToLoad ) {
			if ( sme.matHashcode == matToLoad.first ) {
				submesh_t subMesh = {
					mat,
					new transform_t(),
					sme.vboOffset,
					sme.iboOffset,
					sme.indiceCount,
				};

				subMesh.transformation->modelMatrix = DirectX::XMMatrixIdentity();

				float* startOffset = data.vbo + sme.vboOffset;
				DirectX::BoundingSphere::CreateFromPoints( subMesh.transformation->boundingSphere, sme.indiceCount, ( DirectX::XMFLOAT3* )startOffset, sizeof( defaultVertexLayout_t ) );

				mesh->subMeshes.push_back( subMesh );
			}
		}
	}

	delete data.vbo;
	delete data.ibo;

	return 0;
}

void Render_ReleaseMesh( mesh_t* mesh )
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( mesh->vertexBuffer )
	RELEASE( mesh->indiceBuffer )

	memset( mesh, 0, sizeof( mesh_t ) );
}
