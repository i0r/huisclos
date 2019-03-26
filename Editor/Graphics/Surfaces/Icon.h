#pragma once

struct renderContext_t;
class FreeCamera;
class TextureManager;

#include <Engine/Graphics/Mesh.h>

struct edEntityIcon_t
{
	DirectX::XMFLOAT4 positionAndScale;
};

enum edIcons_t
{
    ED_ICON_ERROR = 0,
    ED_ICON_SPHERE_LIGHT = 1,
    ED_ICON_DISK_LIGHT = 2,
	ED_ICON_SUN_LIGHT = 3,

    ED_ICON_COUNT,
};

class SurfaceIcon
{
public:
								SurfaceIcon();
								SurfaceIcon( SurfaceIcon& ) = delete;
								~SurfaceIcon() = default;

	void						Destroy();
	const int					Create( const renderContext_t* context, TextureManager* texMan );
	void						Render( const renderContext_t* context, const edEntityIcon_t& iconPos, const edIcons_t iconId = ED_ICON_ERROR );

private:
	ID3D11VertexShader*			vertexShader;
	ID3D11PixelShader*			pixelShader;
	ID3D11SamplerState*			samplerState;
	ID3D11Buffer*				cbuffer;
	ID3D11BlendState*			blendState;

	texture_t*					icons[ED_ICON_COUNT];
};
