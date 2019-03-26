#pragma once

#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

struct renderContext_t;

#include <d3d11.h>
#include <map>
#include <memory>

#include <Engine/System/MurmurHash2_64.h>

struct texture_t
{
	~texture_t()
	{
		RELEASE( ressource )
		RELEASE( view )
	}

	ID3D11Resource*				ressource;
	ID3D11ShaderResourceView*	view;
};

struct renderTarget_t
{
	~renderTarget_t()
	{
		RELEASE( tex1D )
		RELEASE( tex2D )
		RELEASE( tex3D )

		RELEASE( ressource )
		RELEASE( view )
	}

	union
	{
		ID3D11Texture1D*			tex1D;
		ID3D11Texture2D*			tex2D;
		ID3D11Texture3D*			tex3D;
	};

	ID3D11ShaderResourceView*		ressource;

	union
	{
		ID3D11RenderTargetView*     view;
		ID3D11DepthStencilView*		depthView;
	};
};

class TextureManager
{
public:
	inline void	Flush() { content.clear(); }

public:
				TextureManager()					= default;
				TextureManager( TextureManager& )	= delete;
				~TextureManager()					= default;

	texture_t*	GetTexture( const renderContext_t* context, const char* texPath );

private:
	std::map<uint64_t, std::unique_ptr<texture_t>> content;
};

void Render_ClearTargetColor( ID3D11DeviceContext* context, const renderTarget_t* rt );
