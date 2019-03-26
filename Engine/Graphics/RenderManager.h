#pragma once

struct window_t;

#include "RenderContext.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
#include "Texture.h"
#include "LightManager.h"

#include "Surfaces/Default.h"
#include "Surfaces/Opaque.h"

#include "World/Skybox.h"
#include "World/ShadowMapping.h"
#include "World/Atmosphere.h"

#include "PostFx/Composition.h"
#include "PostFx/Bloom.h"
#include "PostFx/GaussianBlur.h"

#include <Engine/ThirdParty/DirectXTK/Inc/SimpleMath.h>

struct worldArea_t;
struct areaNode_t;

class RenderManager
{
public:
	inline const renderContext_t*	GetContext() { return &renderContext; }
	inline TextureManager*			GetTextureManager() { return &texMan; }
	inline MaterialManager*			GetMaterialManager() { return &matMan; }

public:
					RenderManager()					= default;
					RenderManager( RenderManager& ) = delete;
					~RenderManager()				= default;

	void			Shutdown();
	const int		Initialize( const window_t* window );
	void			FrameWorld( const float interpolatedFt, Camera* activeCamera, const worldArea_t* activeArea );
	void			Swap() const;
	void			Resize( const unsigned short width, const unsigned short height );

	void			SwapEnvMap();

private:
	struct commonBuffer_t
	{
		float deltaTime;
		int __PADDING__[3];
	} commonBufferData;

private:
	bool			enableVsync;	// store cfg flags in a bitfield instead?

	renderContext_t	renderContext;

	// managers
	TextureManager	texMan;
	LightManager	lightMan;
	MaterialManager matMan;

	//TMP test
		texture_t*		iblLut;
		texture_t*		iblCubeDiff;
		texture_t*		iblCubeEnv;

		renderTarget_t	shadowMapTest;
	// END TMP

	CBuffer			commonBuffer;

	// Surfaces
	SurfaceDefault	defaultSurf;
	SurfaceOpaque	opaqueSurf;

	// Special passes
	Skybox			skybox;
	Atmosphere		atmosphere;
	ShadowMapping	shadowMapper;
	CompositionPass compositionPass;
	BloomPass		bloom;
	GaussianBlur	gaussianBlur;

	// Render Targets
	renderTarget_t	mainRenderTarget;
	renderTarget_t	mainRenderTargetResolved;

	bool			isNight;

private:
	void			RenderNode( Camera* activeCamera, const areaNode_t* node );
	void			UpdateCommonCBuffer();
};