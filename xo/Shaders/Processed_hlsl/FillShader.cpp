#include "pch.h"
#if XO_BUILD_DIRECTX
#include "FillShader.h"

namespace xo {

DXProg_Fill::DXProg_Fill()
{
	Reset();
}

void DXProg_Fill::Reset()
{
	ResetBase();

}

const char* DXProg_Fill::VertSrc()
{
	return
		"\n"
		"cbuffer PerFrame : register(b0)\n"
		"{\n"
		"	float4x4		mvproj;\n"
		"	float2			vport_hsize;\n"
		"\n"
		"	Texture2D		shader_texture;\n"
		"	SamplerState	sample_type;\n"
		"};\n"
		"\n"
		"cbuffer PerObject : register(b1)\n"
		"{\n"
		"	float4		box;\n"
		"	float4		border;\n"
		"	float4		border_color;\n"
		"	float		radius;\n"
		"};\n"
		"\n"
		"struct VertexType_PTC\n"
		"{\n"
		"	float4 pos		: POSITION;\n"
		"	float2 uv		: TEXCOORD0;\n"
		"	float4 color	: COLOR;\n"
		"};\n"
		"\n"
		"struct VertexType_PTCV4\n"
		"{\n"
		"	float4 pos		: POSITION;\n"
		"	float2 uv		: TEXCOORD1;\n"
		"	float4 color	: COLOR;\n"
		"	float4 v4		: TEXCOORD2;\n"
		"};\n"
		"\n"
		"float fromSRGB_Component(float srgb)\n"
		"{\n"
		"	float sRGB_Low	= 0.0031308;\n"
		"	float sRGB_a	= 0.055;\n"
		"\n"
		"	if (srgb <= 0.04045)\n"
		"		return srgb / 12.92;\n"
		"	else\n"
		"		return pow(abs((srgb + sRGB_a) / (1.0 + sRGB_a)), 2.4);\n"
		"}\n"
		"\n"
		"float4 fromSRGB(float4 c)\n"
		"{\n"
		"	float4 linear_c;\n"
		"	linear_c.r = fromSRGB_Component(c.r);\n"
		"	linear_c.g = fromSRGB_Component(c.g);\n"
		"	linear_c.b = fromSRGB_Component(c.b);\n"
		"	linear_c.a = c.a;\n"
		"	return linear_c;\n"
		"}\n"
		"\n"
		"float4 premultiply(float4 c)\n"
		"{\n"
		"	return float4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);\n"
		"}\n"
		"\n"
		"// SV_Position is in screen space, but in GLSL it is in normalized device space\n"
		"float2 frag_to_screen(float2 unit_pt)\n"
		"{\n"
		"	return unit_pt;\n"
		"}\n"
		"\n"
		"#define SHADER_TYPE_MASK     15\n"
		"#define SHADER_FLAG_TEXBG    16\n"
		"\n"
		"#define SHADER_ARC           1\n"
		"#define SHADER_RECT          2\n"
		"#define SHADER_TEXT_SIMPLE   3\n"
		"#define SHADER_TEXT_SUBPIXEL 4\n"
		"\n"
		"struct VSOutput\n"
		"{\n"
		"	float4 pos		: SV_Position;\n"
		"	float4 color	: COLOR;\n"
		"};\n"
		"\n"
		"VSOutput main(VertexType_PTC vertex)\n"
		"{\n"
		"	VSOutput output;\n"
		"	output.pos = mul(mvproj, vertex.pos);\n"
		"	output.color = fromSRGB(vertex.color);\n"
		"	return output;\n"
		"}\n"
;
}

const char* DXProg_Fill::FragSrc()
{
	return
		"\n"
		"cbuffer PerFrame : register(b0)\n"
		"{\n"
		"	float4x4		mvproj;\n"
		"	float2			vport_hsize;\n"
		"\n"
		"	Texture2D		shader_texture;\n"
		"	SamplerState	sample_type;\n"
		"};\n"
		"\n"
		"cbuffer PerObject : register(b1)\n"
		"{\n"
		"	float4		box;\n"
		"	float4		border;\n"
		"	float4		border_color;\n"
		"	float		radius;\n"
		"};\n"
		"\n"
		"struct VertexType_PTC\n"
		"{\n"
		"	float4 pos		: POSITION;\n"
		"	float2 uv		: TEXCOORD0;\n"
		"	float4 color	: COLOR;\n"
		"};\n"
		"\n"
		"struct VertexType_PTCV4\n"
		"{\n"
		"	float4 pos		: POSITION;\n"
		"	float2 uv		: TEXCOORD1;\n"
		"	float4 color	: COLOR;\n"
		"	float4 v4		: TEXCOORD2;\n"
		"};\n"
		"\n"
		"float fromSRGB_Component(float srgb)\n"
		"{\n"
		"	float sRGB_Low	= 0.0031308;\n"
		"	float sRGB_a	= 0.055;\n"
		"\n"
		"	if (srgb <= 0.04045)\n"
		"		return srgb / 12.92;\n"
		"	else\n"
		"		return pow(abs((srgb + sRGB_a) / (1.0 + sRGB_a)), 2.4);\n"
		"}\n"
		"\n"
		"float4 fromSRGB(float4 c)\n"
		"{\n"
		"	float4 linear_c;\n"
		"	linear_c.r = fromSRGB_Component(c.r);\n"
		"	linear_c.g = fromSRGB_Component(c.g);\n"
		"	linear_c.b = fromSRGB_Component(c.b);\n"
		"	linear_c.a = c.a;\n"
		"	return linear_c;\n"
		"}\n"
		"\n"
		"float4 premultiply(float4 c)\n"
		"{\n"
		"	return float4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);\n"
		"}\n"
		"\n"
		"// SV_Position is in screen space, but in GLSL it is in normalized device space\n"
		"float2 frag_to_screen(float2 unit_pt)\n"
		"{\n"
		"	return unit_pt;\n"
		"}\n"
		"\n"
		"#define SHADER_TYPE_MASK     15\n"
		"#define SHADER_FLAG_TEXBG    16\n"
		"\n"
		"#define SHADER_ARC           1\n"
		"#define SHADER_RECT          2\n"
		"#define SHADER_TEXT_SIMPLE   3\n"
		"#define SHADER_TEXT_SUBPIXEL 4\n"
		"\n"
		"struct VSOutput\n"
		"{\n"
		"	float4 pos		: SV_Position;\n"
		"	float4 color	: COLOR;\n"
		"};\n"
		"\n"
		"float4 main(VSOutput input) : SV_Target\n"
		"{\n"
		"    return input.color;\n"
		"}\n"
;
}

const char* DXProg_Fill::Name()
{
	return "Fill";
}


bool DXProg_Fill::LoadVariablePositions()
{
	int nfail = 0;

	if (nfail != 0)
		Trace("Failed to bind %d variables of shader Fill\n", nfail);

	return nfail == 0;
}

uint32_t DXProg_Fill::PlatformMask()
{
	return Platform_All;
}

xo::VertexType DXProg_Fill::VertexType()
{
	return VertexType_PTC;
}

} // namespace xo

#endif // XO_BUILD_DIRECTX

