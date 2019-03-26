#include "Shared.h"
#include <d3d11.h>
#include "GaussianBlur.h"
#include <d3dcompiler.h>

#include <Engine/Graphics/RenderContext.h>

GaussianBlur::GaussianBlur()
	: vertexShader( nullptr )
	, pixelShaders{ nullptr, nullptr }
	, samplerState( nullptr )
{

}

void GaussianBlur::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShaders[0] )
	RELEASE( pixelShaders[1] )
	RELEASE( samplerState )
}

int GaussianBlur::Create( const renderContext_t* renderContext )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer[2] = { nullptr, nullptr };

	D3DReadFileToBlob( L"base_data/shaders/postfx_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/gaussianblur_vertical_ps.cso", &pixelShaderBuffer[0] );
	D3DReadFileToBlob( L"base_data/shaders/gaussianblur_horizontal_ps.cso", &pixelShaderBuffer[1] );

	result = renderContext->device->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );
	if ( FAILED( result ) ) {
		return 1;
	}

	result = renderContext->device->CreatePixelShader( pixelShaderBuffer[0]->GetBufferPointer(), pixelShaderBuffer[0]->GetBufferSize(), NULL, &pixelShaders[0] );
	if ( FAILED( result ) ) {
		return 2;
	}

	result = renderContext->device->CreatePixelShader( pixelShaderBuffer[1]->GetBufferPointer(), pixelShaderBuffer[1]->GetBufferSize(), NULL, &pixelShaders[1] );
	if ( FAILED( result ) ) {
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

	renderContext->device->CreateSamplerState( &samplerDesc, &samplerState );

	vertexShaderBuffer->Release();
	pixelShaderBuffer[0]->Release();
	pixelShaderBuffer[1]->Release();

	return 0;
}

const renderTarget_t* GaussianBlur::Render( ID3D11DeviceContext* devContext, const renderTarget_t* targets )
{
	devContext->VSSetShader( vertexShader, NULL, 0 );

	devContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	devContext->PSSetSamplers( 0, 1, &samplerState );

	bool pingPong = false;
	int writeAttachement = 1, readAttachement = 0;

	for ( int i = 0; i < 2; ++i ) {
		devContext->PSSetShader( pixelShaders[writeAttachement], NULL, 0 );

		devContext->OMSetRenderTargets( 1, &targets[writeAttachement].view, NULL );
		Render_ClearTargetColor( devContext, &targets[writeAttachement] );

		devContext->PSSetShaderResources( 0, 1, &targets[readAttachement].ressource );

		devContext->Draw( 6, 0 );

		pingPong = !pingPong;

		writeAttachement = ( pingPong ) ? 0 : 1;
		readAttachement = ( writeAttachement == 0 ) ? 1 : 0;

		// unbind the ressource so that we can use the render target on the next frame
		ID3D11ShaderResourceView *const pSRV[1] = { NULL };
		devContext->PSSetShaderResources( 0, 1, pSRV );
	}

	return &targets[writeAttachement];
}
