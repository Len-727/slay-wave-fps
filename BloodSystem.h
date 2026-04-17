// ============================================================
//  BloodSystem.h
//  血しぶき＆血デカール管理 (カスタムHLSLシェーダー対応版)
// ============================================================
#pragma once

#include "Entities.h"
#include <vector>
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>

class BloodSystem
{
public:
    BloodSystem();
    ~BloodSystem() = default;

    void Initialize(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        DirectX::CommonStates* states,
        ID3D11ShaderResourceView* bloodParticleSRV,
        DirectX::BasicEffect* particleEffect,
        ID3D11InputLayout* particleInputLayout);

    void Update(float deltaTime);

    void DrawScreenBlood(int screenWidth, int screenHeight);
    void DrawBloodDecals(
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX proj,
        DirectX::XMFLOAT3 cameraPos);

    // === トリガー ===
    void OnEnemyKilled(DirectX::XMFLOAT3 enemyPos, DirectX::XMFLOAT3 playerPos, float maxRange = 8.0f);
    void OnGloryKill(DirectX::XMFLOAT3 enemyPos);
    void OnExplosionKill(DirectX::XMFLOAT3 enemyPos, DirectX::XMFLOAT3 playerPos);
    void OnMeleeKill(DirectX::XMFLOAT3 enemyPos);

private:
    // スクリーンブラッド
    void SpawnScreenBlood(int count, float intensity);
    void UpdateScreenBlood(float deltaTime);
    std::vector<ScreenBlood> m_screenBloods;

    // 床デカール
    void SpawnBloodDecal(DirectX::XMFLOAT3 position, float size);
    void UpdateBloodDecals(float deltaTime);
    std::vector<BloodDecal> m_bloodDecals;
    static const int MAX_BLOOD_DECALS = 200;

    // カスタムシェーダー(自前で所有)
    Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_bloodVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_bloodPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_bloodIL;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_bloodCB;
    bool m_hasCustomShader = false;

    // 定数バッファ構造体(HLSLのcbufferと完全一致)
    struct alignas(16) BloodCBData
    {
        DirectX::XMFLOAT4X4 WorldViewProj;   // 64 bytes
        DirectX::XMFLOAT3   CameraPos;       // 12 bytes
        float                Time;            // 4  bytes
        DirectX::XMFLOAT3   LightDir;        // 12 bytes
        float                ScreenMode;      // 4  bytes (0=3D, 1=2D)
    };
    float m_totalTime = 0.0f;
    float m_lastScreenBloodTime = -1.0f;    //  最後に血を生成した時刻(連続キル演出用)

    // 描画リソース(Gameから借りる)
    ID3D11DeviceContext* m_context = nullptr;
    DirectX::CommonStates* m_states = nullptr;
    ID3D11ShaderResourceView* m_bloodSRV = nullptr;
    DirectX::BasicEffect* m_particleEffect = nullptr;
    ID3D11InputLayout* m_particleInputLayout = nullptr;

    // 描画バッチ
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColorTexture>> m_batch;

    // シェーダーコンパイル
    bool CompileShaders(ID3D11Device* device);

    // 定数バッファ更新
    void SetupBloodCB(DirectX::XMMATRIX wvp, DirectX::XMFLOAT3 camPos,
        DirectX::XMFLOAT3 lightDir, float screenMode);
};