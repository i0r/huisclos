#include "Shared.h"
#include "RenderContext.h"
#include "Texture.h"

#include <d3d11.h>
#include <Engine/ThirdParty/DirectXTK/Inc/DDSTextureLoader.h>

texture_t* TextureManager::GetTexture( const renderContext_t* context, const char* texPath )
{
	const uint64_t texHashcode = MurmurHash64A( texPath, static_cast<int>( strlen( texPath ) ), 0xB );

	auto it = content.find( texHashcode );

	if ( it != content.end() ) {
		return it->second.get();
	}

	content[texHashcode] = std::make_unique<texture_t>();

	static wchar_t widePath[MAX_PATH]{ '\0' }; // because Microsoft
	std::size_t wideConvertedLength = 0;

	mbstowcs_s( &wideConvertedLength, widePath, texPath, MAX_PATH );
	
	const HRESULT texLoadResult = DirectX::CreateDDSTextureFromFile( context->device, widePath, &content[texHashcode]->ressource, &content[texHashcode]->view );

	if ( texLoadResult != S_OK ) {
		content.erase( texHashcode );
		// TODO: log stuff
		return nullptr;
	}

	return content[texHashcode].get();
}

void Render_ClearTargetColor( ID3D11DeviceContext* context, const renderTarget_t* rt )
{
	static constexpr FLOAT CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	context->ClearRenderTargetView( rt->view, CLEAR_COLOR );
}
