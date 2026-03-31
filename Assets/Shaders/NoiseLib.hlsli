// ============================================================
//  NoiseLib.hlsli
//  Gothic Swarm - プロシージャルノイズ関数ライブラリ
//
//  【役割】
//  hash / noise / fbm / warpedFbm など、複数シェーダーで
//  重複していたノイズ関数を一箇所にまとめる。
//
//  【使用先】
//  TitleBackgroundPS, MenuPostProcessPS, BurnFlagPS, FurPS
// ============================================================
#ifndef NOISE_LIB_HLSLI
#define NOISE_LIB_HLSLI

// ============================================================
//  hash 系（疑似乱数生成）
//  入力座標から決定的に乱数を返す。同じ入力→同じ出力。
// ============================================================

// float2 → float のハッシュ（最も汎用的）
float Hash21(float2 p)
{
    p = frac(p * float2(234.34f, 435.345f));
    p += dot(p, p + 34.23f);
    return frac(p.x * p.y);
}

// float2 → float2 のハッシュ（ボロノイ等に使用）
float2 Hash22(float2 p)
{
    float x = Hash21(p);
    float y = Hash21(p + 127.1f);
    return float2(x, y);
}

// sin ベースのハッシュ（BurnFlagPS / MenuPostProcessPS 互換）
float HashSin(float2 p)
{
    float h = dot(p, float2(127.1f, 311.7f));
    return frac(sin(h) * 43758.5453f);
}

// ============================================================
//  Value Noise（値ノイズ）
//  格子点にランダム値を置き、バイリニア補間で滑らかにする。
//  Perlin Noise より安価だがやや「ブロック感」が出る。
// ============================================================
float ValueNoise(float2 p)
{
    float2 i = floor(p);       // 格子のどのセルか
    float2 f = frac(p);        // セル内の位置 (0〜1)

    // Hermite 補間（smoothstep 相当）→ 格子境界のカクつき軽減
    float2 u = f * f * (3.0f - 2.0f * f);

    // 4 隅のハッシュ値
    float a = Hash21(i);                    // 左下
    float b = Hash21(i + float2(1.0f, 0.0f)); // 右下
    float c = Hash21(i + float2(0.0f, 1.0f)); // 左上
    float d = Hash21(i + float2(1.0f, 1.0f)); // 右上

    // バイリニア補間
    return lerp(lerp(a, b, u.x),
                lerp(c, d, u.x), u.y);
}

// ============================================================
//  FBM (Fractal Brownian Motion / フラクタルブラウン運動)
//  ノイズを周波数を倍にしながら重ねて「自然な複雑さ」を作る。
//  octaves が多いほどディテールが増すが負荷も増える。
// ============================================================
float FBM(float2 p, int octaves)
{
    float value     = 0.0f;
    float amplitude = 0.5f;  // 各オクターブの振幅（重ねるごとに半減）
    float frequency = 1.0f;  // 各オクターブの周波数（重ねるごとに倍増）

    for (int i = 0; i < octaves; i++)
    {
        value     += amplitude * ValueNoise(p * frequency);
        frequency *= 2.0f;   // 周波数を倍に（ディテール追加）
        amplitude *= 0.5f;   // 振幅を半分に（高周波は控えめ）
    }
    return value;
}

// 3 オクターブ版（軽量：FurPS 等で使用）
float FBM3(float2 p)
{
    return FBM(p, 3);
}

// 5 オクターブ版（中品質：MenuPostProcessPS 等で使用）
float FBM5(float2 p)
{
    return FBM(p, 5);
}

// 7 オクターブ版（高品質：TitleBackgroundPS で使用）
float FBM7(float2 p)
{
    return FBM(p, 7);
}

// ============================================================
//  Domain Warping（ドメインワーピング）
//  FBM の入力 UV を、別の FBM の出力で歪ませる。
//  → 有機的な「うねり」「渦巻き」パターンを生成。
//  溶岩やマグマの表現に最適。
// ============================================================
float WarpedFBM(float2 uv, float t)
{
    float2 q = float2(
        FBM7(uv + float2(0.0f, 0.0f) + t * 0.06f),
        FBM7(uv + float2(5.2f, 1.3f) + t * 0.07f)
    );
    float2 r = float2(
        FBM7(uv + 4.0f * q + float2(1.7f, 9.2f) + t * 0.05f),
        FBM7(uv + 4.0f * q + float2(8.3f, 2.8f) + t * 0.06f)
    );
    return FBM7(uv + 4.0f * r);
}

#endif // NOISE_LIB_HLSLI
