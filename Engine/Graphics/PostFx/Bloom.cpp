#include "Shared.h"
#include <d3d11.h>
#include "Bloom.h"
#include <d3dcompiler.h>

#include <Engine/Graphics/RenderContext.h>
#include "GaussianBlur.h"

BloomPass::BloomPass()
	: pingPong( false )
	, finalRenderTarget( nullptr )
{

}

void BloomPass::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }
}

int BloomPass::Create( const renderContext_t* renderContext, const int winWidth, const int winHeight )
{
	D3D11_RENDER_TARGET_VIEW_DESC   renderTargetViewDesc;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	renderTargetViewDesc.Texture3D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;
	shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture3D.MipLevels = -1;

	baseWidth = winWidth;
	baseHeight = winHeight;

	D3D11_TEXTURE2D_DESC mainRtDesc = {
		static_cast<UINT>( winWidth ),
		static_cast<UINT>( winHeight ),
		1,
		1,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		{																					// DXGI_SAMPLE_DESC SampleDesc
			4,																				//		UINT Count
			0,																				//		UINT Quality
		},
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
	};

	renderTargetViewDesc.Format = mainRtDesc.Format;
	shaderResourceViewDesc.Format = mainRtDesc.Format;

	if ( FAILED( renderContext->device->CreateTexture2D( &mainRtDesc, nullptr, &renderTargetMS.tex2D ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext->device->CreateRenderTargetView( renderTargetMS.tex2D, &renderTargetViewDesc, &renderTargetMS.view ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext->device->CreateShaderResourceView( renderTargetMS.tex2D, &shaderResourceViewDesc, &renderTargetMS.ressource ) ) ) {
		return 3;
	}

	mainRtDesc.SampleDesc.Count = 1;

	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	for ( int i = 0; i < 6; ++i ) {
		if ( FAILED( renderContext->device->CreateTexture2D( &mainRtDesc, nullptr, &renderTargets[i][0].tex2D ) ) ) {
			return 3;
		}

		if ( FAILED( renderContext->device->CreateRenderTargetView( renderTargets[i][0].tex2D, &renderTargetViewDesc, &renderTargets[i][0].view ) ) ) {
			return 3;
		}

		if ( FAILED( renderContext->device->CreateShaderResourceView( renderTargets[i][0].tex2D, &shaderResourceViewDesc, &renderTargets[i][0].ressource ) ) ) {
			return 3;
		}

		if ( FAILED( renderContext->device->CreateTexture2D( &mainRtDesc, nullptr, &renderTargets[i][1].tex2D ) ) ) {
			return 3;
		}

		if ( FAILED( renderContext->device->CreateRenderTargetView( renderTargets[i][1].tex2D, &renderTargetViewDesc, &renderTargets[i][1].view ) ) ) {
			return 3;
		}

		if ( FAILED( renderContext->device->CreateShaderResourceView( renderTargets[i][1].tex2D, &shaderResourceViewDesc, &renderTargets[i][1].ressource ) ) ) {
			return 3;
		}

		mainRtDesc.Width >>= 1;
		mainRtDesc.Height >>= 1;
	}

	finalRenderTarget = &renderTargets[0][1];
	finalRenderTargetResolve = &renderTargets[0][0];

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer = nullptr;

	D3DReadFileToBlob( L"base_data/shaders/postfx_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/downsample_ps.cso", &pixelShaderBuffer );

	if ( FAILED( renderContext->device->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader ) ) ) {
		return 1;
	}

	if ( FAILED( renderContext->device->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader ) ) ) {
		return 2;
	}

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	renderContext->device->CreateSamplerState( &samplerDesc, &linearSamplerState );

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	return 0;
}

void BloomPass::Render( ID3D11DeviceContext* devContext, GaussianBlur* gaussianBlurPass )
{
	ID3D11Resource* resDst = nullptr;
	finalRenderTargetResolve->view->GetResource( &resDst );

	ID3D11Resource* resSrc = nullptr;
	renderTargetMS.view->GetResource( &resSrc );

	devContext->ResolveSubresource( resDst, D3D11CalcSubresource( 0, 0, 1 ), resSrc, D3D11CalcSubresource( 0, 0, 1 ), DXGI_FORMAT_R16G16B16A16_FLOAT );

	D3D11_VIEWPORT backupViewport = {};
	UINT viewportCount = 1;
	devContext->RSGetViewports( &viewportCount, &backupViewport );

	unsigned int viewportW = baseWidth,
		viewportH = baseHeight;
	for ( int i = 0; i < 6; i++ ) {
		finalRenderTarget = gaussianBlurPass->Render( devContext, renderTargets[i] );

		if ( i < 5 ) {
			devContext->PSSetSamplers( 0, 1, &linearSamplerState );

			devContext->VSSetShader( vertexShader, NULL, 0 );
			devContext->PSSetShader( pixelShader, NULL, 0 );

			devContext->OMSetRenderTargets( 1, &renderTargets[i+1][0].view, NULL );
			Render_ClearTargetColor( devContext, &renderTargets[i + 1][0] );

			// set shadow viewport
			D3D11_VIEWPORT viewport =
			{
				0.0f,
				0.0f,
				static_cast<FLOAT>( viewportW ),
				static_cast<FLOAT>( viewportH ),
				0.0f,
				1.0f,
			};

			devContext->RSSetViewports( 1, &viewport );

			devContext->PSSetShaderResources( 0, 1, &finalRenderTarget->ressource );

			devContext->Draw( 6, 0 );
			
			ID3D11ShaderResourceView *const pSRV[1] = { NULL };
			devContext->PSSetShaderResources( 0, 1, pSRV );

			viewportW >>= 1;
			viewportH >>= 1;
		}
	}

	devContext->RSSetViewports( 1, &backupViewport );
}

const renderTarget_t* BloomPass::GetResolvedRenderTarget( const renderContext_t* renderContext )
{
	return finalRenderTarget;
}
