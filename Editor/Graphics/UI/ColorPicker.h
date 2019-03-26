#pragma once

void AddTriangleFilledMultiColor( ImDrawList* drawList, const ImVec2& a, const ImVec2& b, const ImVec2& c, ImU32 col_a, ImU32 col_b, ImU32 col_c )
{
	if ( ( ( col_a | col_b | col_c ) >> 24 ) == 0 )
		return;

	const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
	drawList->PrimReserve( 3, 3 );
	drawList->PrimWriteIdx( ( ImDrawIdx )( drawList->_VtxCurrentIdx ) ); drawList->PrimWriteIdx( ( ImDrawIdx )( drawList->_VtxCurrentIdx + 1 ) ); drawList->PrimWriteIdx( ( ImDrawIdx )( drawList->_VtxCurrentIdx + 2 ) );
	drawList->PrimWriteVtx( a, uv, col_a );
	drawList->PrimWriteVtx( b, uv, col_b );
	drawList->PrimWriteVtx( c, uv, col_c );
}

bool ColorPicker( const char* label, ImColor* color )
{
	static const float HUE_PICKER_WIDTH = 20.0f;
	static const float CROSSHAIR_SIZE = 7.0f;
	static const ImVec2 SV_PICKER_SIZE = ImVec2( 200, 200 );

	bool value_changed = false;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImVec2 picker_pos = ImGui::GetCursorScreenPos();

	ImColor colors[] = { ImColor( 255, 0, 0 ),
		ImColor( 255, 255, 0 ),
		ImColor( 0, 255, 0 ),
		ImColor( 0, 255, 255 ),
		ImColor( 0, 0, 255 ),
		ImColor( 255, 0, 255 ),
		ImColor( 255, 0, 0 ) };

	for ( int i = 0; i < 6; ++i )
	{
		draw_list->AddRectFilledMultiColor(
			ImVec2( picker_pos.x + SV_PICKER_SIZE.x + 10, picker_pos.y + i * ( SV_PICKER_SIZE.y / 6 ) ),
			ImVec2( picker_pos.x + SV_PICKER_SIZE.x + 10 + HUE_PICKER_WIDTH,
				picker_pos.y + ( i + 1 ) * ( SV_PICKER_SIZE.y / 6 ) ),
			colors[i],
			colors[i],
			colors[i + 1],
			colors[i + 1] );
	}

	float hue, saturation, value;
	ImGui::ColorConvertRGBtoHSV(
		color->Value.x, color->Value.y, color->Value.z, hue, saturation, value );
	auto hue_color = ImColor::HSV( hue, 1, 1 );

	draw_list->AddLine(
		ImVec2( picker_pos.x + SV_PICKER_SIZE.x + 8, picker_pos.y + hue * SV_PICKER_SIZE.y ),
		ImVec2( picker_pos.x + SV_PICKER_SIZE.x + 12 + HUE_PICKER_WIDTH,
			picker_pos.y + hue * SV_PICKER_SIZE.y ),
		ImColor( 255, 255, 255 ) );

	AddTriangleFilledMultiColor( draw_list, picker_pos,
		ImVec2( picker_pos.x + SV_PICKER_SIZE.x, picker_pos.y + SV_PICKER_SIZE.y ),
		ImVec2( picker_pos.x, picker_pos.y + SV_PICKER_SIZE.y ),
		ImColor( 0, 0, 0 ),
		hue_color,
		ImColor( 255, 255, 255 ) );

	float x = saturation * value;
	ImVec2 p( picker_pos.x + x * SV_PICKER_SIZE.x, picker_pos.y + value * SV_PICKER_SIZE.y );
	draw_list->AddLine( ImVec2( p.x - CROSSHAIR_SIZE, p.y ), ImVec2( p.x - 2, p.y ), ImColor( 255, 255, 255 ) );
	draw_list->AddLine( ImVec2( p.x + CROSSHAIR_SIZE, p.y ), ImVec2( p.x + 2, p.y ), ImColor( 255, 255, 255 ) );
	draw_list->AddLine( ImVec2( p.x, p.y + CROSSHAIR_SIZE ), ImVec2( p.x, p.y + 2 ), ImColor( 255, 255, 255 ) );
	draw_list->AddLine( ImVec2( p.x, p.y - CROSSHAIR_SIZE ), ImVec2( p.x, p.y - 2 ), ImColor( 255, 255, 255 ) );

	ImGui::InvisibleButton( "saturation_value_selector", SV_PICKER_SIZE );
	if ( ImGui::IsItemHovered() )
	{
		ImVec2 mouse_pos_in_canvas = ImVec2(
			ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y );
		if ( ImGui::GetIO().MouseDown[0] )
		{
			mouse_pos_in_canvas.x =
				min( mouse_pos_in_canvas.x, mouse_pos_in_canvas.y );

			value = mouse_pos_in_canvas.y / SV_PICKER_SIZE.y;
			saturation = value == 0 ? 0 : ( mouse_pos_in_canvas.x / SV_PICKER_SIZE.x ) / value;
			value_changed = true;
		}
	}

	ImGui::SetCursorScreenPos( ImVec2( picker_pos.x + SV_PICKER_SIZE.x + 10, picker_pos.y ) );
	ImGui::InvisibleButton( "hue_selector", ImVec2( HUE_PICKER_WIDTH, SV_PICKER_SIZE.y ) );

	if ( ImGui::IsItemHovered() )
	{
		if ( ImGui::GetIO().MouseDown[0] )
		{
			hue = ( ( ImGui::GetIO().MousePos.y - picker_pos.y ) / SV_PICKER_SIZE.y );
			value_changed = true;
		}
	}

	*color = ImColor::HSV( hue, saturation, value );
	return value_changed | ImGui::ColorEdit3( label, &color->Value.x );
}
