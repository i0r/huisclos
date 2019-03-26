#pragma once

#include <Engine/Graphics/Texture.h>

struct texture_t;
struct renderContext_t;
class TextureManager;

class CompositionPass
{
public:
								CompositionPass();
								CompositionPass( CompositionPass& ) = delete;
								~CompositionPass()					= default;

	void						Destroy();
	int							Create( renderContext_t* context, TextureManager* texMan );
	void						Render( const renderContext_t* context, const renderTarget_t* mainRt, const renderTarget_t* bloomRt );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;

	ID3D11PixelShader*			pixelShaderAvgLuminances;
	ID3D11SamplerState*         samplerState;
	ID3D11SamplerState*         samplerStateMipMap;

	renderTarget_t				averageLuminance[2];
	int							activeTarget;
};
