#pragma once

#include <Engine/Graphics/Texture.h>

struct renderContext_t;
class GaussianBlur;

class BloomPass
{
public:
	const renderTarget_t*		GetMSRenderTarget() { return &renderTargetMS; }

public:
								BloomPass();
								BloomPass( BloomPass& ) = delete;
								~BloomPass()			= default;

	void						Destroy();
	int							Create( const renderContext_t* renderContext, const int winWidth, const int winHeight );
	void						Render( ID3D11DeviceContext* devContext, GaussianBlur* gaussianBlurPass );

	const renderTarget_t*		GetResolvedRenderTarget( const renderContext_t* renderContext );

private:
	const renderTarget_t*		finalRenderTarget;
	renderTarget_t*				finalRenderTargetResolve;

	unsigned int				baseWidth;
	unsigned int				baseHeight;

	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;

	ID3D11SamplerState*         linearSamplerState;

	renderTarget_t				renderTargetMS;
	renderTarget_t				renderTargets[6][2]; // ping, pong (resolved)
	bool						pingPong;
};
