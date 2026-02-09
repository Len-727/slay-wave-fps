// ========================================
// MenuPostProcessPS.hlsl
// ========================================

Texture2D sceneTexture : register(t0);
Texture2D noiseTexture : register(t1);
SamplerState samplerState : register(s0);


cbuffer MenuFXParams : register(b0)
{
    float time; // 経過時間
    float shakeIntensity; // シェイク強度（0～1）
    float vignetteIntensity; // ビネット強度（0～1）
    float lightningIntensity; // ※旧名だが「FX全体の強度」として再利用
    
    float chromaticAmount; // 色収差量
    float screenWidth; // 画面幅
    float screenHeight; // 画面高さ
    float heartbeat; // ※旧名だが「脈動値」として再利用
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// ========================================
// ハッシュ関数（疑似ランダム）
// ========================================
float hash(float2 p)
{
    float h = dot(p, float2(127.1, 311.7));
    return frac(sin(h) * 43758.5453);
}

// 2Dノイズ
float noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + float2(1, 0));
    float c = hash(i + float2(0, 1));
    float d = hash(i + float2(1, 1));
    
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// ========================================
// 熱歪み（ヒートヘイズ）
// 鍛冶場の熱気で画面がゆらゆら歪む
// ========================================
float2 heatDistortion(float2 uv, float t)
{
    // 2層のノイズで有機的な歪みを作る
    // 上方向にスクロール＝熱気が昇る感じ
    float distX = noise2D(uv * 8.0 + float2(t * 0.3, -t * 0.8)) - 0.5;
    float distY = noise2D(uv * 6.0 + float2(-t * 0.2, -t * 1.0) + 50.0) - 0.5;
    
    // 画面下部ほど歪みが強い（熱源が下にある）
    float heatMask = smoothstep(0.0, 0.8, uv.y);
    
    // 歪み量（shakeIntensityで制御）
    float strength = 0.006 * (1.0 + shakeIntensity * 2.0);
    
    return float2(distX, distY) * strength * heatMask;
}

// ========================================
// エンバーオーバーレイ（FX層の火の粉）リアル版
// noise2Dベース：グリッド構造なし → 四角くならない
// ========================================

// FBM（複数オクターブのノイズ重ね）
float fxFbm(float2 p)
{
    float value = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    for (int i = 0; i < 5; i++)
    {
        value += amp * noise2D(p * freq);
        freq *= 2.0;
        amp *= 0.5;
    }
    return value;
}

// 1レイヤー分の火の粉
float3 fxEmberLayer(float2 uv, float t, float scrollSpeed, float noiseScale,
                    float threshold, float brightness, float3 tintA, float3 tintB)
{
    // UVスクロール（上昇＋横揺れ）
    float2 scrollUV = uv * noiseScale;
    scrollUV.y -= t * scrollSpeed;
    scrollUV.x += sin(t * 0.5 + uv.y * 2.0) * 0.4;

    // ノイズで有機的な光点を生成
    float n = fxFbm(scrollUV);

    // 閾値カット → 高い部分だけ光る
    float spark = saturate((n - threshold) / (1.0 - threshold));
    spark = pow(spark, 1.8); // 1.8 = 程よいシャープさ（3.5だと暗すぎた）

    // 色バリエーション
    float colorMix = frac(n * 5.71 + scrollUV.x * 0.13);
    float3 sparkColor = lerp(tintA, tintB, colorMix);

    // 白熱コア（特に明るい部分は白っぽく）
    float core = saturate((n - (threshold + 0.04)) / 0.03);
    core = pow(core, 1.5);
    sparkColor = lerp(sparkColor, float3(1.0, 0.95, 0.8), core * 0.7);

    // 高さフェード（下部ほど多い）
    float heightFade = smoothstep(0.05, 0.75, uv.y);

    // チカチカ
    float flicker = 0.7 + 0.3 * sin(t * 7.0 + n * 40.0);

    return sparkColor * spark * brightness * heightFade * flicker;
}

float3 emberOverlay(float2 uv, float t)
{
    float3 result = float3(0, 0, 0);

    float3 redOrange = float3(1.0, 0.35, 0.05);
    float3 yellowOrange = float3(1.0, 0.7, 0.15);
    float3 brightYellow = float3(1.0, 0.85, 0.3);

    // 大粒（ゆっくり、目立つ）
    result += fxEmberLayer(uv, t,
        0.12, 8.0, 0.55, 1.2,
        redOrange, yellowOrange);

    // 中粒1
    result += fxEmberLayer(uv, t + 15.0,
        0.22, 14.0, 0.58, 1.0,
        yellowOrange, brightYellow);

    // 中粒2（ずらし）
    result += fxEmberLayer(uv + float2(5.3, 2.1), t + 30.0,
        0.18, 18.0, 0.60, 0.9,
        redOrange, yellowOrange);

    // 小粒（速い）
    result += fxEmberLayer(uv + float2(11.7, 7.4), t + 45.0,
        0.35, 28.0, 0.62, 0.7,
        yellowOrange, brightYellow);

    // 微粒子（キラキラ）
    result += fxEmberLayer(uv + float2(23.1, 14.8), t + 60.0,
        0.5, 40.0, 0.65, 0.5,
        brightYellow, float3(1.0, 0.95, 0.7));

    return result;
}

// ========================================
// 金属スキャンライン
// 極細の横線を走らせる
// ========================================
float scanlines(float2 uv, float t)
{
    // 高速で動くスキャンライン
    float line1 = frac(uv.y * screenHeight * 0.5);
    line1 = smoothstep(0.0, 0.03, line1) * smoothstep(0.06, 0.03, line1);
    
    // ゆっくり動く太いバンド（CRTモニタ風）
    float band = sin(uv.y * 40.0 - t * 2.0) * 0.5 + 0.5;
    band = smoothstep(0.4, 0.6, band);
    
    // たまに走るグリッチライン
    float glitchY = hash(float2(floor(t * 3.0), 7.0));
    float glitch = 1.0 - smoothstep(0.0, 0.005, abs(uv.y - glitchY));
    glitch *= step(0.85, hash(float2(floor(t * 2.0), 13.0)));
    
    return line1 * 0.03 + band * 0.02 + glitch * 0.15;
}

// ========================================
// ヒートブルーム（明るい部分をにじませる）
// ========================================
float3 heatBloom(float2 uv, float t)
{
    // 画面下部のオレンジグロー（炉の反射光）
    float bottomGlow = smoothstep(1.0, 0.5, uv.y);
    float glowNoise = noise2D(float2(uv.x * 5.0, t * 0.3));
    bottomGlow *= glowNoise;
    
    // 脈動で明滅
    float pulse = heartbeat * heartbeat;
    bottomGlow *= (0.6 + pulse * 0.4);
    
    return float3(0.12, 0.03, 0.005) * bottomGlow;
}

// ========================================
// メイン関数
// ========================================
float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.texCoord;
    float t = time;
    
    // ========================================
    // 1) 熱歪み（ヒートヘイズ）
    // ========================================
    float2 distortion = heatDistortion(uv, t);
    float2 distortedUV = uv + distortion;
    
    // ========================================
    // 2) 暖色の色収差（RGB分離）
    //    冷たい色収差ではなく、炎の熱で滲むイメージ
    // ========================================
    float2 center = float2(0.5, 0.5);
    float2 dir = distortedUV - center;
    float dist = length(dir);
    
    // 色収差のオフセット（画面端ほど強い）
    float2 caOffset = dir * dist * chromaticAmount * 1.5;
    
    // 赤を外側に、青を内側に（暖色が広がる感じ）
    float r = sceneTexture.Sample(samplerState, distortedUV + caOffset * 1.2).r;
    float g = sceneTexture.Sample(samplerState, distortedUV + caOffset * 0.3).g;
    float b = sceneTexture.Sample(samplerState, distortedUV - caOffset * 0.5).b;
    float a = sceneTexture.Sample(samplerState, distortedUV).a;
    
    float3 color = float3(r, g, b);
    
    // ========================================
    // 3) 暖色ビネット（画面端をオレンジ暗く）
    //    ホラーの「赤い縁」→「炉の熱で焼ける縁」に
    // ========================================
    float vignette = 1.0 - dist * 1.3;
    vignette = saturate(vignette);
    vignette = pow(vignette, 1.3);
    
    // ビネットの色＝暗いオレンジ（黒ではなく暖色で暗くする）
    float edgeDark = (1.0 - vignette) * vignetteIntensity;
    
    // 暗くなる部分にオレンジを混ぜる
    color *= vignette;
    color.r += edgeDark * 0.08; // 赤みを残す
    color.g += edgeDark * 0.02; // 微かなオレンジ
    
    // ========================================
    // 4) ヒートブルーム（下部のオレンジグロー）
    // ========================================
    color += heatBloom(uv, t);
    
    // ========================================
    // 5) エンバーオーバーレイ（FX層の火の粉）
    // ========================================
    //float3 embers = emberOverlay(uv, t);
    //color += embers * lightningIntensity;
    
    // ========================================
    // 6) スキャンライン（金属・無骨な質感）
    // ========================================
    float scan = scanlines(uv, t);
    color += float3(0.8, 0.4, 0.1) * scan; // オレンジ色のスキャンライン
    
    // ========================================
    // 7) 全体の色調補正（DOOM Eternal風）
    // ========================================
    // 暖色強調
    color.r *= 1.1;
    color.g *= 0.92;
    color.b *= 0.8;
    
    // 微かなコントラスト強調B
    color = pow(color, float3(1.05, 1.05, 1.08));
    
    // 明るい部分のブルーム感
    float lum = dot(color, float3(0.299, 0.587, 0.114));
    color += color * max(lum - 0.6, 0.0) * 0.5;
    
    return float4(saturate(color), a);
}
