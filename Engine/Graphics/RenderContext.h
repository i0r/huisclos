#pragma once

#include <d3d11.h>

struct window_t;

struct renderContext_t
{
	ID3D11DeviceContext*		deviceContext;
	ID3D11Device*				device;
	ID3D11RenderTargetView*		backBuffer;
	IDXGISwapChain*				swapChain;
	ID3D11RasterizerState*		rasterState;

	struct {
		ID3D11Texture2D*			buffer;
		ID3D11DepthStencilState*	stateAtmosphere;
		ID3D11DepthStencilState*	stateOpaque;
		ID3D11DepthStencilView*		view;
	} depthStencilBuffer;
};

const int	Sys_CreateRenderContext( renderContext_t* context, const window_t* window );
void		Sys_DestroyRenderContext( renderContext_t* context );
void		Sys_ResizeRenderContext( renderContext_t* context, const unsigned short width, const unsigned short height );
