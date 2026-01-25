struct VSInput
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// ƒƒCƒ“ŠÖ”
PSInput main(VSInput input)
{
    PSInput output;
    
   
    output.position = float4(input.position, 1.0f);
    
    // UVÀ•W‚ğ‚»‚Ì‚Ü‚Üo—Í
    output.texcoord = input.texcoord;
    
    return output;
}