// ========================================
// FireParticleVS_Standalone.hlsl
// ========================================

// ========================================
// 定数バッファ
// ========================================

cbuffer MatrixBuffer : register(b0)
{
    matrix viewMatrix;
    matrix projectionMatrix;
};

//=========================================
//  構造体
//=========================================
struct VS_INPUT
{
    float3 position : POSITION; //  パーティクル中心位置
    float2 texCoord : TEXCOORD0; //  UV座標
    float4 color : COLOR; //  パーティクル色
    float size : PSIZE; //  パーティクルサイズ  
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
};

// ========================================
// 頂点シェーダー（ビルボード）
// ========================================

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // === ビルボード計算 ===
    // カメラを常に向く四角形を作る
    
    // パーティクルの中心位置
    float3 centerPos = input.position;
    
    // ビュー行列からカメラの右・上ベクトルを取り出す
    // viewMatrix の列ベクトルを使う
    float3 right = float3(viewMatrix._11, viewMatrix._21, viewMatrix._31);
    float3 up = float3(viewMatrix._12, viewMatrix._22, viewMatrix._32);
    
    // UV座標から四角形の頂点位置を計算
    // texCoord: (0,0), (1,0), (0,1), (1,1)
    // 中心からのオフセット
    float2 offset = (input.texCoord - 0.5) * input.size;
    
    // ビルボード頂点位置を計算
    // centerPos + (右方向 * offset.x) + (上方向 * offset.y)
    float3 worldPos = centerPos + right * offset.x + up * offset.y;
    
    // ビュー座標に変換
    float4 viewPos = mul(float4(worldPos, 1.0), viewMatrix);
    
    // プロジェクション座標に変換
    output.position = mul(viewPos, projectionMatrix);
    
    // UV座標と色をそのまま渡す
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}