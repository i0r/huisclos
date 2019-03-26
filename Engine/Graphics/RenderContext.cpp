#include "Shared.h"
#include <Engine/System/Window.h>
#include "RenderContext.h"

const int Sys_CreateRenderContext( renderContext_t* context, const window_t* window )
{
	UINT vsyncNumerator		= 0,
		 vsyncDenominator	= 0;

	{
		IDXGIFactory* factory = nullptr;
		CreateDXGIFactory( __uuidof( IDXGIFactory ), ( void** )&factory );

		IDXGIAdapter* adapter = nullptr;
		factory->EnumAdapters( 0, &adapter );

		IDXGIOutput* adapterOutput = nullptr;
		unsigned int numModes = 0;
		adapter->EnumOutputs( 0, &adapterOutput );
		adapterOutput->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL );

		DXGI_MODE_DESC* displayModeList = new DXGI_MODE_DESC[numModes]();
		adapterOutput->GetDisplayModeList( DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList );

		for ( unsigned int i = 0; i < numModes; ++i ) {
			if ( displayModeList[i].Width == window->width && displayModeList[i].Height == window->height ) {
				vsyncNumerator = displayModeList[i].RefreshRate.Numerator;
				vsyncDenominator = displayModeList[i].RefreshRate.Denominator;
				break;
			}
		}

		delete[] displayModeList;

		adapterOutput->Release();
		adapter->Release();
		factory->Release();
	}

	constexpr D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	const DXGI_SWAP_CHAIN_DESC swapChainDesc = {
		{																					// DXGI_MODE_DESC BufferDesc
			window->width,																	//		UINT Width
			window->height,																	//		UINT Height
			{																				//		DXGI_RATIONAL RefreshRate
				vsyncNumerator,																//			UINT Numerator			
				vsyncDenominator,															//			UINT Denominator
			},																				//
			DXGI_FORMAT_R8G8B8A8_UNORM,														//		DXGI_FORMAT Format
			DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,											//		DXGI_MODE_SCANLINE_ORDER ScanlineOrdering
			DXGI_MODE_SCALING_UNSPECIFIED,													//		DXGI_MODE_SCALING Scaling
		},																					//
		{																					// DXGI_SAMPLE_DESC SampleDesc
			1,																				//		UINT Count
			0,																				//		UINT Quality
		},																					//
		DXGI_USAGE_RENDER_TARGET_OUTPUT,													// DXGI_USAGE BufferUsage
		1,																					// UINT BufferCount
		window->handle,																		// HWND OutputWindow
		static_cast<BOOL>( window->flags & WIN_DISP_MODE_WINDOWED ),						// BOOL Windowed
		DXGI_SWAP_EFFECT_DISCARD,															// DXGI_SWAP_EFFECT SwapEffect
		0,																					// UINT Flags
	};

	UINT creationFlags = 0;

	if ( FAILED( D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, &featureLevel, 1,
												D3D11_SDK_VERSION, &swapChainDesc, &context->swapChain, &context->device,
												NULL, &context->deviceContext ) ) ) {
		return 1;
	}

	{
		ID3D11Texture2D* backBufferPtr = nullptr;

		context->swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBufferPtr );

		if ( FAILED( context->device->CreateRenderTargetView( backBufferPtr, NULL, &context->backBuffer ) ) ) {
			return 2;
		}

		backBufferPtr->Release();
	}


	constexpr D3D11_RASTERIZER_DESC rasterDesc = 
	{
		D3D11_FILL_SOLID,
		D3D11_CULL_BACK,
		0,          // BOOL FrontCounterClockwise;
		0,          // INT DepthBias;
		0.0f,       // FLOAT DepthBiasClamp;
		0.0f,       // FLOAT SlopeScaledDepthBias;
		TRUE,       // BOOL DepthClipEnable;
		0,          // BOOL ScissorEnable;
		0,          // BOOL MultisampleEnable;
		0,          // BOOL AntialiasedLineEnable;
	};

	if ( FAILED( context->device->CreateRasterizerState( &rasterDesc, &context->rasterState ) ) ) {
		return 3;
	}

	context->deviceContext->RSSetState( context->rasterState );

	Sys_ResizeRenderContext( context, window->width, window->height );

	return 0;
}

void Sys_DestroyRenderContext( renderContext_t* context )
{
	if ( context->swapChain != nullptr ) {
		context->swapChain->SetFullscreenState( false, nullptr );
	}

	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( context->rasterState )
	RELEASE( context->backBuffer )
	RELEASE( context->deviceContext )
	RELEASE( context->device )
	RELEASE( context->swapChain )

	RELEASE( context->depthStencilBuffer.buffer )
	RELEASE( context->depthStencilBuffer.stateAtmosphere )
	RELEASE( context->depthStencilBuffer.stateOpaque )
	RELEASE( context->depthStencilBuffer.view )
}

void Sys_ResizeRenderContext( renderContext_t* context, const unsigned short width, const unsigned short height )
{
	// resolve pointer indirection only once for the most used object of the struct
	ID3D11DeviceContext*	deviceContext	= context->deviceContext;
	ID3D11Device*			device			= context->device;

	deviceContext->OMSetRenderTargets( 0, 0, 0 );
	context->backBuffer->Release();
	context->swapChain->ResizeBuffers( 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0 );

	{
		ID3D11Texture2D* pBuffer = nullptr;

		context->swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( void** )&pBuffer );
		device->CreateRenderTargetView( pBuffer, NULL, &context->backBuffer );

		pBuffer->Release();
	}

	deviceContext->OMSetRenderTargets( 1, &context->backBuffer, NULL );

	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( context->depthStencilBuffer.buffer )
	RELEASE( context->depthStencilBuffer.stateAtmosphere )
	RELEASE( context->depthStencilBuffer.stateOpaque )
	RELEASE( context->depthStencilBuffer.view )

	D3D11_VIEWPORT viewport = 
	{
		0.0f,
		0.0f,
		static_cast<FLOAT>( width ),
		static_cast<FLOAT>( height ),
		0.0f,
		1.0f,
	};

	context->deviceContext->RSSetViewports( 1, &viewport );
	
	const D3D11_TEXTURE2D_DESC depthBufferDesc = {
		width,							// UINT Width
		height,							// UINT Height
		1,								// UINT MipLevels
		1,								// UINT ArraySize
		DXGI_FORMAT_R24G8_TYPELESS,	// DXGI_FORMAT Format
		{								// DXGI_SAMPLE_DESC SampleDesc
			4,							//		UINT Count
			0,							//		UINT Quality
		},								//
		D3D11_USAGE_DEFAULT,			// D3D11_USAGE Usage
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,		// UINT BindFlags
		0,								// UINT CPUAccessFlags
		0,								// UINT MiscFlags
	};

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {
		TRUE,							// BOOL DepthEnable
		D3D11_DEPTH_WRITE_MASK_ALL,		// D3D11_DEPTH_WRITE_MASK DepthWriteMask
		D3D11_COMPARISON_LESS_EQUAL,	// D3D11_COMPARISON_FUNC DepthFunc
		TRUE,							// BOOL StencilEnable
		0xFF,							// UINT8 StencilReadMask
		0xFF,							// UINT8 StencilWriteMask
		{								// D3D11_DEPTH_STENCILOP_DESC FrontFace
			D3D11_STENCIL_OP_KEEP,		//		D3D11_STENCIL_OP StencilFailOp
			D3D11_STENCIL_OP_INCR,		//		D3D11_STENCIL_OP StencilDepthFailOp
			D3D11_STENCIL_OP_KEEP,		//		D3D11_STENCIL_OP StencilPassOp
			D3D11_COMPARISON_ALWAYS,	//		D3D11_COMPARISON_FUNC StencilFunc
		},								//
		{								// D3D11_DEPTH_STENCILOP_DESC BackFace
			D3D11_STENCIL_OP_KEEP,		//		D3D11_STENCIL_OP StencilFailOp
			D3D11_STENCIL_OP_DECR,		//		D3D11_STENCIL_OP StencilDepthFailOp
			D3D11_STENCIL_OP_KEEP,		//		D3D11_STENCIL_OP StencilPassOp
			D3D11_COMPARISON_ALWAYS,	//		D3D11_COMPARISON_FUNC StencilFunc
		},								//
	};

	const D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {
		DXGI_FORMAT_D24_UNORM_S8_UINT,					// DXGI_FORMAT Format
		D3D11_DSV_DIMENSION_TEXTURE2DMS,			// D3D11_DSV_DIMENSION ViewDimension
		0,										// UINT Flags
	};

	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = 1;

	// some error check would be nice maybe? :)
	context->device->CreateTexture2D( &depthBufferDesc, NULL, &context->depthStencilBuffer.buffer );
	context->device->CreateDepthStencilState( &depthStencilDesc, &context->depthStencilBuffer.stateOpaque );

	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.DepthEnable = FALSE;

	context->device->CreateDepthStencilState( &depthStencilDesc, &context->depthStencilBuffer.stateAtmosphere );
	context->deviceContext->OMSetDepthStencilState( context->depthStencilBuffer.stateOpaque, 1 );
	context->device->CreateDepthStencilView( context->depthStencilBuffer.buffer, &depthStencilViewDesc, &context->depthStencilBuffer.view );

	context->deviceContext->OMSetRenderTargets( 1, &context->backBuffer, context->depthStencilBuffer.view );
}
