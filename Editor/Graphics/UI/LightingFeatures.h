#pragma once

#include <Engine/ThirdParty/imgui/imgui.h>

static constexpr int intensityPresetCount = 12;
static constexpr char* intensityPresetLabels[intensityPresetCount] = { "LED 37 mW", "Candle flame", "LED 1W", "Incandescent lamp 40W", "Fluorescent lamp", "Tungsten Bulb 100W", "Fluorescent light", "HID Car headlight", "Induction Lamp 200W", "Low pressure sodium vapor lamp 127W", "Metal-halide lamp 400W", "Direct sunlight" };
static constexpr float intensityPresetValues[intensityPresetCount] = { 0.20f, 15.0f, 75.0f, 325.0f, 1200.0f, 1600.0f, 2600.0f, 3000.0f, 16000.0f, 25000.0f, 40000.0f, 64000.0f };

static constexpr int temperaturePresetCount = 13;
// presets from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf and some google search
static constexpr char* temperaturePresetLabels[temperaturePresetCount] = { "Candle flame", "Tungsten bulb", "Tungsten lamp 500W-1K", "Quartz light", "Tungsten lamp 2K",
"Tungsten lamp 5K/10K", "Mecury vapor bulb", "Daylight", "RGB Monitor", "Fluorescent lights", "Sky overcast", "Outdoor shade areas", "Sky partly cloudy" };
static constexpr float temperaturePresetValues[temperaturePresetCount] = { 1930.0f, 2900.0f, 3000.0f, 3500.0f, 3275.0f, 3380.0f, 5700.0f, 6000.0f, 6280.0f, 6500.0f, 7500.0f, 8000.0f, 9000.0f };

static constexpr char* colorMode[2] = { "RGB", "Temperature" };

namespace
{
	DirectX::XMFLOAT4 TemperatureToRGB( const float colorTemperature )
	{
		float x = colorTemperature / 1000.0f;
		float x2 = x * x;
		float x3 = x2 * x;
		float x4 = x3 * x;
		float x5 = x4 * x;

		float R = 0.0f, G = 0.0f, B = 0.0f;

		// red
		if ( colorTemperature <= 6600.0f ) {
			R = 1.0f;
		} else {
			R = 0.0002889f * x5 - 0.01258f * x4 + 0.2148f * x3 - 1.776f * x2 + 6.907f * x - 8.723f;
		}

		// green
		if ( colorTemperature <= 6600.0f ) {
			G = -4.593e-05f * x5 + 0.001424f * x4 - 0.01489f * x3 + 0.0498f * x2 + 0.1669f * x - 0.1653f;
		} else {
			G = -1.308e-07f * x5 + 1.745e-05f * x4 - 0.0009116f * x3 + 0.02348f * x2 - 0.3048f * x + 2.159f;
		}

		// blue
		if ( colorTemperature <= 2000.0f ) {
			B = 0.0f;
		} else if ( colorTemperature < 6600.0f ) {
			B = 1.764e-05f * x5 + 0.0003575f * x4 - 0.01554f * x3 + 0.1549f * x2 - 0.3682f * x + 0.2386f;
		} else {
			B = 1.0f;
		}

		return DirectX::XMFLOAT4( R, G, B, 0.0f );
	}

	// RGB -> K approximation
	float RGBToTemperature( float* colorRGB )
	{
		float tMin = 2000.0f, tMax = 23000.0f;
		float colorTemperature = 0.0f;

		for ( colorTemperature = ( tMax + tMin ) / 2.0f; tMax - tMin > 0.1f; colorTemperature = ( tMax + tMin ) / 2.0f ) {
			DirectX::XMFLOAT4 newColor = TemperatureToRGB( colorTemperature );

			if ( newColor.z / newColor.x > colorRGB[2] / colorRGB[0] ) {
				tMax = colorTemperature;
			} else {
				tMin = colorTemperature;
			}
		}

		return colorTemperature;
	}
}

void Ed_PanelLuminousPower( float* lightIntensity )
{
	ImGui::SliderFloat( "Luminous Power (lm)", lightIntensity, 0.0f, 64000.0f ); // 60 000 is roughly the illuminance of the sun at noon on a clear sky day

	int intensityPresetIndex = -1;

	if ( ImGui::Combo( "Preset (lm)", &intensityPresetIndex, intensityPresetLabels, intensityPresetCount ) && intensityPresetIndex != -1 ) {
		*lightIntensity = intensityPresetValues[intensityPresetIndex];
	}

	ImGui::Separator();
}

void Ed_PanelColor( int& activeColorMode, float* colorRGB )
{
	ImGui::Combo( "Color Mode", &activeColorMode, colorMode, 2 );

	if ( activeColorMode == 0 ) {
		ImColor lightCol = { colorRGB[0], colorRGB[1], colorRGB[2] };

		ColorPicker( "Color (sRGB)", &lightCol );

		// technically, we could directly assign the lightCol array to the input pointer, but this might fuck up the luminous power element
		colorRGB[0] = lightCol.Value.x;
		colorRGB[1] = lightCol.Value.y;
		colorRGB[2] = lightCol.Value.z;
	} else {
		float colorTemperature = RGBToTemperature( colorRGB );

		ImGui::SliderFloat( "Temperature (K)", &colorTemperature, 0.0f, 10000.0f );

		int temperaturePresetIndex = -1;

		if ( ImGui::Combo( "Preset (K)", &temperaturePresetIndex, temperaturePresetLabels, temperaturePresetCount ) && temperaturePresetIndex != -1 ) {
			colorTemperature = temperaturePresetValues[temperaturePresetIndex];
		}

		DirectX::XMFLOAT4 newColor = TemperatureToRGB( colorTemperature );

		ImGui::ColorEdit3( "Color (sRGB)", &newColor.x );

		colorRGB[0] = newColor.x;
		colorRGB[1] = newColor.y;
		colorRGB[2] = newColor.z;
	}
}
