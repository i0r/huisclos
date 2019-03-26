#pragma once

struct texture_t;
struct renderContext_t;
class MaterialManager;

#include <Engine/Graphics/Mesh.h>

class Skybox
{
public:
	Skybox();
	Skybox( Skybox& )	= delete;
	~Skybox()			= default;

	void						Destroy();
	const int					Create( const renderContext_t* context, MaterialManager* matMan, ID3D11Device* dev );
	const bool					LoadEnvMap( const renderContext_t* context, TextureManager* texMan, MaterialManager* matMan, const char* resFilePath );
	void						Render( const renderContext_t* context );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;
	ID3D11InputLayout*			shaderLayout;
	ID3D11SamplerState*			samplerState;

	texture_t*					envMap;
	mesh_t						skyboxGeometry;
};
