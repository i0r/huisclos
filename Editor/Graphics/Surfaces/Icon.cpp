#include <d3d11.h>
#include "Icon.h"
#include <d3dcompiler.h>

#include <Engine/Graphics/RenderContext.h>
#include <Engine/Graphics/CBuffer.h>
#include <Engine/Graphics/Texture.h>

SurfaceIcon::SurfaceIcon()
{

}

void SurfaceIcon::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }
}

const int SurfaceIcon::Create( const renderContext_t* context, TextureManager* texMan )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer = nullptr;

	D3DReadFileToBlob( L"base_data/shaders/ed_icon_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/ed_icon_ps.cso", &pixelShaderBuffer );

	result = context->device->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );
	if ( FAILED( result ) ) {
		return 1;
	}

	result = context->device->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader );
	if ( FAILED( result ) ) {
		return 2;
	}

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 8;

	context->device->CreateSamplerState( &samplerDesc, &samplerState );

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	Render_CreateCBuffer( context, cbuffer, sizeof( edEntityIcon_t ) );

	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory( &blendStateDesc, sizeof( D3D11_BLEND_DESC ) );
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	context->device->CreateBlendState( &blendStateDesc, &blendState );

    icons[ED_ICON_ERROR] = texMan->GetTexture( context, "base_data/textures/editor/icon_error.dds" );
    icons[ED_ICON_SPHERE_LIGHT] = texMan->GetTexture( context, "base_data/textures/editor/icon_spherelight.dds" );
    icons[ED_ICON_DISK_LIGHT] = texMan->GetTexture( context, "base_data/textures/editor/icon_disklight.dds" );
	icons[ED_ICON_SUN_LIGHT] = texMan->GetTexture( context, "base_data/textures/editor/icon_sunlight.dds" );

	return 0;
}

void SurfaceIcon::Render( const renderContext_t* context, const edEntityIcon_t& iconPos, const edIcons_t iconId )
{
	Render_UploadCBuffer( context, cbuffer, &iconPos, sizeof( edEntityIcon_t ) );
	context->deviceContext->VSSetConstantBuffers( 6, 1, &cbuffer );

	context->deviceContext->VSSetShader( vertexShader, NULL, 0 );
	context->deviceContext->PSSetShader( pixelShader, NULL, 0 );

	context->deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	context->deviceContext->PSSetSamplers( 0, 1, &samplerState );

	context->deviceContext->PSSetShaderResources( 0, 1, &icons[iconId]->view );

	context->deviceContext->OMSetBlendState( blendState, 0, 0xffffffff );
	context->deviceContext->Draw( 6, 0 );
	context->deviceContext->OMSetBlendState( NULL, 0, 0xffffffff );
}
