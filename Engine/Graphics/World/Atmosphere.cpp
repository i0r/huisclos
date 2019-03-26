#include "Shared.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Atmosphere.h"

Atmosphere::Atmosphere()
    : vertexShader( nullptr )
    , pixelShader( nullptr )
    , shaderLayout( nullptr )
    , samplerState( nullptr )
{

}

Atmosphere::~Atmosphere()
{
	#define RELEASE( obj ) if ( obj != nullptr ) { obj->Release(); obj = nullptr; }

	RELEASE( vertexShader )
	RELEASE( pixelShader )
	RELEASE( shaderLayout )
	RELEASE( samplerState )
}

int Atmosphere::Create( ID3D11Device* dev )
{
    HRESULT result = 0;

    ID3D10Blob*		vertexShaderBuffer = nullptr;
    ID3D10Blob*		pixelShaderBuffer = nullptr;

    D3DReadFileToBlob( L"base_data/shaders/atmosphere_vs.cso", &vertexShaderBuffer );
    D3DReadFileToBlob( L"base_data/shaders/atmosphere_ps.cso", &pixelShaderBuffer );

    result = dev->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShader );
    if ( FAILED( result ) ) {
        return 1;
    }

    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShader );
    if ( FAILED( result ) ) {
        return 2;
    }

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = dev->CreateSamplerState( &samplerDesc, &samplerState );
    if ( FAILED( result ) ) {
        return 3;
    }

    return Prepare( dev );
}

void Atmosphere::Render( ID3D11DeviceContext* devContext )
{
    devContext->VSSetShader( vertexShader, NULL, 0 );
    devContext->PSSetShader( pixelShader, NULL, 0 );

    devContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    devContext->PSSetShaderResources( 0, 1, &inscatterTex.ressource );
    devContext->PSSetShaderResources( 1, 1, &irradianceTex.ressource );
    devContext->PSSetShaderResources( 2, 1, &transmittanceTex.ressource );

    devContext->PSSetSamplers( 0, 1, &samplerState );

    devContext->Draw( 6, 0 );

    devContext->PSSetShaderResources( 8, 1, &inscatterTex.ressource );
    devContext->PSSetShaderResources( 6, 1, &irradianceTex.ressource );
    devContext->PSSetShaderResources( 7, 1, &transmittanceTex.ressource );
}

int Atmosphere::Precompute( ID3D11Device* dev, ID3D11DeviceContext* devContext )
{
	D3D11_VIEWPORT backupViewport = {};
	UINT viewportCount = 1;
	devContext->RSGetViewports( &viewportCount, &backupViewport );

    ID3D11InputLayout*  shaderLayoutPostFx = nullptr;

    ID3D11VertexShader*	vertexShaderPostFx = nullptr;
    ID3D11PixelShader*	pixelShaderTransmittance = nullptr;

    ID3D10Blob*		    vertexShaderBuffer = nullptr;
    ID3D10Blob*		    pixelShaderBuffer = nullptr;

	ID3D11SamplerState* samplerStateFx = nullptr;

    ID3D11BlendState*           d3dBlendState;

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	dev->CreateSamplerState( &samplerDesc, &samplerStateFx );

    D3D11_BUFFER_DESC precomputeBuffer = {};

    precomputeBuffer.Usage = D3D11_USAGE_DYNAMIC;
    precomputeBuffer.ByteWidth = sizeof( atmospherePrecomputeCbuffer_t );
    precomputeBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    precomputeBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    precomputeBuffer.MiscFlags = 0;
    precomputeBuffer.StructureByteStride = 0;

    dev->CreateBuffer( &precomputeBuffer, NULL, &cbuffer );

    D3D11_VIEWPORT viewport = {
        0.0f,
        0.0f,
        static_cast<FLOAT>( TRANSMITTANCE_W ),
        static_cast<FLOAT>( TRANSMITTANCE_H ),
        0.0f,
        1.0f,
    };

    FLOAT color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/postfx_vs.cso", &vertexShaderBuffer );
    D3DReadFileToBlob( L"base_data/shaders/transmittance_ps.cso", &pixelShaderBuffer );

    HRESULT result = dev->CreateVertexShader( vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &vertexShaderPostFx );
    if ( FAILED( result ) ) {
        return 1;
    }

    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderTransmittance );
    if ( FAILED( result ) ) {
        return 2;
    }

    D3D11_INPUT_ELEMENT_DESC layoutDesc[1] = {};
    layoutDesc[0].SemanticName = "POSITION";
    layoutDesc[0].SemanticIndex = 0;
    layoutDesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
    layoutDesc[0].InputSlot = 0;
    layoutDesc[0].AlignedByteOffset = 0;
    layoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    layoutDesc[0].InstanceDataStepRate = 0;

    result = dev->CreateInputLayout( layoutDesc, 1, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &shaderLayoutPostFx );
    if ( FAILED( result ) ) {
        return 3;
    }

	pixelShaderBuffer->Release();

    devContext->RSSetViewports( 1, &viewport );
    devContext->OMSetRenderTargets( 1, &transmittanceTex.view, NULL );
    devContext->ClearRenderTargetView( transmittanceTex.view, color );

    devContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

    devContext->IASetInputLayout( shaderLayoutPostFx );

    devContext->VSSetShader( vertexShaderPostFx, NULL, 0 );
    devContext->PSSetShader( pixelShaderTransmittance, NULL, 0 );

    devContext->Draw( 6, 0 );

	

    //-----------------------------------------------------------------------------------------------------------------

	D3DReadFileToBlob( L"base_data/shaders/irradiance_ps.cso", &pixelShaderBuffer );

	ID3D11PixelShader* pixelShaderIrradiance = nullptr;
	result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderIrradiance );
	if ( FAILED( result ) ) {
		return 2;
	}

	pixelShaderBuffer->Release();
	
	viewport.Width = SKY_W;
	viewport.Height = SKY_H;

	devContext->RSSetViewports( 1, &viewport );
	devContext->OMSetRenderTargets( 1, &deltaETex.view, NULL );
	devContext->ClearRenderTargetView( deltaETex.view, color );

	devContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

	devContext->PSSetShader( pixelShaderIrradiance, NULL, 0 );
	devContext->PSSetSamplers( 0, 1, &samplerStateFx );
	devContext->PSSetShaderResources( 0, 1, &transmittanceTex.ressource );

	devContext->Draw( 6, 0 );

	

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/inscatter1_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderInscatter = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderInscatter );
    if ( FAILED( result ) ) {
        return 2;
    }

	pixelShaderBuffer->Release();

    viewport.Width = RES_MU_S * RES_NU;
    viewport.Height = RES_MU;

    devContext->RSSetViewports( 1, &viewport );
    devContext->PSSetShader( pixelShaderInscatter, NULL, 0 );
    devContext->PSSetSamplers( 0, 1, &samplerStateFx );
    devContext->PSSetShaderResources( 0, 1, &transmittanceTex.ressource );

    for ( int i = 0; i < RES_R; ++i ) {
        ID3D11RenderTargetView* rt[2] = { deltaSRRtV[i], deltaSMRtV[i] };

        devContext->OMSetRenderTargets( 2, rt, NULL );

        devContext->ClearRenderTargetView( deltaSRRtV[i], color );
        devContext->ClearRenderTargetView( deltaSMRtV[i], color );

        float r = i / ( RES_R - 1.0f );
        r = r * r;
        r = sqrtf( Rg * Rg + r * ( Rt * Rt - Rg * Rg ) ) + ( i == 0 ? 0.01 : ( i == RES_R - 1 ? -0.001 : 0.0 ) );
        double dmin = Rt - r;
        double dmax = sqrt( r * r - Rg * Rg ) + sqrt( Rt * Rt - Rg * Rg );
        double dminp = r - Rg;
        double dmaxp = sqrt( r * r - Rg * Rg );

        precomputeData.r = r;
        dhdH = DirectX::XMFLOAT4( float( dmin ), float( dmax ), float( dminp ), float( dmaxp ) );
        precomputeData.layer = i;

        UpdatePrecomputeCBuffer( devContext );

        devContext->Draw( 6, 0 );
        
    }

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/copyirradiance_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderCopyIrradiance = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderCopyIrradiance );
    if ( FAILED( result ) ) {
        return 2;
    }

	pixelShaderBuffer->Release();
	
    viewport.Width = SKY_W;
    viewport.Height = SKY_H;

    devContext->RSSetViewports( 1, &viewport );
    devContext->PSSetShader( pixelShaderCopyIrradiance, NULL, 0 );
    devContext->PSSetSamplers( 0, 1, &samplerStateFx );
    devContext->PSSetShaderResources( 0, 1, &deltaETex.ressource );

    devContext->OMSetRenderTargets( 1, &irradianceTex.view, NULL );
    devContext->ClearRenderTargetView( irradianceTex.view, color );

    precomputeData.k = 0.0f;
    UpdatePrecomputeCBuffer( devContext );

    devContext->Draw( 6, 0 );
    

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/copyinscatter_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderCopyInscatter = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderCopyInscatter );
    if ( FAILED( result ) ) {
        return 2;
    }

	pixelShaderBuffer->Release();

    viewport.Width = RES_MU_S * RES_NU;
    viewport.Height = RES_MU;

    devContext->RSSetViewports( 1, &viewport );
    devContext->PSSetShader( pixelShaderCopyInscatter, NULL, 0 );
    devContext->PSSetSamplers( 0, 1, &samplerStateFx );

    for ( int i = 0; i < RES_R; ++i ) {
        devContext->OMSetRenderTargets( 1, &inscatterRtV[i], NULL );
        devContext->ClearRenderTargetView( inscatterRtV[i], color );

        devContext->PSSetShaderResources( 0, 1, &deltaSRTex.ressource );
        devContext->PSSetShaderResources( 1, 1, &deltaSMTex.ressource );
        double r = i / ( RES_R - 1.0 );
        r = r * r;
        r = sqrt( Rg * Rg + r * ( Rt * Rt - Rg * Rg ) ) + ( i == 0 ? 0.01 : ( i == RES_R - 1 ? -0.001 : 0.0 ) );
        double dmin = Rt - r;
        double dmax = sqrt( r * r - Rg * Rg ) + sqrt( Rt * Rt - Rg * Rg );
        double dminp = r - Rg;
        double dmaxp = sqrt( r * r - Rg * Rg );

        precomputeData.r = r;
        dhdH = DirectX::XMFLOAT4( float( dmin ), float( dmax ), float( dminp ), float( dmaxp ) );
        precomputeData.layer = i;

        UpdatePrecomputeCBuffer( devContext );

        devContext->Draw( 6, 0 );
        
    }

    //-----------------------------------------------------------------------------------------------------------------
   
    D3DReadFileToBlob( L"base_data/shaders/inscatterS_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderCopyInscatterS = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderCopyInscatterS );
    if ( FAILED( result ) ) {
        return 2;
    }

	pixelShaderBuffer->Release();

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/irradianceN_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderirradianceN = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderirradianceN );
    if ( FAILED( result ) ) {
        return 2;
    }

    pixelShaderBuffer->Release();

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/inscatterN_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderinscatterN = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderinscatterN );
    if ( FAILED( result ) ) {
        return 2;
    }

    pixelShaderBuffer->Release();

    //-----------------------------------------------------------------------------------------------------------------

    D3DReadFileToBlob( L"base_data/shaders/copyinscatterN_ps.cso", &pixelShaderBuffer );

    ID3D11PixelShader* pixelShaderCopyInscatterN = nullptr;
    result = dev->CreatePixelShader( pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pixelShaderCopyInscatterN );
    if ( FAILED( result ) ) {
        return 2;
    }

    pixelShaderBuffer->Release();
        
    D3D11_BLEND_DESC omDesc;
    ZeroMemory( &omDesc, sizeof( D3D11_BLEND_DESC ) );
    omDesc.RenderTarget[0].BlendEnable = true;
    omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


    if ( FAILED( dev->CreateBlendState( &omDesc, &d3dBlendState ) ) ) return -2;

    // loop for each scattering order (line 6 in algorithm 4.1)
    for ( int order = 2; order <= 4; ++order ) {
        // computes deltaJ (line 7 in algorithm 4.1)
        //-----------------------------------------------------------------------------------------------------------------
   
        viewport.Width = RES_MU_S * RES_NU;
        viewport.Height = RES_MU;

        devContext->RSSetViewports( 1, &viewport );
        devContext->PSSetShader( pixelShaderCopyInscatterS, NULL, 0 );

        devContext->PSSetShaderResources( 0, 1, &deltaETex.ressource );
        devContext->PSSetShaderResources( 1, 1, &deltaSRTex.ressource );
        devContext->PSSetShaderResources( 2, 1, &deltaSMTex.ressource );
        devContext->PSSetShaderResources( 3, 1, &transmittanceTex.ressource );

		devContext->PSSetSamplers( 0, 1, &samplerStateFx );

        precomputeData.first = order == 2 ? 1.0f : 0.0f;   
        UpdatePrecomputeCBuffer( devContext );

        for ( int i = 0; i < RES_R; ++i ) {
            devContext->OMSetRenderTargets( 1, &deltaJRtV[i], NULL );

            double r = i / ( RES_R - 1.0 );
            r = r * r;
            r = sqrt( Rg * Rg + r * ( Rt * Rt - Rg * Rg ) ) + ( i == 0 ? 0.01 : ( i == RES_R - 1 ? -0.001 : 0.0 ) );
            double dmin = Rt - r;
            double dmax = sqrt( r * r - Rg * Rg ) + sqrt( Rt * Rt - Rg * Rg );
            double dminp = r - Rg;
            double dmaxp = sqrt( r * r - Rg * Rg );

            precomputeData.r = r;
            dhdH = DirectX::XMFLOAT4( float( dmin ), float( dmax ), float( dminp ), float( dmaxp ) );
            precomputeData.layer = i;

            UpdatePrecomputeCBuffer( devContext );

            devContext->Draw( 6, 0 );
            
        }

        // computes deltaE (line 8 in algorithm 4.1)
        //-----------------------------------------------------------------------------------------------------------------
        viewport.Width = SKY_W;
        viewport.Height = SKY_H;

        devContext->RSSetViewports( 1, &viewport );
        devContext->PSSetShader( pixelShaderirradianceN, NULL, 0 );

		devContext->PSSetSamplers( 0, 1, &samplerStateFx );
        devContext->PSSetShaderResources( 0, 1, &deltaSRTex.ressource );
        devContext->PSSetShaderResources( 1, 1, &deltaSMTex.ressource );
        devContext->PSSetShaderResources( 2, 1, &transmittanceTex.ressource );

        devContext->OMSetRenderTargets( 1, &deltaETex.view, NULL );

        devContext->Draw( 6, 0 );
        

        // computes deltaS (line 9 in algorithm 4.1)
        //-----------------------------------------------------------------------------------------------------------------
        viewport.Width = RES_MU_S * RES_NU;
        viewport.Height = RES_MU;

        devContext->RSSetViewports( 1, &viewport );
        devContext->PSSetShader( pixelShaderinscatterN, NULL, 0 );

		devContext->PSSetSamplers( 0, 1, &samplerStateFx );
        devContext->PSSetShaderResources( 0, 1, &transmittanceTex.ressource );
        devContext->PSSetShaderResources( 1, 1, &deltaJTex.ressource );

        for ( int i = 0; i < RES_R; ++i ) {
            devContext->OMSetRenderTargets( 1, &deltaSRRtV[i], NULL );

            double r = i / ( RES_R - 1.0 );
            r = r * r;
            r = sqrt( Rg * Rg + r * ( Rt * Rt - Rg * Rg ) ) + ( i == 0 ? 0.01 : ( i == RES_R - 1 ? -0.001 : 0.0 ) );
            double dmin = Rt - r;
            double dmax = sqrt( r * r - Rg * Rg ) + sqrt( Rt * Rt - Rg * Rg );
            double dminp = r - Rg;
            double dmaxp = sqrt( r * r - Rg * Rg );

            precomputeData.r = r;
            dhdH = DirectX::XMFLOAT4( float( dmin ), float( dmax ), float( dminp ), float( dmaxp ) );
            precomputeData.layer = i;

            UpdatePrecomputeCBuffer( devContext );

            devContext->Draw( 6, 0 );
            
        }

        devContext->OMSetBlendState( d3dBlendState, 0, 0xffffffff );

        // adds deltaE into irradiance texture E (line 10 in algorithm 4.1)
        //-----------------------------------------------------------------------------------------------------------------
        viewport.Width = SKY_W;
        viewport.Height = SKY_H;

        devContext->RSSetViewports( 1, &viewport );
        devContext->PSSetShader( pixelShaderCopyIrradiance, NULL, 0 );

		devContext->PSSetSamplers( 0, 1, &samplerStateFx );
        devContext->PSSetShaderResources( 0, 1, &deltaETex.ressource );
        devContext->OMSetRenderTargets( 1, &irradianceTex.view, NULL );

        precomputeData.k = 1.0f;

        UpdatePrecomputeCBuffer( devContext );

        devContext->Draw( 6, 0 );
        

        // adds deltaS into inscatter texture S (line 11 in algorithm 4.1)
        //-----------------------------------------------------------------------------------------------------------------
        viewport.Width = RES_MU_S * RES_NU;
        viewport.Height = RES_MU;

        devContext->RSSetViewports( 1, &viewport );
        devContext->PSSetShader( pixelShaderCopyInscatterN, NULL, 0 );

		devContext->PSSetSamplers( 0, 1, &samplerStateFx );
        devContext->PSSetShaderResources( 0, 1, &deltaSRTex.ressource );

        for ( int i = 0; i < RES_R; ++i ) {
            devContext->OMSetRenderTargets( 1, &inscatterRtV[i], NULL );

            double r = i / ( RES_R - 1.0 );
            r = r * r;
            r = sqrt( Rg * Rg + r * ( Rt * Rt - Rg * Rg ) ) + ( i == 0 ? 0.01 : ( i == RES_R - 1 ? -0.001 : 0.0 ) );
            double dmin = Rt - r;
            double dmax = sqrt( r * r - Rg * Rg ) + sqrt( Rt * Rt - Rg * Rg );
            double dminp = r - Rg;
            double dmaxp = sqrt( r * r - Rg * Rg );

            precomputeData.r = r;
            dhdH = DirectX::XMFLOAT4( float( dmin ), float( dmax ), float( dminp ), float( dmaxp ) );
            precomputeData.layer = i;

            UpdatePrecomputeCBuffer( devContext );

            devContext->Draw( 6, 0 );
            
        }

        devContext->OMSetBlendState( nullptr, 0, 0xffffffff );
    }

    //-----------------------------------------------------------------------------------------------------------------

	pixelShaderCopyIrradiance->Release();
	pixelShaderInscatter->Release();
    pixelShaderTransmittance->Release();
    pixelShaderIrradiance->Release();
	vertexShaderBuffer->Release();
    shaderLayoutPostFx->Release();

	devContext->RSSetViewports( 1, &backupViewport );
	
	return 0;
}

int Atmosphere::Prepare( ID3D11Device* dev )
{
    D3D11_RENDER_TARGET_VIEW_DESC   renderTargetViewDesc;
    renderTargetViewDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;
    renderTargetViewDesc.Texture3D.MipSlice = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.ViewDimension                = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip    = 0;
    shaderResourceViewDesc.Texture2D.MipLevels          = -1;
    shaderResourceViewDesc.Texture3D.MostDetailedMip    = 0;
    shaderResourceViewDesc.Texture3D.MipLevels          = -1;

    // precompute ressources required by Bruneton's skymodel
    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE2D_DESC transmittanceDesc = {
        TRANSMITTANCE_W,
        TRANSMITTANCE_H,
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

    renderTargetViewDesc.Format = transmittanceDesc.Format;
    shaderResourceViewDesc.Format = transmittanceDesc.Format;

    HRESULT result = dev->CreateTexture2D( &transmittanceDesc, nullptr, &transmittanceTex.tex2D );
    if ( FAILED( result ) ) {
        return 1;
    }

    if ( FAILED( dev->CreateRenderTargetView( transmittanceTex.tex2D, &renderTargetViewDesc, &transmittanceTex.view ) ) ) {
        return 2;
    }

    if ( FAILED( dev->CreateShaderResourceView( transmittanceTex.tex2D, &shaderResourceViewDesc, &transmittanceTex.ressource ) ) ) {
        return 3;
    }

    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE2D_DESC irradianceDesc = {
        SKY_W,
        SKY_H,
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

    renderTargetViewDesc.Format = irradianceDesc.Format;
    shaderResourceViewDesc.Format = irradianceDesc.Format;

    if ( FAILED( dev->CreateTexture2D( &irradianceDesc, nullptr, &irradianceTex.tex2D ) ) ) {
        return 4;
    }

    if ( FAILED( dev->CreateRenderTargetView( irradianceTex.tex2D, &renderTargetViewDesc, &irradianceTex.view ) ) ) {
        return 5;
    }

    if ( FAILED( dev->CreateShaderResourceView( irradianceTex.tex2D, &shaderResourceViewDesc, &irradianceTex.ressource ) ) ) {
        return 6;
    }

    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE3D_DESC inscatterDesc = {
        RES_MU_S * RES_NU,
        RES_MU,
        RES_R,
        1,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
    };

    shaderResourceViewDesc.Format = inscatterDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

    if ( FAILED( dev->CreateTexture3D( &inscatterDesc, nullptr, &inscatterTex.tex3D ) ) ) {
        return 7;
    }

    if ( FAILED( dev->CreateShaderResourceView( inscatterTex.tex3D, &shaderResourceViewDesc, &inscatterTex.ressource ) ) ) {
        return 9;
    }

    D3D11_RENDER_TARGET_VIEW_DESC inscatterRt[RES_R] = {};
    for ( int i = 0; i < RES_R; ++i ) {
        inscatterRt[i].ViewDimension            = D3D11_RTV_DIMENSION_TEXTURE3D;
        inscatterRt[i].Format                   = inscatterDesc.Format;
        inscatterRt[i].Texture3D.MipSlice       = 0;
        inscatterRt[i].Texture3D.FirstWSlice    = i;
        inscatterRt[i].Texture3D.WSize          = 1;

        result = dev->CreateRenderTargetView( inscatterTex.tex3D, &inscatterRt[i], &inscatterRtV[i] );
        if ( FAILED( result ) ) {
            return 8;
        }
    }

    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE2D_DESC deltaEDesc = {
        SKY_W,
        SKY_H,
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

    renderTargetViewDesc.Format = irradianceDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    shaderResourceViewDesc.Format = irradianceDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

    if ( FAILED( dev->CreateTexture2D( &deltaEDesc, nullptr, &deltaETex.tex2D ) ) ) {
        return 10;
    }

    if ( FAILED( dev->CreateRenderTargetView( deltaETex.tex2D, &renderTargetViewDesc, &deltaETex.view ) ) ) {
        return 11;
    }

    if ( FAILED( dev->CreateShaderResourceView( deltaETex.tex2D, &shaderResourceViewDesc, &deltaETex.ressource ) ) ) {
        return 12;
    }

    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE3D_DESC deltaSRDesc = {
        RES_MU_S * RES_NU, 
        RES_MU, 
        RES_R,
        1,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
    };

    renderTargetViewDesc.Format = deltaSRDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;

    shaderResourceViewDesc.Format = deltaSRDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

    if ( FAILED( dev->CreateTexture3D( &deltaSRDesc, nullptr, &deltaSRTex.tex3D ) ) ) {
        return 13;
    }

    D3D11_RENDER_TARGET_VIEW_DESC deltaSRRtDesc[RES_R] = {};
    for ( int i = 0; i < RES_R; ++i ) {
        deltaSRRtDesc[i].ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        deltaSRRtDesc[i].Format = deltaSRDesc.Format;
        deltaSRRtDesc[i].Texture3D.MipSlice = 0;
        deltaSRRtDesc[i].Texture3D.FirstWSlice = i;
        deltaSRRtDesc[i].Texture3D.WSize = 1;

        result = dev->CreateRenderTargetView( deltaSRTex.tex3D, &deltaSRRtDesc[i], &deltaSRRtV[i] );
        if ( FAILED( result ) ) {
            return 8;
        }
    }

    if ( FAILED( dev->CreateShaderResourceView( deltaSRTex.tex3D, &shaderResourceViewDesc, &deltaSRTex.ressource ) ) ) {
        return 15;
    }

    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE3D_DESC deltaSMDesc = {
        RES_MU_S * RES_NU, 
        RES_MU, 
        RES_R,
        1,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
    };

    renderTargetViewDesc.Format = deltaSMDesc.Format;
    shaderResourceViewDesc.Format = deltaSMDesc.Format;

    if ( FAILED( dev->CreateTexture3D( &deltaSMDesc, nullptr, &deltaSMTex.tex3D ) ) ) {
        return 13;
    }

    D3D11_RENDER_TARGET_VIEW_DESC deltaSMRtDesc[RES_R] = {};
    for ( int i = 0; i < RES_R; ++i ) {
        deltaSMRtDesc[i].ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        deltaSMRtDesc[i].Format = deltaSMDesc.Format;
        deltaSMRtDesc[i].Texture3D.MipSlice = 0;
        deltaSMRtDesc[i].Texture3D.FirstWSlice = i;
        deltaSMRtDesc[i].Texture3D.WSize = 1;

        result = dev->CreateRenderTargetView( deltaSMTex.tex3D, &deltaSMRtDesc[i], &deltaSMRtV[i] );
        if ( FAILED( result ) ) {
            return 8;
        }
    }

    if ( FAILED( dev->CreateShaderResourceView( deltaSMTex.tex3D, &shaderResourceViewDesc, &deltaSMTex.ressource ) ) ) {
        return 15;
    }

    //-----------------------------------------------------------------------------------------------------------------

    const D3D11_TEXTURE3D_DESC deltaJDesc = {
        RES_MU_S * RES_NU, 
        RES_MU, 
        RES_R,
        1,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
    };
    
    renderTargetViewDesc.Format = deltaJDesc.Format;
    shaderResourceViewDesc.Format = deltaJDesc.Format;

    if ( FAILED( dev->CreateTexture3D( &deltaJDesc, nullptr, &deltaJTex.tex3D ) ) ) {
        return 16;
    }

    D3D11_RENDER_TARGET_VIEW_DESC deltaJRtDesc[RES_R] = {};
    for ( int i = 0; i < RES_R; ++i ) {
        deltaJRtDesc[i].ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        deltaJRtDesc[i].Format = deltaJDesc.Format;
        deltaJRtDesc[i].Texture3D.MipSlice = 0;
        deltaJRtDesc[i].Texture3D.FirstWSlice = i;
        deltaJRtDesc[i].Texture3D.WSize = 1;

        result = dev->CreateRenderTargetView( deltaJTex.tex3D, &deltaJRtDesc[i], &deltaJRtV[i] );
        if ( FAILED( result ) ) {
            return 8;
        }
    }


    if ( FAILED( dev->CreateShaderResourceView( deltaJTex.tex3D, &shaderResourceViewDesc, &deltaJTex.ressource ) ) ) {
        return 18;
    }

    //-----------------------------------------------------------------------------------------------------------------

	#ifdef _DEBUG
		transmittanceTex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "Transmittance" ), "Transmittance" );
		irradianceTex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "irradiance" ), "irradiance" );
		inscatterTex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "inscatter" ), "inscatter" );
		deltaETex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "deltaE" ), "deltaE" );
		deltaSRTex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "deltaSR" ), "deltaSR" );
		deltaSMTex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "deltaSM" ), "deltaSM" );
		deltaJTex.tex2D->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( "deltaJ" ), "deltaJ" );
	#endif

    return 0;
}

void Atmosphere::UpdatePrecomputeCBuffer( ID3D11DeviceContext* devContext )
{
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    atmospherePrecomputeCbuffer_t* dataPtr = nullptr;

    HRESULT result = devContext->Map( cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
    if ( FAILED( result ) ) {
        return;
    }

    dataPtr = ( atmospherePrecomputeCbuffer_t* )mappedResource.pData;

    dataPtr->r = precomputeData.r;
    dataPtr->k = precomputeData.k;
    dataPtr->first = precomputeData.first;
    dataPtr->layer = precomputeData.layer;
    dataPtr->dhdH = DirectX::XMLoadFloat4( &dhdH );

    devContext->PSSetConstantBuffers( 0, 1, &cbuffer );

    devContext->Unmap( cbuffer, 0 );
}