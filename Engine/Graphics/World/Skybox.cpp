#include "Shared.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Skybox.h"

#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Graphics/RenderContext.h>

Skybox::Skybox()
	: vertexShader( nullptr )
	, pixelShader( nullptr )
	, shaderLayout( nullptr )
	, envMap( nullptr )
{

}

void Skybox::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShader )
	RELEASE( shaderLayout )
}

const int Skybox::Create( const renderContext_t* context, MaterialManager* matMan, ID3D11Device* dev )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer = nullptr;

	D3DReadFileToBlob( L"base_data/shaders/skybox_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/skybox_ps.cso", &pixelShaderBuffer );

	result = dev->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );
	if ( FAILED( result ) ) {
		return 1;
	}

	result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader );
	if ( FAILED( result ) ) {
		return 2;
	}

	D3D11_INPUT_ELEMENT_DESC layoutDesc[5] = {};
	layoutDesc[0].SemanticName = "POSITION";
	layoutDesc[0].SemanticIndex = 0;
	layoutDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[0].InputSlot = 0;
	layoutDesc[0].AlignedByteOffset = 0;
	layoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[0].InstanceDataStepRate = 0;

	layoutDesc[1].SemanticName = "NORMAL";
	layoutDesc[1].SemanticIndex = 0;
	layoutDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[1].InputSlot = 0;
	layoutDesc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[1].InstanceDataStepRate = 0;

	layoutDesc[2].SemanticName = "TEXCOORD";
	layoutDesc[2].SemanticIndex = 0;
	layoutDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	layoutDesc[2].InputSlot = 0;
	layoutDesc[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[2].InstanceDataStepRate = 0;

	layoutDesc[3].SemanticName = "TANGENT";
	layoutDesc[3].SemanticIndex = 0;
	layoutDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[3].InputSlot = 0;
	layoutDesc[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[3].InstanceDataStepRate = 0;

	layoutDesc[4].SemanticName = "BINORMAL";
	layoutDesc[4].SemanticIndex = 0;
	layoutDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[4].InputSlot = 0;
	layoutDesc[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[4].InstanceDataStepRate = 0;


	result = dev->CreateInputLayout( layoutDesc, 5, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &shaderLayout );
	if ( FAILED( result ) ) {
		return 3;
	}

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 8;

	if ( FAILED( dev->CreateSamplerState( &samplerDesc, &samplerState ) ) ) {
		return 4;
	}

	Render_CreateMeshFromFile( context, matMan, &skyboxGeometry, "base_data/meshes/world/skybox.sge" );

	return 0;
}

const bool Skybox::LoadEnvMap( const renderContext_t* context, TextureManager* texMan, MaterialManager* matMan, const char* resFilePath )
{
	envMap = texMan->GetTexture( context, resFilePath );

	return envMap != nullptr;
}

void Skybox::Render( const renderContext_t* context )
{
	context->deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	context->deviceContext->IASetInputLayout( shaderLayout );

	context->deviceContext->VSSetShader( vertexShader, NULL, 0 );
	context->deviceContext->PSSetShader( pixelShader, NULL, 0 );

	context->deviceContext->PSSetSamplers( 0, 1, &samplerState );

	Render_BindMesh( context, &skyboxGeometry );
	for ( const submesh_t& subMesh :skyboxGeometry.subMeshes ) {
		context->deviceContext->PSSetShaderResources( 0, 1, &envMap->view );
		context->deviceContext->DrawIndexed( subMesh.indiceCount, subMesh.iboOffset, 0 );
	}
}
