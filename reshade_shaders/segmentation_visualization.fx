// Copyright Jason Bunk (2023)
#include "ReShade.fxh"

// This is a small part of the semantic segmentation visualization tool.
// The colorization logic is currently in C++ in "buffer_colorizer_visualization.cpp".
// I tried doing the colorization logic here, but I had trouble passing/copying the RGBA32U segmentation index buffer to this shader.
// Reshade itself says it supports RGBA32F but not RGBA32U, so maybe it would need an update.

texture2D SemSegTex
{
	Width = BUFFER_WIDTH;
	Height = BUFFER_HEIGHT;
	Format = RGBA8;
};
sampler2D SemSegSampler { Texture = SemSegTex; };

uniform float SemSegDrawAlpha <
    ui_max = 1.0;
    ui_min = 0.0;
    ui_tooltip = "Alpha blend with original game. 1 = fully replace with seg, 0 = as if this shader were disabled.";
    ui_type = "slider";
> = 0.8;

float4 PS_SemSegView(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 color = tex2D(ReShade::BackBuffer, texcoord);
	float4 sampled = tex2D(SemSegSampler, texcoord);
	return color * (1 - SemSegDrawAlpha) + sampled * SemSegDrawAlpha;
}

technique SemSegView < enabled = false; >
{
	pass SemSegViewPass
	{
		VertexShader = PostProcessVS;
		PixelShader = PS_SemSegView;
	}
}
