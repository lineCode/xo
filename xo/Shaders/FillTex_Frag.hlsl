
struct VSOutput
{
	float4 pos		: SV_Position;
	float4 color	: COLOR;
	float2 texuv0	: TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target
{
	float4 col;
	col = input.color * shader_texture.Sample(sample_type, input.texuv0);
    return col;
}
