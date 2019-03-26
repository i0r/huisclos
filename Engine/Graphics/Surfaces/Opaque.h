#pragma once

struct mesh_t;
struct renderContext_t;
class FreeCamera;
class Camera;

class SurfaceOpaque
{
public:
								SurfaceOpaque();
								SurfaceOpaque( SurfaceOpaque& ) = delete;
								~SurfaceOpaque() = default;

	void						Destroy();
	const int					Create( const renderContext_t* context );
	void						Render( const renderContext_t* context, const mesh_t* mesh, const Camera* camera );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;
	ID3D11InputLayout*			shaderLayout;
	ID3D11SamplerState*			samplerState;
	ID3D11SamplerState*			shadowSamplerState;
	ID3D11Buffer*				cbuffer;
};
