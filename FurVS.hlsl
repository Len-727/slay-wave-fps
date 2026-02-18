// =============================================================
// FurVS.hlsl - ファーシェーダー（頂点シェーダー）
// 
// 【役割】シェル法でメッシュを法線方向に膨らませる
// 【仕組み】同じメッシュを何層も描画し、各層を少しずつ
//          法線方向にずらすことで毛/苔の立体感を出す
// =============================================================

// --- 定数バッファ（C++から毎フレーム送られるデータ）---
cbuffer FurParams : register(b0)
{
    matrix World; // ワールド変換行列（位置・回転・スケール）
    matrix View; // ビュー行列（カメラの位置と向き）
    matrix Projection; // 射影行列（3D→2Dへの変換）
    
    float FurLength; // 毛の長さ（苔なら0.05?0.15くらい）
    float CurrentLayer; // 現在の層番号（0.0?1.0に正規化済み）
    float TotalLayers; // 総層数（16?32）
    float Time; // 時間（風の揺れ用）
    
    float3 WindDirection; // 風の方向
    float WindStrength; // 風の強さ
};

// --- 入力（頂点バッファから来るデータ）---
struct VS_INPUT
{
    float3 Position : POSITION; // 頂点の位置
    float3 Normal : NORMAL; // 頂点の法線（面に垂直な方向）
    float2 TexCoord : TEXCOORD0; // テクスチャ座標（UV）
};

// --- 出力（ピクセルシェーダーに渡すデータ）---
struct VS_OUTPUT
{
    float4 Position : SV_POSITION; // スクリーン上の位置
    float2 TexCoord : TEXCOORD0; // UV座標
    float Layer : TEXCOORD1; // 現在の層（0.0=根元, 1.0=先端）
    float3 WorldPos : TEXCOORD2; // ワールド空間の位置
    float3 WorldNormal : TEXCOORD3; // ワールド空間の法線
};

// =============================================================
// メイン処理
// =============================================================
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // --- 層の進行度（0.0=根元、1.0=先端）---
    float layerRatio = CurrentLayer / TotalLayers;
    
    // --- 法線方向に頂点を押し出す ---
    // 上の層ほど多く押し出す → 毛が生えてるように見える
    float3 displacement = input.Normal * FurLength * layerRatio;
    
    // --- 風の影響（先端ほど大きく揺れる）---
    // sin波で自然な揺れを作る
    float windWave = sin(Time * 2.0 + input.Position.x * 0.5 + input.Position.z * 0.7);
    float3 windOffset = WindDirection * WindStrength * windWave * layerRatio * layerRatio;
    // layerRatio^2 = 根元は動かず、先端だけ大きく揺れる
    
    // --- 最終位置 = 元の位置 + 押し出し + 風 ---
    float3 furPosition = input.Position + displacement + windOffset;
    
    // --- 座標変換: ローカル → ワールド → ビュー → スクリーン ---
    float4 worldPos = mul(float4(furPosition, 1.0), World);
    float4 viewPos = mul(worldPos, View);
    output.Position = mul(viewPos, Projection);
    
    // --- ピクセルシェーダーに渡す情報 ---
    output.TexCoord = input.TexCoord;
    output.Layer = layerRatio;
    output.WorldPos = worldPos.xyz;
    output.WorldNormal = normalize(mul(input.Normal, (float3x3) World));
    
    return output;
}
