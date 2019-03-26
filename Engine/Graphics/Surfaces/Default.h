#pragma once

class SurfaceDefault
{
public:
								SurfaceDefault();
								SurfaceDefault( SurfaceDefault& )	= delete;
								~SurfaceDefault()					= default;

	void						Destroy();
	const int					Create( ID3D11Device* dev );
	void						Render( ID3D11DeviceContext* devContext, const UINT indexCount );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;
	ID3D11InputLayout*			shaderLayout;
};
