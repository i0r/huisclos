#pragma once

#include <Engine/Graphics/Camera.h>

#include <Engine/ThirdParty/imgui/imgui.h>
#include <Editor/ThirdParty/ImGuizmo/ImGuizmo.h>

void Ed_PanelTransformation( const Camera* cam, int& activeManipulationMode, DirectX::XMFLOAT4X4& modelMatrix, float* translation, float* rotation = nullptr, float* scale = nullptr )
{
	static float fakeStorage[3] = {};

	ImGuizmo::DecomposeMatrixToComponents(
		( float* )modelMatrix.m,
		translation,
		( rotation != nullptr ) ? rotation : fakeStorage,
		( scale != nullptr ) ? scale : fakeStorage
	);

	{
		ImGui::InputFloat3( "Translation", translation, 3 );

		if ( rotation != nullptr ) ImGui::InputFloat3( "Rotation", rotation, 3 );
		if ( scale != nullptr ) ImGui::InputFloat3( "Scale", scale, 3 );
	}

	ImGuizmo::RecomposeMatrixFromComponents(
		translation,
		( rotation != nullptr ) ? rotation : fakeStorage,
		( scale != nullptr ) ? scale : fakeStorage,
		( float* )modelMatrix.m
	);

	ImGui::RadioButton( "Translate (W)", &activeManipulationMode, 0 );

	if ( rotation != nullptr ) {
		ImGui::SameLine();
		ImGui::RadioButton( "Rotate (X)", &activeManipulationMode, 1 );
	}

	if ( scale != nullptr ) {
		ImGui::SameLine();
		ImGui::RadioButton( "Scale (C)", &activeManipulationMode, 2 );
	}

	ImGuizmo::Manipulate( cam->GetViewMatrix(), cam->GetProjectionMatrix(), static_cast< ImGuizmo::OPERATION >( activeManipulationMode ), ImGuizmo::MODE::WORLD, ( float* )modelMatrix.m );

	ImGuizmo::DecomposeMatrixToComponents(
		( float* )modelMatrix.m,
		translation,
		( rotation != nullptr ) ? rotation : fakeStorage,
		( scale != nullptr ) ? scale : fakeStorage
	);
}
