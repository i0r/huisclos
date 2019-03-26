#include "Shared.h"
#include "CBuffer.h"
#include "RenderContext.h"

const bool Render_CreateCBuffer( const renderContext_t* context, CBuffer& cbuffer, const unsigned int bufferSize )
{
	// this only create a basic cbuffer
	// might be a stupid idea since it only wraps basic api calls...
	const D3D11_BUFFER_DESC cBufferDesc = {
		bufferSize,							// UINT ByteWidth
		D3D11_USAGE_DYNAMIC,				// D3D11_USAGE Usage
		D3D11_BIND_CONSTANT_BUFFER,			// UINT BindFlags
		D3D11_CPU_ACCESS_WRITE,				// UINT CPUAccessFlags
		0,									// UINT MiscFlags
		0,									// UINT StructureByteStride
	};

	return ( FAILED( context->device->CreateBuffer( &cBufferDesc, NULL, &cbuffer ) ) == S_OK );
}

const bool Render_UploadCBuffer( const renderContext_t* context, CBuffer cbuffer, const void* rawDataPtr, const unsigned int bufferSize )
{
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};

	if ( FAILED( context->deviceContext->Map( cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource ) ) ) {
		return false;
	}

	// TODO: should definitely consider a func to do partial cbuffer update (memcping the whole buffer is pointless)
	memcpy( mappedResource.pData, rawDataPtr, bufferSize );

	context->deviceContext->Unmap( cbuffer, 0 );

	return true;
}
