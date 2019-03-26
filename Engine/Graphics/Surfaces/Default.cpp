#include "Shared.h"
#include <d3d11.h>
#include "Default.h"
#include <d3dcompiler.h>

SurfaceDefault::SurfaceDefault()
	: vertexShader( nullptr )
	, pixelShader( nullptr )
	, shaderLayout( nullptr )
{

}

void SurfaceDefault::Destroy()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShader )
	RELEASE( shaderLayout )
}

const int SurfaceDefault::Create( ID3D11Device* dev )
{
	HRESULT result = 0;

	ID3D10Blob*		vertexShaderBuffer = nullptr;
	ID3D10Blob*		pixelShaderBuffer = nullptr;

	D3DReadFileToBlob( L"base_data/shaders/default_vs.cso", &vertexShaderBuffer );
	D3DReadFileToBlob( L"base_data/shaders/default_ps.cso", &pixelShaderBuffer );

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
	layoutDesc[1].AlignedByteOffset = 0;
	layoutDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[1].InstanceDataStepRate = 0;

	layoutDesc[2].SemanticName = "TEXCOORD";
	layoutDesc[2].SemanticIndex = 0;
	layoutDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	layoutDesc[2].InputSlot = 0;
	layoutDesc[2].AlignedByteOffset = 0;
	layoutDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[2].InstanceDataStepRate = 0;

	layoutDesc[3].SemanticName = "TANGENT";
	layoutDesc[3].SemanticIndex = 0;
	layoutDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[3].InputSlot = 0;
	layoutDesc[3].AlignedByteOffset = 0;
	layoutDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[3].InstanceDataStepRate = 0;

	layoutDesc[4].SemanticName = "BINORMAL";
	layoutDesc[4].SemanticIndex = 0;
	layoutDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	layoutDesc[4].InputSlot = 0;
	layoutDesc[4].AlignedByteOffset = 0;
	layoutDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	layoutDesc[4].InstanceDataStepRate = 0;

	result = dev->CreateInputLayout( layoutDesc, 5, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &shaderLayout );
	if ( FAILED( result ) ) {
		return 3;
	}
	
	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	return 0;
}

void SurfaceDefault::Render( ID3D11DeviceContext* devContext, const UINT indexCount )
{
	devContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	devContext->IASetInputLayout( shaderLayout );

	devContext->VSSetShader( vertexShader, NULL, 0 );
	devContext->PSSetShader( pixelShader, NULL, 0 );

	devContext->DrawIndexed( indexCount, 0, 0 );
}
