#pragma once

struct texture_t;
struct renderContext_t;
class TextureManager;

#include <Engine/Graphics/LightManager.h>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/CBuffer.h>

class ShadowMapping
{
public:
	ShadowMapping();
	ShadowMapping( ShadowMapping& )	= delete;
	~ShadowMapping()				= default;

	void						Destroy();
	const int					Create( const renderContext_t* context );

	void						RenderDiskAreaLight( const renderContext_t* context, const mesh_t* mesh, const diskAreaLight_t& light );
	void						RenderSun( const renderContext_t* context, sunLight_t& sun );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;
	ID3D11InputLayout*			shaderLayout;
	ID3D11SamplerState*			samplerState;

	struct
	{
		DirectX::XMMATRIX	lightMatrix;
		DirectX::XMMATRIX	modelMatrix;
	} matricesData;

	CBuffer						matricesCbuffer;
};
