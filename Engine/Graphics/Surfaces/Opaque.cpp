#include "Shared.h"
#include <d3d11.h>
#include "Opaque.h"
#include <d3dcompiler.h>

#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/RenderContext.h>
#include <Engine/Graphics/CBuffer.h>
#include <Engine/Graphics/Camera.h>

struct matModelBuffer_t
{
	DirectX::XMMATRIX model;
};

SurfaceOpaque::SurfaceOpaque()
	: vertexShader( nullptr )
	, pixelShader( nullptr )
	, shaderLayout( nullptr )
	, samplerState( nullptr )
{

}

void SurfaceOpaque::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShader )
	RELEASE( shaderLayout )
	RELEASE( samplerState )
}

const int SurfaceOpaque::Create( const renderContext_t* context )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer	= nullptr;
	ID3D10Blob*		pixelShaderBuffer	= nullptr;

	D3DReadFileToBlob( L"base_data/shaders/opaque_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/opaque_ps.cso", &pixelShaderBuffer );

	if ( FAILED( context->device->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader ) ) ) {
		return 1;
	}

	if ( FAILED( context->device->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader ) ) ) {
		return 2;
	}

	D3D11_INPUT_ELEMENT_DESC layoutDesc[5] = {};
	layoutDesc[0].SemanticName			= "POSITION";
	layoutDesc[0].SemanticIndex			= 0;
	layoutDesc[0].Format				= DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[0].InputSlot				= 0;
	layoutDesc[0].AlignedByteOffset		= 0;
	layoutDesc[0].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[0].InstanceDataStepRate	= 0;

	layoutDesc[1].SemanticName			= "NORMAL";
	layoutDesc[1].SemanticIndex			= 0;
	layoutDesc[1].Format				= DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[1].InputSlot				= 0;
	layoutDesc[1].AlignedByteOffset		= D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[1].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[1].InstanceDataStepRate	= 0;

	layoutDesc[2].SemanticName			= "TEXCOORD";
	layoutDesc[2].SemanticIndex			= 0;
	layoutDesc[2].Format				= DXGI_FORMAT_R32G32_FLOAT;
	layoutDesc[2].InputSlot				= 0;
	layoutDesc[2].AlignedByteOffset		= D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[2].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[2].InstanceDataStepRate	= 0;

	layoutDesc[3].SemanticName			= "TANGENT";
	layoutDesc[3].SemanticIndex			= 0;
	layoutDesc[3].Format				= DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[3].InputSlot				= 0;
	layoutDesc[3].AlignedByteOffset		= D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[3].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[3].InstanceDataStepRate	= 0;

	layoutDesc[4].SemanticName			= "BINORMAL";
	layoutDesc[4].SemanticIndex			= 0;
	layoutDesc[4].Format				= DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[4].InputSlot				= 0;
	layoutDesc[4].AlignedByteOffset		= D3D11_APPEND_ALIGNED_ELEMENT;
	layoutDesc[4].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[4].InstanceDataStepRate	= 0;

	if ( FAILED( context->device->CreateInputLayout( layoutDesc, 5, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &shaderLayout ) ) ) {
		return 3;
	}
	
	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 8;

	if ( FAILED( context->device->CreateSamplerState( &samplerDesc, &samplerState ) ) ) {
		return 4;
	}

	D3D11_SAMPLER_DESC shadowSamplerDesc = {};
	shadowSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

	if ( FAILED( context->device->CreateSamplerState( &shadowSamplerDesc, &shadowSamplerState ) ) ) {
		return 4;
	}

	Render_CreateCBuffer( context, cbuffer, sizeof( matModelBuffer_t ) );

	context->deviceContext->VSSetConstantBuffers( 2, 1, &cbuffer );

	return 0;
}

void SurfaceOpaque::Render( const renderContext_t* context, const mesh_t* mesh, const Camera* camera )
{
	context->deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	context->deviceContext->IASetInputLayout( shaderLayout );

	context->deviceContext->VSSetShader( vertexShader, NULL, 0 );
	context->deviceContext->PSSetShader( pixelShader, NULL, 0 );

	context->deviceContext->PSSetSamplers( 0, 1, &samplerState );
	context->deviceContext->PSSetSamplers( 1, 1, &shadowSamplerState );

	Render_UploadCBuffer( context, cbuffer, &mesh->transformation->modelMatrix, sizeof( matModelBuffer_t ) );
	context->deviceContext->VSSetConstantBuffers( 2, 1, &cbuffer );

	for ( const submesh_t& subMesh : mesh->subMeshes ) {
		/*if ( !cam.frustum.Contains( subMesh.transformation->boundingSphere ) ) {
			continue;
		}*/

		// this is ineffective as hell
		// TODO: proper model matrix management
		// ( model matrix pool using a cbuffer? just need a way to identify each submesh by hashing or id)

		if ( subMesh.material->alpha != nullptr ) {
			D3D11_BLEND_DESC omDesc = {};
			omDesc.RenderTarget[0].BlendEnable = TRUE;
			omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			omDesc.RenderTarget[0].SrcBlend			= D3D11_BLEND_SRC_ALPHA;
			omDesc.RenderTarget[0].DestBlend		= D3D11_BLEND_INV_SRC_ALPHA;

			omDesc.RenderTarget[0].SrcBlendAlpha	= D3D11_BLEND_ONE;
			omDesc.RenderTarget[0].DestBlendAlpha	= D3D11_BLEND_ONE;

			omDesc.RenderTarget[0].BlendOp			= D3D11_BLEND_OP_ADD;
			omDesc.RenderTarget[0].BlendOpAlpha		= D3D11_BLEND_OP_ADD;

			ID3D11BlendState* mBlendState = NULL;
			context->device->CreateBlendState( &omDesc, &mBlendState );

			float blendFactor[] = { 0,0, 0, 0 };
			UINT sampleMask = 0xffffffff;
			context->deviceContext->OMSetBlendState( mBlendState, blendFactor, sampleMask );
		}

		Render_BindOpaqueMaterial( context->deviceContext, subMesh.material );
		context->deviceContext->DrawIndexed( subMesh.indiceCount, subMesh.iboOffset, 0 );
		context->deviceContext->OMSetBlendState( NULL, NULL, 0xFFFFFFFF );
	}
}
