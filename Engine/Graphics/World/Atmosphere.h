#pragma once

#include "AtmosphereConstants.h"

#include <Engine/Graphics/LightManager.h>
#include <Engine/Graphics/Mesh.h>
#include <Engine/Graphics/CBuffer.h>
#include <Engine/Graphics/Texture.h>

struct atmospherePrecomputeCbuffer_t
{
    DirectX::XMVECTOR dhdH;
    float r;
    float k;
    int layer;
    float first;
};

class Atmosphere
{
public:
    Atmosphere();
    Atmosphere( Atmosphere& ) = delete;
    ~Atmosphere();

    int							Create( ID3D11Device* dev );
    void						Render( ID3D11DeviceContext* devContex );

    int                         Precompute( ID3D11Device* dev, ID3D11DeviceContext* devContex );

private:
    ID3D11VertexShader*			vertexShader;
    ID3D11PixelShader*			pixelShader;
    ID3D11InputLayout*			shaderLayout;
    ID3D11SamplerState*			samplerState;

    //=========================================================================

    // each 3d rt should be split in an array of 2d rt (to avoid gs usage)
    // this should probably be included properly into the rendertarget struct
    // !!TODO!!
    ID3D11RenderTargetView*     inscatterRtV[RES_R];

    renderTarget_t              irradianceTex;
    renderTarget_t              inscatterTex;
    renderTarget_t              transmittanceTex;


    renderTarget_t              deltaETex;

    ID3D11RenderTargetView*     deltaSRRtV[RES_R];
    renderTarget_t              deltaSRTex;

    ID3D11RenderTargetView*     deltaSMRtV[RES_R];
    renderTarget_t              deltaSMTex;

    ID3D11RenderTargetView*     deltaJRtV[RES_R];
    renderTarget_t              deltaJTex;

    DirectX::XMFLOAT4				dhdH;
    atmospherePrecomputeCbuffer_t   precomputeData;
    ID3D11Buffer*		            cbuffer;

private:
    int                         Prepare( ID3D11Device* dev );
    void                        UpdatePrecomputeCBuffer( ID3D11DeviceContext* devContext );
};
