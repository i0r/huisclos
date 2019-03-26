#pragma once

#include <Engine/Graphics/Texture.h>

struct renderContext_t;

class GaussianBlur
{
public:
								GaussianBlur();
								GaussianBlur( GaussianBlur& )	= delete;
								~GaussianBlur()					= default;

	void						Destroy();
	int							Create( const renderContext_t* renderContext );
	const renderTarget_t*		Render( ID3D11DeviceContext* devContext, const renderTarget_t* targets );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShaders[2];
	ID3D11SamplerState*         samplerState;
};
