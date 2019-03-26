#pragma once

#include <Engine/ThirdParty/imgui/imgui.h>
#include <string>

void Ed_RenderToDClock( const unsigned long long tickCount )
{
	const ImVec2 winSize = ImGui::GetIO().DisplaySize;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	static ImVec4 col = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
	static ImVec4 col2 = ImVec4( 1.0f, 0.0f, 0.0f, 1.0f );

	const ImU32 col32 = ImColor( col );
	const ImU32 col232 = ImColor( col2 );

	const float clockRadius = winSize.x / 34.0f;

	const ImVec2 clockCenter = ImVec2( winSize.x - clockRadius - 5.0f, winSize.y - clockRadius - 25.0f );

	unsigned long long mseconds = tickCount % 100;
	unsigned long long seconds = tickCount / 100;
	unsigned long long minutes = seconds / 60;
	unsigned long long hours = 6 + minutes / 60;

	hours %= 24;
	minutes %= 60;
	seconds %= 60;

	static char timeStr[12] = {};
	sprintf_s( timeStr, "%02llu:%02llu:%02llu.%02llu", hours, minutes, seconds, mseconds );

	const ImVec2 dCSize = ImGui::CalcTextSize( timeStr );
	const ImVec2 dLSize = ImGui::CalcTextSize( "World ToD" );
	const ImVec2 digitalClockCenter = ImVec2( clockCenter.x - dCSize.x / 2.0f, winSize.y - 20.0f );
	const ImVec2 clockLabelCenter = ImVec2( clockCenter.x - dLSize.x / 2.0f, clockCenter.y - clockRadius - dLSize.y - 5.0f );

	const ImVec2 mNeedle = ImVec2( clockCenter.x, clockCenter.y - clockRadius / 1.25f );
	const ImVec2 hNeedle = ImVec2( clockCenter.x, clockCenter.y - clockRadius / 2.00f );

	draw_list->AddText( clockLabelCenter, col32, "World ToD" );

	draw_list->AddCircle( clockCenter, clockRadius, col32, 20, 1.0f );

	draw_list->AddLine( clockCenter, mNeedle, col32, 1.0f );
	draw_list->AddLine( clockCenter, hNeedle, col232, 1.0f );

	draw_list->AddText( digitalClockCenter, col32, timeStr );
}
