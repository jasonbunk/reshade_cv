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
> = 6;


#include "ReShade.fxh"
#include "DrawText.fxh"

uniform float4 dispcam_latestcampos;
uniform float4 dispcam_latestcamcol0;
uniform float4 dispcam_latestcamcol1;
uniform float4 dispcam_latestcamcol2;

float PS_displaycamcoordscol(float coldrawoffset, float4 camcol, float2 texcoord : TEXCOORD) : SV_Target
{
    if(camcol.w < 0.5) {
        return 0;
    }
    float3 res = 0;
    if(DrawText_Digit(float2(coldrawoffset+dispcam_offset_x, dispcam_offset_y     ), 30, 1, texcoord, dispcam_precision, camcol.x, res.x) < 0.0
	&& DrawText_Digit(float2(coldrawoffset+dispcam_offset_x, dispcam_offset_y+40.0), 30, 1, texcoord, dispcam_precision, camcol.y, res.y) < 0.0
	&& DrawText_Digit(float2(coldrawoffset+dispcam_offset_x, dispcam_offset_y+80.0), 30, 1, texcoord, dispcam_precision, camcol.z, res.z) < 0.0) {
        return 0;
	}
	if(res.x < 0.0001 && res.y < 0.0001 && res.z < 0.0001) {
	    return 0;
	}
	return 1;
}


float4 PS_displaycamcoords(float4 pos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 color = tex2D(ReShade::BackBuffer, texcoord.xy);
	if(PS_displaycamcoordscol(  0.0, dispcam_latestcamcol0, texcoord) > 0.5
	|| PS_displaycamcoordscol(150.0, dispcam_latestcamcol1, texcoord) > 0.5
	|| PS_displaycamcoordscol(300.0, dispcam_latestcamcol2, texcoord) > 0.5
	|| PS_displaycamcoordscol(500.0, dispcam_latestcampos,  texcoord) > 0.5) {
	    return 1; // white
	}
	return color;
}

technique displaycamcoords
{
    pass {
        VertexShader = PostProcessVS;
        PixelShader = PS_displaycamcoords;
    }
}
