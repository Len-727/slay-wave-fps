struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    
    float2 texcoord;
    if (vertexID == 0)
    {
        texcoord = float2(0, 0);
    }
    else if (vertexID == 1)
    {
        texcoord = float2(2, 0);
    }
    else
    {
        texcoord = float2(0, 2);
    }
    
    output.texcoord = texcoord;
    output.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}