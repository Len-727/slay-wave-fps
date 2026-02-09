// ========================================
// TitleBackgroundPS.hlsl 
// ========================================

SamplerState samplerState : register(s0);

cbuffer TitleBGParams : register(b0)
{
    float time; // 経過時間（秒）
    float screenWidth; // 画面幅
    float screenHeight; // 画面高さ
    float intensity; // 全体の強さ（フェードイン用、0→1）
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// ========================================
// ユーティリティ関数
// ========================================

float hash21(float2 p)
{
    p = frac(p * float2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return frac(p.x * p.y);
}

float2 hash22(float2 p)
{
    float x = hash21(p);
    float y = hash21(p + 127.1);
    return float2(x, y);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);
    
    float a = hash21(i);
    float b = hash21(i + float2(1, 0));
    float c = hash21(i + float2(0, 1));
    float d = hash21(i + float2(1, 1));
    
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// フラクタルノイズ（7オクターブ＝超ディテール）
float fbm(float2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < 7; i++)
    {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// ドメインワーピング（ノイズでUVをねじ曲げる→有機的なうねり）
float warpedFbm(float2 uv, float t)
{
    float2 q = float2(
        fbm(uv + float2(0.0, 0.0) + t * 0.06),
        fbm(uv + float2(5.2, 1.3) + t * 0.07)
    );
    float2 r = float2(
        fbm(uv + 4.0 * q + float2(1.7, 9.2) + t * 0.05),
        fbm(uv + 4.0 * q + float2(8.3, 2.8) + t * 0.06)
    );
    return fbm(uv + 4.0 * r);
}

// ========================================
// 溶岩の亀裂（太く明るい版）
// ========================================
float lavaCracks(float2 uv, float t, float scale)
{
    float2 p = uv * scale;
    
    float minDist = 1.0;
    float secondDist = 1.0;
    
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float2 cell = float2(float(x), float(y));
            float2 cellId = floor(p) + cell;
            
            // ゆっくり蠢く亀裂（溶岩の流動感）
            float2 cellCenter = cell + float2(
                hash21(cellId) * 0.8 + sin(t * 0.2 + hash21(cellId) * 6.28) * 0.12,
                hash21(cellId + 100.0) * 0.8 + cos(t * 0.18 + hash21(cellId + 100.0) * 6.28) * 0.12
            );
            
            float dist = length(frac(p) - cellCenter);
            
            if (dist < minDist)
            {
                secondDist = minDist;
                minDist = dist;
            }
            else if (dist < secondDist)
            {
                secondDist = dist;
            }
        }
    }
    
    // 亀裂を太く（0.12 = かなり太い）
    float crack = 1.0 - smoothstep(0.0, 0.12, secondDist - minDist);
    
    // 亀裂のコアをさらに明るく
    float core = 1.0 - smoothstep(0.0, 0.03, secondDist - minDist);
    crack += core * 0.5;
    
    return saturate(crack);
}

// ========================================
// マグマの脈動（ドクドク光る溶岩）
// ========================================
float magmaPulse(float2 uv, float t)
{
    // ドメインワーピングでうねる溶岩
    float warp = warpedFbm(uv * 2.5, t);
    
    // 複数の脈動周波数を重ねる（心臓のドクドク感）
    float pulse1 = sin(t * 1.2 + warp * 8.0) * 0.5 + 0.5;
    float pulse2 = sin(t * 0.7 + warp * 5.0 + 2.0) * 0.5 + 0.5;
    float pulse3 = sin(t * 2.0 + warp * 3.0 + 4.0) * 0.5 + 0.5;
    
    // 合成してパワフルな脈動に
    float pulse = pulse1 * 0.5 + pulse2 * 0.3 + pulse3 * 0.2;
    pulse = pow(pulse, 1.5);
    
    return pulse * warp;
}

// ========================================
// エンバー（火の粉）v3 ノイズベース完全リライト
// グリッド構造を完全排除、有機的な散布
// ========================================

// 単一レイヤーの火の粉
// ノイズを高い閾値で切り取って「光点」にする方式
// セルベースではないので四角くならない
float3 emberLayer(float2 uv, float t, float scrollSpeed, float noiseScale,
                  float threshold, float brightness, float3 tintA, float3 tintB)
{
    // UVをスクロール（上昇＋ゆるい横揺れ）
    float2 scrollUV = uv * noiseScale;
    scrollUV.y -= t * scrollSpeed;
    scrollUV.x += sin(t * 0.4 + uv.y * 3.0) * 0.3;

    // ノイズ値を取得（fbmで有機的な模様）
    float n = fbm(scrollUV);

    // 閾値で切り取り：光る点だけ残す
    float spark = saturate((n - threshold) / (1.0 - threshold));

    // べき乗でシャープに（中心が明るく端が急速に暗くなる）
    spark = pow(spark, 3.0);

    // 色バリエーション（ノイズから派生）
    float colorMix = frac(n * 7.31 + scrollUV.x * 0.1);
    float3 sparkColor = lerp(tintA, tintB, colorMix);

    // 白熱コア（特に明るい部分は白っぽく）
    float coreHeat = saturate((n - (threshold + 0.03)) / 0.02);
    coreHeat = pow(coreHeat, 2.0);
    sparkColor = lerp(sparkColor, float3(1.0, 0.95, 0.8), coreHeat * 0.7);

    // 高さフェード（画面下部ほど火の粉が多い）
    float heightFade = smoothstep(0.0, 0.8, uv.y);

    // チカチカ（時間で明滅）
    float flicker = 0.7 + 0.3 * sin(t * 8.0 + n * 50.0);

    return sparkColor * spark * brightness * heightFade * flicker;
}

// 火の粉メイン（8レイヤー重ね）
float3 embers(float2 uv, float t)
{
    float3 result = float3(0, 0, 0);

    // 色パレット
    float3 redOrange = float3(1.0, 0.3, 0.02);
    float3 yellowOrange = float3(1.0, 0.65, 0.1);
    float3 brightYellow = float3(1.0, 0.85, 0.3);
    float3 deepRed = float3(0.9, 0.15, 0.0);

    // 大粒（近景）: ゆっくり上昇、明るい
    result += emberLayer(uv, t,
        0.15, 8.0, 0.72, 0.6,
        redOrange, yellowOrange);

    // 中粒レイヤー1
    result += emberLayer(uv, t + 10.0,
        0.25, 14.0, 0.75, 0.5,
        yellowOrange, brightYellow);

    // 中粒レイヤー2（ずらし）
    result += emberLayer(uv + float2(3.7, 1.2), t + 20.0,
        0.20, 16.0, 0.76, 0.45,
        deepRed, redOrange);

    // 小粒レイヤー1: 速い上昇
    result += emberLayer(uv + float2(7.1, 5.3), t + 30.0,
        0.4, 24.0, 0.78, 0.4,
        yellowOrange, brightYellow);

    // 小粒レイヤー2
    result += emberLayer(uv + float2(13.5, 8.9), t + 40.0,
        0.35, 28.0, 0.79, 0.35,
        redOrange, yellowOrange);

    // 微粒子レイヤー1: 速くて細かい
    result += emberLayer(uv + float2(21.0, 3.4), t + 50.0,
        0.55, 40.0, 0.80, 0.3,
        brightYellow, float3(1.0, 0.95, 0.7));

    // 微粒子レイヤー2
    result += emberLayer(uv + float2(31.7, 11.2), t + 60.0,
        0.50, 45.0, 0.81, 0.25,
        yellowOrange, brightYellow);

    // 超微粒子（遠景のキラキラ）
    result += emberLayer(uv + float2(47.3, 19.8), t + 70.0,
        0.7, 60.0, 0.83, 0.2,
        brightYellow, float3(1.0, 1.0, 0.9));

    return result;
}

// ========================================
// 炎柱（下から立ち上がる炎の帯）
// ========================================
float fireColumns(float2 uv, float t)
{
    float result = 0.0;
    
    for (int i = 0; i < 4; i++)
    {
        float offset = float(i) * 0.25 + 0.1;
        
        // 炎柱のX位置（ゆっくり揺れる）
        float colX = offset + sin(t * 0.3 + float(i) * 2.0) * 0.08;
        
        // X方向の距離（炎の幅）
        float distX = abs(uv.x - colX);
        float width = 0.03 + sin(t * 1.5 + float(i) * 3.0 + uv.y * 8.0) * 0.015;
        float column = 1.0 - smoothstep(0.0, width, distX);
        
        // Y方向の減衰（下から上に弱くなる）
        float yFade = smoothstep(1.0, 0.3, uv.y);
        
        // ノイズで炎をゆらゆらさせる
        float fireNoise = noise(float2(uv.x * 10.0, uv.y * 5.0 - t * 3.0 + float(i) * 10.0));
        column *= fireNoise;
        
        result += column * yFade;
    }
    
    return saturate(result);
}

// ========================================
// メイン関数
// ========================================
float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.texCoord;
    float t = time;
    
    // ========================================
    // 0) 熱歪み（UVを少し歪ませる→画面全体がゆらぐ）
    // ========================================
    float2 distortion;
    distortion.x = noise(uv * 5.0 + float2(t * 0.5, 0.0)) - 0.5;
    distortion.y = noise(uv * 5.0 + float2(0.0, t * 0.4)) - 0.5;
    float2 warpedUV = uv + distortion * 0.008;
    
    // ========================================
    // 1) ベースの暗い溶岩地面
    // ========================================
    float metalNoise = fbm(warpedUV * 8.0 + 5.0);
    float darkPattern = warpedFbm(warpedUV * 1.5, t * 0.5);
    
    float3 base = float3(0.035, 0.02, 0.015);
    base += float3(0.025, 0.015, 0.008) * metalNoise;
    base += float3(0.02, 0.008, 0.003) * darkPattern;
    
    // ========================================
    // 2) 溶岩の亀裂（2レイヤー＝太い＋細い）
    // ========================================
    float crack1 = lavaCracks(warpedUV, t, 3.5);
    float crack2 = lavaCracks(warpedUV, t * 1.3 + 100.0, 6.0);
    float totalCrack = saturate(crack1 + crack2 * 0.5);
    
    // マグマの脈動
    float pulse = magmaPulse(warpedUV, t);
    
    // 溶岩の色（暗い赤 → 眩しいオレンジ黄色）
    float3 lavaDark = float3(0.7, 0.1, 0.0);
    float3 lavaMid = float3(1.0, 0.4, 0.05);
    float3 lavaBright = float3(1.0, 0.8, 0.3);
    
    float lavaIntensity = 0.7 + pulse * 0.5;
    float3 lavaColor = lerp(lavaDark, lavaMid, pulse);
    lavaColor = lerp(lavaColor, lavaBright, totalCrack * pulse * 0.5);
    
    base += lavaColor * totalCrack * lavaIntensity;
    
    // 亀裂周辺のにじみ（ブルーム風）
    float bloom = lavaCracks(warpedUV * 0.95 + 0.025, t, 3.0);
    base += float3(0.2, 0.05, 0.0) * bloom * pulse * 0.4;
    
    // ========================================
    // 3) 炎柱（下から立ち上がる激しい炎）
    // ========================================
    float columns = fireColumns(warpedUV, t);
    
    float3 fireColor = lerp(
        float3(0.8, 0.2, 0.0),
        float3(1.0, 0.7, 0.2),
        1.0 - warpedUV.y
    );
    base += fireColor * columns * 0.8;
    
    // ========================================
    // 4) 下部のヒートグロー（激しい炉の光）
    // ========================================
    float bottomHeat = smoothstep(1.0, 0.35, uv.y);
    float heatNoise = fbm(float2(uv.x * 8.0, t * 0.2));
    float heatPulse = sin(t * 1.0) * 0.15 + 0.85;
    bottomHeat *= (0.6 + heatNoise * 0.4) * heatPulse;
    
    float3 heatColor = float3(0.35, 0.08, 0.01);
    base += heatColor * bottomHeat;
    
    // 最下部の溶岩プール
    float lavaPool = smoothstep(1.0, 0.85, uv.y);
    float poolNoise = fbm(float2(uv.x * 4.0 + t * 0.3, t * 0.15));
    base += float3(0.4, 0.12, 0.02) * lavaPool * poolNoise;
    
    // ========================================
    // 5) 煙＋灰（上部）
    // ========================================
    float smoke = fbm(uv * 3.0 + float2(t * 0.04, -t * 0.03));
    float smoke2 = fbm(uv * 2.0 + float2(-t * 0.03, t * 0.05) + 20.0);
    float combinedSmoke = (smoke + smoke2) * 0.5;
    
    float smokeMask = smoothstep(0.5, 0.0, uv.y);
    float3 smokeColor = float3(0.05, 0.035, 0.025);
    smokeColor += float3(0.05, 0.02, 0.005) * combinedSmoke;
    base += smokeColor * combinedSmoke * smokeMask * 0.6;
    
    // ========================================
    // 6) エンバー（火の粉）ノイズベース v3
    // ========================================
    float3 emberResult = embers(uv, t);
    base += emberResult;
    
    // ========================================
    // 7) 大きな炎のフラッシュ（ランダムに一瞬明るく）
    // ========================================
    float flash = hash21(float2(floor(t * 0.8), 42.0));
    flash = pow(max(flash - 0.92, 0.0) * 12.5, 2.0);
    base += float3(0.15, 0.05, 0.01) * flash;
    
    // ========================================
    // 8) ビネット（下部は除外して炉の光を保つ）
    // ========================================
    float2 center = uv - float2(0.5, 0.45);
    float vignette = 1.0 - dot(center, center) * 1.6;
    vignette = saturate(vignette);
    vignette = lerp(vignette, 1.0, smoothstep(0.7, 1.0, uv.y) * 0.5);
    base *= vignette;
    
    // ========================================
    // 9) 色調補正（DOOM Eternal風の攻撃的な暖色）
    // ========================================
    base.r *= 1.2;
    base.g *= 0.85;
    base.b *= 0.6;
    
    // コントラストをガツンと上げる
    base = pow(base, float3(1.1, 1.15, 1.2));
    
    // 明るい部分をさらに強調（HDR風ブルーム感）B
    float luminance = dot(base, float3(0.299, 0.587, 0.114));
    base += base * max(luminance - 0.5, 0.0) * 0.8;
    
    // フェードイン
    base *= intensity;
    
    return float4(saturate(base), 1.0);
}
