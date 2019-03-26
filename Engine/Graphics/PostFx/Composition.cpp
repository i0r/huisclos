#include "Shared.h"
#include <d3d11.h>
#include "Composition.h"
#include <d3dcompiler.h>

#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/RenderContext.h>

CompositionPass::CompositionPass()
	: vertexShader( nullptr )
	, pixelShader( nullptr )
	, samplerState( nullptr )
	, activeTarget( 0 )
{

}

void CompositionPass::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShader )
	RELEASE( samplerState )
}

int CompositionPass::Create( renderContext_t* context, TextureManager* texMan )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer = nullptr;
	ID3D10Blob*		pixelShader2Buffer = nullptr;

	D3DReadFileToBlob( L"base_data/shaders/postfx_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/composition_ps.cso", &pixelShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/autoexposure_ps.cso", &pixelShader2Buffer );

	result = context->device->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );
	if ( FAILED( result ) ) {
		return 1;
	}

	result = context->device->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader );
	if ( FAILED( result ) ) {
		return 2;
	}

	result = context->device->CreatePixelShader( pixelShader2Buffer->GetBufferPointer(), pixelShader2Buffer->GetBufferSize(), NULL, &pixelShaderAvgLuminances );
	if ( FAILED( result ) ) {
		return 2;
	}

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	context->device->CreateSamplerState( &samplerDesc, &samplerState );

	D3D11_SAMPLER_DESC samplerDescMipMap = {};
	samplerDescMipMap.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDescMipMap.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescMipMap.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescMipMap.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescMipMap.MinLOD = 0;
	samplerDescMipMap.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDescMipMap.MaxAnisotropy = 8;

	if ( FAILED( context->device->CreateSamplerState( &samplerDescMipMap, &samplerStateMipMap ) ) ) {
		return 4;
	}

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();
	pixelShader2Buffer->Release();

	D3D11_RENDER_TARGET_VIEW_DESC   renderTargetViewDesc;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	renderTargetViewDesc.Texture3D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;

	D3D11_TEXTURE2D_DESC mainRtDesc = {
		static_cast<UINT>( 1024 ),
		static_cast<UINT>( 1024 ),
		0,
		1,
		DXGI_FORMAT_R32_FLOAT,
		{																					// DXGI_SAMPLE_DESC SampleDesc
			1,																				//		UINT Count
			0,																				//		UINT Quality
		},
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		D3D11_RESOURCE_MISC_GENERATE_MIPS,
	};

	renderTargetViewDesc.Format = mainRtDesc.Format;
	shaderResourceViewDesc.Format = mainRtDesc.Format;

	for ( int i = 0; i < 2; ++i ) {
		if ( FAILED( context->device->CreateTexture2D( &mainRtDesc, nullptr, &averageLuminance[i].tex2D ) ) ) {
			return 3;
		}

		if ( FAILED( context->device->CreateRenderTargetView( averageLuminance[i].tex2D, &renderTargetViewDesc, &averageLuminance[i].view ) ) ) {
			return 3;
		}

		if ( FAILED( context->device->CreateShaderResourceView( averageLuminance[i].tex2D, &shaderResourceViewDesc, &averageLuminance[i].ressource ) ) ) {
			return 3;
		}

		context->deviceContext->GenerateMips( averageLuminance[i].ressource );
	}

	return 0;
}

void CompositionPass::Render( const renderContext_t* context, const renderTarget_t* mainRt, const renderTarget_t* bloomRt )
{
	static constexpr FLOAT CLEAR_COLOR[4] = { 0.5294f, 0.8078f, 0.9803f, 1.0f };

	// common stuff
	ID3D11ShaderResourceView *const renderTargets[2] = { mainRt->ressource, bloomRt->ressource };
	context->deviceContext->PSSetShaderResources( 0, 2, renderTargets );

	context->deviceContext->PSSetSamplers( 0, 1, &samplerState );
	context->deviceContext->PSSetSamplers( 1, 1, &samplerStateMipMap );
	
	// compute average luminance
	D3D11_VIEWPORT backupViewport = {};
	UINT viewportCount = 1;
	context->deviceContext->RSGetViewports( &viewportCount, &backupViewport );

	// set shadow viewport
	D3D11_VIEWPORT viewport =
	{
		0.0f,
		0.0f,
		1024.0f,
		1024.0f,
		0.0f,
		1.0f,
	};

	context->deviceContext->RSSetViewports( 1, &viewport );

	context->deviceContext->OMSetRenderTargets( 1, &averageLuminance[activeTarget].view, NULL );
	context->deviceContext->ClearRenderTargetView( averageLuminance[activeTarget].view, CLEAR_COLOR );

	context->deviceContext->VSSetShader( vertexShader, NULL, 0 );
	context->deviceContext->PSSetShader( pixelShaderAvgLuminances, NULL, 0 );

	context->deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	context->deviceContext->Draw( 6, 0 );

	context->deviceContext->GenerateMips( averageLuminance[activeTarget].ressource );

	// restore previous viewport
	context->deviceContext->RSSetViewports( 1, &backupViewport );

	// compose our frame
	context->deviceContext->OMSetRenderTargets( 1, &context->backBuffer, NULL );
	context->deviceContext->ClearRenderTargetView( context->backBuffer, CLEAR_COLOR );

	context->deviceContext->PSSetShaderResources( 2, 1, &averageLuminance[activeTarget].ressource );
	context->deviceContext->PSSetShaderResources( 3, 1, &averageLuminance[activeTarget == 0 ? 1 : 0].ressource );

	( activeTarget == 1 ) ? activeTarget-- : activeTarget++;

	context->deviceContext->PSSetShader( pixelShader, NULL, 0 );

	context->deviceContext->Draw( 6, 0 );

	// unbind the ressource so that we can use the render target on the next frame
	ID3D11ShaderResourceView *const pSRV[3] = { nullptr, nullptr, nullptr };
	context->deviceContext->PSSetShaderResources( 0, 3, pSRV );
}
