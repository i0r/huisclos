#include "Shared.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "ShadowMapping.h"

#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/RenderContext.h>

ShadowMapping::ShadowMapping()
	: vertexShader( nullptr )
	, pixelShader( nullptr )
	, shaderLayout( nullptr )
{

}

void ShadowMapping::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShader )
	RELEASE( shaderLayout )
}

const int ShadowMapping::Create( const renderContext_t* context )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer = nullptr;

	D3DReadFileToBlob( L"base_data/shaders/shadow_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/shadow_ps.cso", &pixelShaderBuffer );

	result = context->device->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );
	if ( FAILED( result ) ) {
		return 1;
	}

	result = context->device->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader );
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


	result = context->device->CreateInputLayout( layoutDesc, 5, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &shaderLayout );
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

	if ( FAILED( context->device->CreateSamplerState( &samplerDesc, &samplerState ) ) ) {
		return 4;
	}

	Render_CreateCBuffer( context, matricesCbuffer, sizeof( matricesData ) );

	return 0;
}

void ShadowMapping::RenderDiskAreaLight( const renderContext_t* context, const mesh_t* mesh, const diskAreaLight_t& light )
{
	context->deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	context->deviceContext->IASetInputLayout( shaderLayout );

	context->deviceContext->VSSetShader( vertexShader, NULL, 0 );
	context->deviceContext->PSSetShader( pixelShader, NULL, 0 );

	context->deviceContext->PSSetSamplers( 0, 1, &samplerState );

	DirectX::XMMATRIX lightProj = DirectX::XMMatrixPerspectiveFovLH( 180.0f, 1.0f, 0.01f, 128.0f * light.worldPositionRadius.w );

	DirectX::XMFLOAT3 top = DirectX::XMFLOAT3( 0.0f, 1.0f, 0.0f );
	DirectX::XMVECTOR topVec = DirectX::XMLoadFloat3( &top );

	DirectX::XMFLOAT3 worldPos = DirectX::XMFLOAT3( light.worldPositionRadius.x, light.worldPositionRadius.y, light.worldPositionRadius.z - ( 10.0f * light.worldPositionRadius.w ) );

	DirectX::XMVECTOR positionVector = DirectX::XMLoadFloat3( &worldPos );

	DirectX::XMFLOAT3 eyeLookAt = DirectX::XMFLOAT3( light.planeNormal.x, light.planeNormal.y, light.planeNormal.z );
	DirectX::XMVECTOR eyeVec = DirectX::XMLoadFloat3( &eyeLookAt );

	DirectX::XMVECTOR lookAtVector = DirectX::XMVectorAdd( positionVector, eyeVec );

	DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH( positionVector, lookAtVector, topVec );

	matricesData.lightMatrix = DirectX::XMMatrixTranspose( lightView * lightProj );

	for ( const submesh_t& subMesh : mesh->subMeshes ) {
		if ( subMesh.material->colorData.flags & MAT_FLAG_IS_SHADELESS ) { // do not include light emitters in the shadow mapping
			continue;
		}

		matricesData.modelMatrix = subMesh.transformation->modelMatrix;

		Render_UploadCBuffer( context, matricesCbuffer, &matricesData, sizeof( matricesData ) );

		context->deviceContext->VSSetConstantBuffers( 4, 1, &matricesCbuffer );

		context->deviceContext->DrawIndexed( subMesh.indiceCount, subMesh.iboOffset, 0 );
	}
}

void ShadowMapping::RenderSun( const renderContext_t* context, sunLight_t& sun )
{
	ID3D11RenderTargetView* nullRenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { NULL };
	context->deviceContext->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRenderTargets, sun.cascadeAtlas.depthView );

	D3D11_VIEWPORT backupViewport = {};
	UINT viewportCount = 1;
	context->deviceContext->RSGetViewports( &viewportCount, &backupViewport );

	// set shadow viewport
	D3D11_VIEWPORT viewport =
	{
		0.0f,
		0.0f,
		2048.0f,
		2048.0f,
		0.0f,
		1.0f,
	};

	context->deviceContext->RSSetViewports( 1, &viewport );

	// restore previous viewport
	context->deviceContext->RSSetViewports( 1, &backupViewport );
}
