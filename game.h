// Game.h - ゲームクラス定義（構造を理解しよう）
#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl/client.h>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>          
#include <algorithm>
#include <CommonStates.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <GeometricPrimitive.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <SimpleMath.h>
#include <SimpleMath.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <WICTextureLoader.h>   // PNG/JPGなど
#include <DDSTextureLoader.h>   // DDSテクスチャ
#include <CommonStates.h>
#include <set>
//#include <Effekseer.h>
//#include <EffekseerRendererDX11.h>

#include "Entities.h"
#include "InstanceData.h"
#include "WeaponSystem.h"
#include "EnemySystem.h"
#include "ParticleSystem.h"
#include "Wavemanager.h"
#include "Player.h"
#include "UISystem.h"
#include "Model.h"
#include "MapSystem.h"
#include "WeaponSpawn.h"
#include "Shadow.h"

class Game
{
public:
    Game() noexcept;
    ~Game() = default;

    // コピー禁止（DirectXリソースは複製不可）
    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // === ゲームの基本メソッド（役割を理解しよう） ===
    void Initialize(HWND window, int width, int height);  // 初期化
    void Tick();                                          // メインループ
    void OnWindowSizeChanged(int width, int height);      // ウィンドウサイズ変更

private:
    //  m_weaponSystem->Update()    みたいに使う
    std::unique_ptr<WeaponSystem> m_weaponSystem;       //  武器管理システム    unique_ptr 自動でメモリ管理

    //  m_enemySystem->Update()     みたいに使う
    std::unique_ptr<EnemySystem> m_enemySystem;         //  敵管理システム      unique_ptr 自動でメモリ管理

    //  m_particleSystem->Update()  みたいに使う
    std::unique_ptr<ParticleSystem> m_particleSystem;   //  パーティクル管理システム    unique_ptr 自動でメモリ管理

    //  m_waveManager->Update() みたいに使う
    std::unique_ptr<WaveManager> m_waveManager;         //  ウェーブ管理システム

    //  m_player->Update()  みたいに使う
    std::unique_ptr<Player> m_player;       //  プレイヤー管理システム

    //  m_uiSystem->Draw...みたいに使う
    std::unique_ptr<UISystem> m_uiSystem;   //  UI管理システム

    std::unique_ptr<Model> m_weaponModel;   //  武器モデル
    std::unique_ptr<Model> m_testModel;     //  テストモデル
    std::unique_ptr<Model> m_enemyModel;    //  敵テストモデル

    std::unique_ptr<MapSystem> m_mapSystem;

    std::unique_ptr<WeaponSpawnSystem> m_weaponSpawnSystem; //  壁武器管理システム
    WeaponSpawn* m_nearbyWeaponSpawn;                       //  プレイヤー近くの壁武器
    //  --- 壁武器用モデル ---
    std::unique_ptr<Model> m_pistolModel;   //  初期武器ピストル
    std::unique_ptr<Model> m_shotgunModel;  //  ショットガンモデル
    std::unique_ptr<Model> m_rifleModel;    //  ライフルモデル
    std::unique_ptr<Model> m_sniperModel;   //  スナイパーモデル


    //  === 影   ===
    std::unique_ptr<Shadow> m_shadow;


    // === ゲームループの内部処理 ===
    void Update();     // ゲームロジック更新
    void Render();     // 描画処理


    void Clear();
    void CreateDevice();
    void CreateResources();
    void CreateRenderResources();

    //void DrawGrid();
    //void DrawCubes();
    //void DrawHealthBar(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch);
    void DrawBillboard();
    void DrawWeapon();
    void DrawWeaponSpawns();
    void DrawParticles();
    void DrawEnemies();
    void DrawUI();
   


    void UpdateTitle();
    void UpdatePlaying();
    void UpdateGameOver();
    void UpdateFade();

    void RenderTitle();
    void RenderPlaying();
    void RenderGameOver();
    void RenderFade();
    void RenderDamageFlash();



    float CheckRayIntersection(DirectX::XMFLOAT3 rayStart,
        DirectX::XMFLOAT3 rayDir,
        DirectX::XMFLOAT3 enemyPos);

    // DirectXデバイス（GPU制御）
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;

    // レンダリングターゲット（描画先）
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // ウィンドウ情報
    HWND    m_window;
    int     m_outputWidth;
    int     m_outputHeight;

    //  3D描画用
    std::unique_ptr<DirectX::CommonStates>      m_states;
    std::unique_ptr<DirectX::BasicEffect>       m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;
    std::unique_ptr<DirectX::GeometricPrimitive> m_cube;


    
    //std::unique_ptr<DirectX::GeometricPrimitive> m_muzzleFlashModel;
    float m_weaponSwayX;
    float m_weaponSwayY;
    float m_lastCameraRotX;
    float m_lastCameraRotY;

    
    
    bool m_showMuzzleFlash;
    float m_muzzleFlashTimer;


    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;


    bool m_mouseClicked;
    bool m_lastMouseState;

    //  キューブ破壊
    //bool m_cubesDestroyed[3];
    int m_score;
    // ビルボード表示用
    bool m_showDamageDisplay;
    float m_damageDisplayTimer;
    DirectX::XMFLOAT3 m_damageDisplayPos;
    int m_damageValue;

   

    GameState m_gameState;
    float m_fadeAlpha;
    bool m_fadingIn;
    bool m_fadeActive;

   

    //  UI用
    std::unique_ptr<DirectX::SpriteFont> m_font;
    std::unique_ptr<DirectX::SpriteFont> m_fontLarge;

    //  FPS計算用
    float m_fpsTimer;
    int m_frameCount;
    float m_currentFPS;

    //  スローモーション
    float m_timeScale;      //  時間の流れる速度
    float m_slowMoTimer;    //  スローモーション残り時間

    //  カメラシェイク
    float m_cameraShake;        //  カメラの揺れ強度
    float m_cameraShakeTimer;   //  揺れの残り時間

    //  ヒットストップ
    float m_hitStopTimer;   //  ヒットストップ残り時間

    //  Effekseerの管理クラス
    //Effekseer::ManagerRef m_effekseerManager;
    //EffekseerRendererDX11::RendererRef m_effekseerRenderer;

    ////  読み込んだエフェクトデータ
    //Effekseer::EffectRef m_effectBlood; //  血のエフェクト用(テスト)
};
