#include "Shared.h"
#include <d3d11.h>
#include "Texture.h"
#include "Material.h"
#include "RenderContext.h"
#include "CBuffer.h"

#include <Engine/ThirdParty/DirectXTK/Inc/SimpleMath.h>
#include <Engine/ThirdParty/DirectXTK/Inc/DDSTextureLoader.h>
#include <Engine/Io/DictionaryReader.h>

#include <cctype>
#include <string>
#include <algorithm>

void MaterialManager::Initialize( const renderContext_t* context, TextureManager* texMan )
{
	renderContext = context;
	textureManager = texMan;
}

material_t* MaterialManager::GetMaterial( const char* matPath )
{
	const uint64_t matHashcode = MurmurHash64A( matPath, static_cast<int>( strlen( matPath ) ), 0xB );

	auto it = content.find( matHashcode );

	if ( it != content.end() ) {
		return it->second.get();
	}

	content[matHashcode] = std::make_unique<material_t>();

	const int matCreation = Render_CreateMaterialFromFile( renderContext, textureManager, content[matHashcode].get(), matPath );

	if ( matCreation != 0 ) {
		content.erase( matHashcode );
		// TODO: log stuff
		return nullptr;
	}

	return content[matHashcode].get();
}

void Render_BindUIMaterial( ID3D11DeviceContext* devContext, const material_t* mat )
{
	devContext->PSSetShaderResources( 0, 1, &mat->albedo->view );
	devContext->PSSetConstantBuffers( 1, 1, &mat->cbuffer );
}

void Render_BindOpaqueMaterial( ID3D11DeviceContext* devContext, const material_t* mat )
{
	if ( mat->albedo != nullptr ) devContext->PSSetShaderResources( 0, 1, &mat->albedo->view );
	if ( mat->normal != nullptr ) devContext->PSSetShaderResources( 1, 1, &mat->normal->view );
	if ( mat->ambientOcclusion != nullptr ) devContext->PSSetShaderResources( 2, 1, &mat->ambientOcclusion->view );
	if ( mat->metalness != nullptr ) devContext->PSSetShaderResources( 3, 1, &mat->metalness->view );
	if ( mat->roughness != nullptr ) devContext->PSSetShaderResources( 4, 1, &mat->roughness->view );
	if ( mat->alpha != nullptr ) devContext->PSSetShaderResources( 9, 1, &mat->alpha->view );

	devContext->PSSetConstantBuffers( 1, 1, &mat->cbuffer );
}

void Render_BindAlphaTestedMaterial( ID3D11DeviceContext* devContext, const material_t* mat )
{
	Render_BindOpaqueMaterial( devContext, mat );

	if ( mat->alpha != nullptr ) devContext->PSSetShaderResources( 5, 1, &mat->alpha->view );
}

namespace
{
	DirectX::XMFLOAT3 atov( const std::string& str )
	{
		if ( str.front() != '{' || str.back() != '}' ) {
			return DirectX::XMFLOAT3();
		}

		DirectX::XMFLOAT3 vec = {};
		
		std::size_t offsetX = str.find_first_of( ',' ),
					offsetY = str.find_first_of( ',', offsetX ),
					offsetZ = str.find_last_of( ',' ),

					vecEnd = str.find_last_of( '}' );

		std::string vecX = str.substr( 1, offsetX - 1 );
		std::string vecY = str.substr( offsetX + 1, offsetY - 1 );
		std::string vecZ = str.substr( offsetZ + 1, vecEnd - offsetZ - 2 );

		vec.x = static_cast<float>( atof( vecX.c_str() ) );
		vec.y = static_cast<float>( atof( vecY.c_str() ) );
		vec.z = static_cast<float>( atof( vecZ.c_str() ) );
		
		return vec;
	}

	// convert a litteral array of flags to a material bitfield
	unsigned int atombf( const std::string& str )
	{
		if ( str.front() != '[' || str.back() != ']' ) {
			return 0;
		}

		unsigned int bitfield = 0x0;

		std::string strCpy( str );
		strCpy.erase( std::remove_if( strCpy.begin(), strCpy.end(), isspace ) );

		std::size_t nextFlagOffset	= strCpy.find_first_of( ',' );
		std::size_t currentOffset	= 1;

		while ( nextFlagOffset != std::string::npos ) {
			std::string flag = strCpy.substr( currentOffset, ( nextFlagOffset - currentOffset ) );

			if ( flag == "shadeless" ) {
				bitfield |= MAT_FLAG_IS_SHADELESS;
			} else if ( flag == "has_albedo" ) {
				bitfield |= MAT_FLAG_HAS_ALBEDO;
			} else if ( flag == "has_normal" ) {
				bitfield |= MAT_FLAG_HAS_NORMALMAP;
			} else if ( flag == "has_ao" ) {
				bitfield |= MAT_FLAG_HAS_AOMAP;
			} else if ( flag == "has_metalness" ) {
				bitfield |= MAT_FLAG_HAS_METALNESSMAP;
			} else if ( flag == "has_roughness" ) {
				bitfield |= MAT_FLAG_HAS_ROUGHNESSMAP;
			} else if ( flag == "has_alpha" ) {
				bitfield |= MAT_FLAG_HAS_ALPHAMAP;
			}

			currentOffset = nextFlagOffset + 1;
			nextFlagOffset = strCpy.find_first_of( ',', currentOffset );
		}

		return bitfield;
	}
}

int Render_CreateMaterialFromFile( const renderContext_t* context, TextureManager* texMan, material_t* mat, const char* fileName )
{
	dictionary_t dico = {};

	if ( Io_ReadDictionaryFile( fileName, dico ) != 0 ) {
		return 1;
	}

	using dicoPair_t = std::pair<std::string, std::string>;

	for ( const dicoPair_t& pair : dico ) {
		// TODO: hash the dico key!
		// string as key is pure evil!
		if ( pair.first == "name" ) {
			
		} else if ( pair.first == "emissivity" ) {
			mat->colorData.emissivityFactor = static_cast<float>( atof( pair.second.c_str() ) );
		} else if ( pair.first == "type" ) {
			if ( pair.second == "opaque" ) {
				mat->surfType = SURF_OPAQUE;
			} else if ( pair.second == "transparent" ) {
				mat->surfType = SURF_TRANSPARENT;
			} else if ( pair.second == "invisible" ) {
				mat->surfType = SURF_INVISIBLE;
			} else if ( pair.second == "ui" ) {
				mat->surfType = SURF_UI;
			} else {
				mat->surfType = SURF_DEFAULT;
			}
		} else if ( pair.first == "flags" ) {
			mat->colorData.flags = atombf( pair.second );
		} else if ( pair.first == "diffuse" ) {
			mat->colorData.diffuseColor = atov( pair.second );
		} else if ( pair.first == "albedo" ) {
			mat->albedo = texMan->GetTexture( context, pair.second.c_str() );
		} else if ( pair.first == "normal" ) {
			mat->normal = texMan->GetTexture( context, pair.second.c_str() );
		} else if ( pair.first == "ao" ) {
			mat->ambientOcclusion = texMan->GetTexture( context, pair.second.c_str() );
		} else if ( pair.first == "metalness" ) {
			mat->metalness = texMan->GetTexture( context, pair.second.c_str() );
		} else if ( pair.first == "roughness" ) {
			mat->roughness = texMan->GetTexture( context, pair.second.c_str() );
		} else if ( pair.first == "alpha" ) {
			mat->alpha = texMan->GetTexture( context, pair.second.c_str() );
		} else if ( pair.first == "reflectivity" ) {
			mat->colorData.reflectivity = atov( pair.second );
		}		
	}

	Render_CreateCBuffer( context, mat->cbuffer, sizeof( mat->colorData ) );
	Render_UploadCBuffer( context, mat->cbuffer, &mat->colorData, sizeof( mat->colorData ) );

	return 0;
}

void Render_ReleaseMaterial( material_t* mat )
{
	#define MAT_RELEASE( obj ) if ( obj != nullptr ) { delete obj; obj = nullptr; }

	//MAT_RELEASE( mat->albedo )
	//MAT_RELEASE( mat->normal )
	//MAT_RELEASE( mat->ambientOcclusion )
	//MAT_RELEASE( mat->metalness )
	//MAT_RELEASE( mat->roughness )

	memset( mat, 0, sizeof( material_t ) );
}
