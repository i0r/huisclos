#pragma once

#include <Engine/ThirdParty/DirectXTK/Inc/SimpleMath.h>

#include <memory>
#include <map>

struct texture_t;
struct renderContext_t;

enum surfType_t
{
	SURF_DEFAULT,		// default/error shader

	SURF_INVISIBLE,		// might be used for navpath, or stuff that does not need to be shaded
	SURF_OPAQUE,		// default physically based surface
	SURF_TRANSPARENT,	// glass and stuff
	SURF_UI,			// buttons, menus, debug helpers, ...

	SURF_COUNT
};

enum matFlag_t
{
	MAT_FLAG_IS_SHADELESS		= 1 << 0,
	MAT_FLAG_HAS_ALBEDO			= 1 << 1,
	MAT_FLAG_HAS_NORMALMAP		= 1 << 2,
	MAT_FLAG_HAS_AOMAP			= 1 << 3,
	MAT_FLAG_HAS_ROUGHNESSMAP	= 1 << 4,
	MAT_FLAG_HAS_METALNESSMAP	= 1 << 5,
	MAT_FLAG_HAS_ALPHAMAP		= 1 << 6,
};

using matFlags_t = unsigned int;

struct material_t
{
	surfType_t	surfType;			// 4

	texture_t*	albedo;				// 8
	texture_t*	normal;				// 8
	texture_t*	ambientOcclusion;	// 8
	texture_t*	metalness;			// 8
	texture_t*	roughness;			// 8
	texture_t*	alpha;				// 8

	ID3D11Buffer*	cbuffer;

	struct {
		DirectX::XMFLOAT3	diffuseColor;
		float				emissivityFactor;
		DirectX::XMFLOAT3	reflectivity;
		matFlags_t			flags;
	} colorData;
};

class MaterialManager
{
public:
	inline void	Flush() { content.clear(); }

public:
	MaterialManager() = default;
	MaterialManager( MaterialManager& ) = delete;
	~MaterialManager() = default;

	void		Initialize( const renderContext_t* context, TextureManager* texMan );
	material_t*	GetMaterial( const char* matPath );

private:
	std::map<uint64_t, std::unique_ptr<material_t>> content;

	const renderContext_t*  renderContext;
	TextureManager*			textureManager;
};

// static_assert( sizeof( material_t ) == 56, "material_t is NOT cache friendly" );

void	Render_BindOpaqueMaterial( ID3D11DeviceContext* devContext, const material_t* mat );
void	Render_BindOpaqueMaterial( ID3D11DeviceContext* devContext, const material_t* mesh );
int		Render_CreateMaterialFromFile( const renderContext_t* context, TextureManager* texMan, material_t* mat, const char* fileName );
void	Render_ReleaseMaterial( material_t* mat );
