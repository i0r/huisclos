#pragma once

struct renderContext_t;
struct ID3D11Buffer;

using CBuffer = ID3D11Buffer*;

const bool Render_CreateCBuffer( const renderContext_t* context, CBuffer& cbuffer, const unsigned int bufferSize );
const bool Render_UploadCBuffer( const renderContext_t* context, CBuffer cbuffer, const void* rawDataPtr,
								 const unsigned int bufferSize );
