#include "Shared.h"
#include <Engine/System/Window.h>
#include <Engine/Game/World.h>
#include "RenderContext.h"
#include "CBuffer.h"
#include "RenderManager.h"

#include <Engine/ThirdParty/DirectXTK/Inc/DDSTextureLoader.h>
#include <Engine/ThirdParty/DirectXTK/Inc/ScreenGrab.h>

void RenderManager::Shutdown()
{
	defaultSurf.Destroy();
	opaqueSurf.Destroy();

	skybox.Destroy();
	compositionPass.Destroy();

	texMan.Flush();
	matMan.Flush();

	// destroy the context at the end
	Sys_DestroyRenderContext( &renderContext );
}

const int RenderManager::Initialize( const window_t* window )
{
	const int contextCreationStatus = Sys_CreateRenderContext( &renderContext, window );

	if ( contextCreationStatus != 0 ) {
		return contextCreationStatus;
	}

	if ( !lightMan.Initialize( &renderContext ) ) {
		return 1;
	}

	matMan.Initialize( &renderContext, &texMan );

	// might use some bullshit 'manager' to store materials all together
	defaultSurf.Create( renderContext.device );
	opaqueSurf.Create( &renderContext );
	compositionPass.Create( &renderContext, &texMan );
	bloom.Create( &renderContext, window->width, window->height );
	shadowMapper.Create( &renderContext );
	gaussianBlur.Create( &renderContext );
	atmosphere.Create( renderContext.device );
	atmosphere.Precompute( renderContext.device, renderContext.deviceContext );

	Render_CreateCBuffer( &renderContext, commonBuffer, sizeof( commonBuffer_t ) );
	renderContext.deviceContext->PSSetConstantBuffers( 4, 1, &commonBuffer );

	// skybox, should be replaced with realistic atmospheric scaterring
	skybox.Create( &renderContext, &matMan, renderContext.device );

	// IBL Test with predefined environment
	iblLut = texMan.GetTexture( &renderContext, "base_data/textures/dev/brdf_lut.dds" );
	if ( iblLut == nullptr ) {
		return 2;
	}
	renderContext.deviceContext->PSSetShaderResources( 5, 1, &iblLut->view );
	
	SwapEnvMap();

	D3D11_RENDER_TARGET_VIEW_DESC   renderTargetViewDesc;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;

	D3D11_TEXTURE2D_DESC mainRtDesc = {
		window->width,
		window->height,
		1,
		1,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		{																					// DXGI_SAMPLE_DESC SampleDesc
			4,																				//		UINT Count
			0,																				//		UINT Quality
		},
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
	};

	renderTargetViewDesc.Format = mainRtDesc.Format;
	shaderResourceViewDesc.Format = mainRtDesc.Format;

	if ( FAILED( renderContext.device->CreateTexture2D( &mainRtDesc, nullptr, &mainRenderTarget.tex2D ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext.device->CreateRenderTargetView( mainRenderTarget.tex2D, &renderTargetViewDesc, &mainRenderTarget.view ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext.device->CreateShaderResourceView( mainRenderTarget.tex2D, &shaderResourceViewDesc, &mainRenderTarget.ressource ) ) ) {
		return 3;
	}
	
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	mainRtDesc.SampleDesc.Count = 1;

	if ( FAILED( renderContext.device->CreateTexture2D( &mainRtDesc, nullptr, &mainRenderTargetResolved.tex2D ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext.device->CreateRenderTargetView( mainRenderTargetResolved.tex2D, &renderTargetViewDesc, &mainRenderTargetResolved.view ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext.device->CreateShaderResourceView( mainRenderTargetResolved.tex2D, &shaderResourceViewDesc, &mainRenderTargetResolved.ressource ) ) ) {
		return 3;
	}
	
	const D3D11_TEXTURE2D_DESC shadowMapTestDesc = {
		2048,
		2048,
		1,
		1,
		DXGI_FORMAT_R24G8_TYPELESS,
		{																					// DXGI_SAMPLE_DESC SampleDesc
			1,																				//		UINT Count
			0,																				//		UINT Quality
		},
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
	};

	renderTargetViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; 

	if ( FAILED( renderContext.device->CreateTexture2D( &shadowMapTestDesc, nullptr, &shadowMapTest.tex2D ) ) ) {
		return 3;
	}

	if ( FAILED( renderContext.device->CreateShaderResourceView( shadowMapTest.tex2D, &shaderResourceViewDesc, &shadowMapTest.ressource ) ) ) {
		return 3;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	if ( FAILED( renderContext.device->CreateDepthStencilView( shadowMapTest.tex2D, &depthStencilViewDesc, &shadowMapTest.depthView ) ) ) {
		return 3;
	}

	return 0;
}

void RenderManager::RenderNode( Camera* activeCamera, const areaNode_t* node )
{
	for ( const areaNode_t* child : node->children ) {
		if ( child->flags & NODE_FLAG_CONTENT_MESH ) {
			mesh_t* mesh = static_cast< mesh_t* >( child->content );

			Render_BindMesh( &renderContext, mesh );
			opaqueSurf.Render( &renderContext, mesh, activeCamera );
		}

		if ( child->children.size() > 0 ) {
			RenderNode( activeCamera, child );
		}
	}
}

void RenderManager::FrameWorld( const float interpolatedFt, Camera* activeCamera, const worldArea_t* activeArea)
{
	// update Common cbuffer (dt, sys infos, ...)
	commonBufferData.deltaTime = interpolatedFt;
	UpdateCommonCBuffer();
	
	// update and prepare active camera for frame rendering
	activeCamera->UpdateMatrices( &renderContext );

	// update light list
	// WARNING: totally bloated; should be optimized ASAP!!!!!
	lightMan.Update( &renderContext, activeArea, isNight );

	//skybox.Render( &renderContext );

	// DRAW STUFF FROM RENDER LIST
	// => Sort the data!
	// => use a 64bits bitfield containing all the data required to setup the renderer properly
	// => tex to bind, raster state, ...

	ID3D11RenderTargetView* mainPassRv[2] = { mainRenderTarget.view, bloom.GetMSRenderTarget()->view };
	renderContext.deviceContext->OMSetRenderTargets( 2, mainPassRv, renderContext.depthStencilBuffer.view );
	Render_ClearTargetColor( renderContext.deviceContext, bloom.GetMSRenderTarget() );
	renderContext.deviceContext->ClearDepthStencilView( renderContext.depthStencilBuffer.view, D3D11_CLEAR_DEPTH, 1.0F, 0x0 );

	ID3D11ShaderResourceView *const pSRV[1] = { NULL };
	renderContext.deviceContext->PSSetShaderResources( 0, 1, pSRV );

	renderContext.deviceContext->PSSetShaderResources( 8, 1, &shadowMapTest.ressource );

	renderContext.deviceContext->OMSetDepthStencilState( renderContext.depthStencilBuffer.stateAtmosphere, 1 );
	atmosphere.Render( renderContext.deviceContext );
	renderContext.deviceContext->OMSetDepthStencilState( renderContext.depthStencilBuffer.stateOpaque, 1 );

	if ( activeArea->nodes != nullptr ) {
		RenderNode( activeCamera, activeArea->nodes );
	}

	// unbind the ressource so that we can use the render target on the next frame
	renderContext.deviceContext->PSSetShaderResources( 8, 1, pSRV );

	bloom.Render( renderContext.deviceContext, &gaussianBlur );

	ID3D11Resource* resSrc = nullptr;
	mainRenderTargetResolved.view->GetResource( &resSrc );

	ID3D11Resource* resDst = nullptr;
	mainRenderTarget.view->GetResource( &resDst );

	renderContext.deviceContext->ResolveSubresource( resSrc, D3D11CalcSubresource( 0, 0, 1 ), resDst, D3D11CalcSubresource( 0, 0, 1 ), DXGI_FORMAT_R16G16B16A16_FLOAT );
	compositionPass.Render( &renderContext, &mainRenderTargetResolved, bloom.GetResolvedRenderTarget( &renderContext ) );
}

// screenshot
//renderContext.deviceContext->OMSetRenderTargets( 0, 0, 0 );
//
//ID3D11Texture2D* backBufferTex = nullptr;
//HRESULT hr = renderContext.swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( void** )&backBufferTex );
//if ( SUCCEEDED( hr ) )
//{
//	hr = DirectX::SaveDDSTextureToFile( renderContext.deviceContext, backBufferTex, L"SCREENSHOT.DDS" );
//}

void RenderManager::Swap() const
{
	renderContext.swapChain->Present( ( enableVsync ? 1 : 0 ), 0 );
}

void RenderManager::Resize( const unsigned short width, const unsigned short height )
{
	Sys_ResizeRenderContext( &renderContext, width, height );

	D3D11_RENDER_TARGET_VIEW_DESC   renderTargetViewDesc;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;
	renderTargetViewDesc.Texture3D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = -1;
	shaderResourceViewDesc.Texture3D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture3D.MipLevels = -1;

	const D3D11_TEXTURE2D_DESC mainRtDesc = {
		width,
		height,
		1,
		1,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		{																					// DXGI_SAMPLE_DESC SampleDesc
			1,																				//		UINT Count
			0,																				//		UINT Quality
		},
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
	};

	renderTargetViewDesc.Format = mainRtDesc.Format;
	shaderResourceViewDesc.Format = mainRtDesc.Format;

	if ( FAILED( renderContext.device->CreateTexture2D( &mainRtDesc, nullptr, &mainRenderTarget.tex2D ) ) ) {
		return;
	}

	if ( FAILED( renderContext.device->CreateRenderTargetView( mainRenderTarget.tex2D, &renderTargetViewDesc, &mainRenderTarget.view ) ) ) {
		return;
	}

	if ( FAILED( renderContext.device->CreateShaderResourceView( mainRenderTarget.tex2D, &shaderResourceViewDesc, &mainRenderTarget.ressource ) ) ) {
		return;
	}

	// resize render targets and any additional stuff
}

void RenderManager::SwapEnvMap()
{
	skybox.LoadEnvMap( &renderContext, &texMan, &matMan, ( isNight ? "base_data/textures/dev/ibl_env_j.dds" : "base_data/textures/dev/ibl_env_n.dds" ) );

	iblCubeDiff = texMan.GetTexture( &renderContext, ( isNight ? "base_data/textures/dev/ibl_diffuse_j.dds" : "base_data/textures/dev/ibl_diffuse_n.dds" ) );
	if ( iblCubeDiff == nullptr ) {
		return;
	}

	renderContext.deviceContext->PSSetShaderResources( 6, 1, &iblCubeDiff->view );

	iblCubeEnv = texMan.GetTexture( &renderContext, ( isNight ? "base_data/textures/dev/ibl_spec_j.dds" : "base_data/textures/dev/ibl_spec_n.dds" ) );
	if ( iblCubeEnv == nullptr ) {
		return;
	}

	renderContext.deviceContext->PSSetShaderResources( 7, 1, &iblCubeEnv->view );

	isNight = !isNight;
}

void RenderManager::UpdateCommonCBuffer()
{
	Render_UploadCBuffer( &renderContext, commonBuffer, &commonBufferData, sizeof( commonBuffer_t ) );
	renderContext.deviceContext->PSSetConstantBuffers( 4, 1, &commonBuffer );
}
