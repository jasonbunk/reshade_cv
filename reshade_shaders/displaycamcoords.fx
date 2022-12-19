#include "ReShadeUI.fxh"

uniform float dispcam_offset_x < __UNIFORM_SLIDER_FLOAT1
    ui_min = 0; ui_max = 3600;
    ui_step = 10;
> = 100;

uniform float dispcam_offset_y < __UNIFORM_SLIDER_FLOAT1
    ui_min = 0; ui_max = 1800;
    ui_step = 10;
> = 100;

uniform int dispcam_precision < __UNIFORM_SLIDER_INT1
    ui_min = 0; ui_max = 9;
    ui_step = 1;
> = 4;


#include "ReShade.fxh"
#include "DrawText.fxh"

uniform float3 dispcam_latestcampos;

float4 PS_displaycamcoords(float4 pos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 color = tex2D(ReShade::BackBuffer, texcoord.xy);
    float4 res = 0; res.w = 1;
    if(DrawText_Digit(float2(dispcam_offset_x, dispcam_offset_y     ), 30, 1, texcoord, dispcam_precision, dispcam_latestcampos.x, res.x) < 0.0
	&& DrawText_Digit(float2(dispcam_offset_x, dispcam_offset_y+40.0), 30, 1, texcoord, dispcam_precision, dispcam_latestcampos.y, res.y) < 0.0
	&& DrawText_Digit(float2(dispcam_offset_x, dispcam_offset_y+80.0), 30, 1, texcoord, dispcam_precision, dispcam_latestcampos.z, res.z) < 0.0) {
        return color;
	}
	if(res.x < 0.0001 && res.y < 0.0001 && res.z < 0.0001) {
	    return color;
	}
	return 1; //res;
}


technique displaycamcoords
{
    pass {
        VertexShader = PostProcessVS;
        PixelShader = PS_displaycamcoords;
    }
}