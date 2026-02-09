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
#include "CommonStates.h"
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
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>


#include "TitleScene.h"
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
    ~Game();

    // コピー禁止（DirectXリソースは複製不可）
    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // === ゲームの基本メソッド（役割を理解しよう） ===
    void Initialize(HWND window, int width, int height);  // 初期化
    void Tick();                                          // メインループ
    void OnWindowSizeChanged(int width, int height);      // ウィンドウサイズ変更

private:

    std::unique_ptr<TitleScene> m_titleScene;

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

    //  ===========================
    //  === キャラクターモデル   ===
    //  ===========================
    std::unique_ptr<Model> m_weaponModel;   //  武器モデル
    std::unique_ptr<Model> m_testModel;     //  テストモデル

    //  ==================
    //  === 敵モデル    ===
    //  ==================
    std::unique_ptr<Model> m_enemyModel;    //  NORMAL用モデル
    std::unique_ptr<Model> m_runnerModel;   //  RUNNER用モデル
    std::unique_ptr<Model> m_tankModel;     //  TANK用モデル


    std::unique_ptr<MapSystem> m_mapSystem;

    std::unique_ptr<WeaponSpawnSystem> m_weaponSpawnSystem; //  壁武器管理システム
    WeaponSpawn* m_nearbyWeaponSpawn;                       //  プレイヤー近くの壁武器
    //  --- 壁武器用モデル ---
    std::unique_ptr<Model> m_pistolModel;   //  初期武器ピストル
    std::unique_ptr<Model> m_shotgunModel;  //  ショットガンモデル
    std::unique_ptr<Model> m_rifleModel;    //  ライフルモデル
    std::unique_ptr<Model> m_sniperModel;   //  スナイパーモデル

    //  --- グローリーキル用手とナイフのモデル
    std::unique_ptr<Model> m_gloryKillArmModel;
    std::unique_ptr<Model> m_gloryKillKnifeModel;

    //  --- テクスチャ   ---
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_armDiffuseTexture;


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
    void DrawEnemies(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix, bool skipGloryKillTarget = false);
    void DrawSingleEnemy(const Enemy& enemy, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix);
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
    void RenderGloryKillFlash();



    float CheckRayIntersection(
        DirectX::XMFLOAT3 rayStart,
        DirectX::XMFLOAT3 rayDir,
        DirectX::XMFLOAT3 enemyPos,
        float enemyRotationY,
        EnemyType enemyType
    );

    void UpdateGloryKillAnimation(float deltaTime);

    void PerformMeleeAttack();  //  近接攻撃を実行

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

    //  deltaTime計測用
    LARGE_INTEGER m_lastFrameTime;
    LARGE_INTEGER m_performanceFrequency;
    float m_deltaTime;

    //  スローモーション
    float m_timeScale;      //  時間の流れる速度
    float m_slowMoTimer;    //  スローモーション残り時間

    //  アニメーション累積時間
    float m_accumulatedAnimTime;

    int m_gloryKillTargetID;
    float m_gloryKillRange;

    //  グローリーキル演出用
    bool m_gloryKillActive;           // グローリーキル実行中
    float m_gloryKillInvincibleTimer; // 無敵時間タイマー
    float m_gloryKillFlashTimer;      // 画面フラッシュタイマー
    float m_gloryKillFlashAlpha;      // フラッシュの透明度
    Enemy* m_gloryKillTargetEnemy;    // ターゲット敵へのポインタ

    //  グローリーキルカメラ用
    bool m_gloryKillCameraActive;   //  グローリーキルカメラ有効
    DirectX::XMFLOAT3 m_gloryKillCameraPos; //  カメラ位置
    DirectX::XMFLOAT3 m_gloryKillCameraTarget;  //  カメラターゲット
    float m_gloryKillCameraLerpTime;    //  カメラ補間時間


    // グローリーキル腕・ナイフアニメーション用
    bool m_gloryKillArmAnimActive;       // アニメーション再生中
    float m_gloryKillArmAnimTime;        // アニメーション時間（0.0～1.0）
    DirectX::XMFLOAT3 m_gloryKillArmPos; // 腕の位置
    DirectX::XMFLOAT3 m_gloryKillArmRot; // 腕の回転

    // === グローリーキルモデル調整用（デバッグ） ===
    bool m_debugShowGloryKillArm = false;

    float m_gloryKillArmScale = 0.010f;      // スケール
    DirectX::XMFLOAT3 m_gloryKillArmOffset = { 0.48f, -0.238f, 0.217f };  // 前方, 上下, 左右

    // ベース回転（モデルを正しい向きに補正）
    DirectX::XMFLOAT3 m_gloryKillArmBaseRot = { 0.392f, 1.472f, 0.0f };
    // アニメーション回転（グローリーキル動作で変化）
    DirectX::XMFLOAT3 m_gloryKillArmAnimRot = { 0.0f, 0.0f, 0.0f };

    float m_gloryKillKnifeScale = 0.01f;
    DirectX::XMFLOAT3 m_gloryKillKnifeOffset = { 0.265f, 0.099f, -0.041f };
    DirectX::XMFLOAT3 m_gloryKillKnifeBaseRot = { -0.196f, -0.098f, -0.229f };
    DirectX::XMFLOAT3 m_gloryKillKnifeAnimRot = { 0.0f, -0.098f, -0.196f };

    // グローリーキルアニメーション時間設定
    static constexpr float GLORY_KILL_ANIM_DURATION = 1.2f;  // 全体の時間（秒）

    // グローリーキル腕・ナイフモデル（プリミティブ）
    std::unique_ptr<DirectX::GeometricPrimitive> m_gloryKillArm;   // 腕（円柱）
    std::unique_ptr<DirectX::GeometricPrimitive> m_gloryKillKnife; // ナイフ（円錐）

    // === ガウシアンブラー用（ポストプロセス） ===
    // オフスクリーンレンダーターゲット
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_offscreenTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_offscreenRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_offscreenSRV;

    // ブラーシェーダー
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_fullscreenVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_blurPS;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_blurConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_linearSampler;

    // ブラーパラメータ構造体
    struct BlurParams
    {
        DirectX::XMFLOAT2 texelSize;
        float blurStrength;
        float focalDepth;
    };

    //  描画会深度(ぼかし用)
    bool m_gloryKillDOFActive; //  描写会深度有効
    float m_gloryKillDOFIntensity;  //  ぼかし強度
    float m_gloryKillFocalDepth;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthSRV;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_offscreenDepthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_offscreenDepthStencilView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_offscreenDepthSRV;

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


    //  ==========================
    //  === ImGui用  ===
    //  ==========================
    // 
    //  デバッグ用
    bool m_showDebugWindow;     //  デバッグウィンドウ表示
    bool m_showHitboxes;        //  当たり判定表示
    bool m_showHeadHitboxes;    //  頭の当たり判定を表示
    bool m_showBulletTrajectory; //  弾の軌跡表示
    bool m_showPhysicsHitboxes;

    //  === リアルタイム調整用変数 ===
    float m_debugRunnerSpeed;   //  Runner速度調整用
    float m_debugTankSpeed;      //  Tank速度調整用
    float m_debugRunnerHP;       //  RunnerHP調整用
    float m_debugTankHP;         //  TankHP調整用
    float m_debugHeadRadius;     //  頭の判定サイズ調整用

    //  === 弾の軌跡保存用 ===
    struct BulletTrace
    {
        DirectX::XMFLOAT3 start;
        DirectX::XMFLOAT3 end;
        float lifetime;
    };
    std::vector<BulletTrace> m_bulletTraces;

    


    //  === ImGui 用変数   ===
    void InitImGui();
    void ShutdownImGui();
    void DrawDebugUI();
    void DrawHitboxes();
    void DrawCapsule(
        DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        const DirectX::XMFLOAT3& center,
        float radius,
        float cylinderHeight,
        const DirectX::XMFLOAT4& color);

    //  === リアルタイム調整用のコンフィグ ===
    EnemyTypeConfig
    m_normalConfigDebug;
    EnemyTypeConfig m_runnerConfigDebug;
    EnemyTypeConfig m_tankConfigDebug;
    bool m_useDebugHitboxes;  // デバッグ値を使うかどうか


    //  === Bullet Physics構造体   ===
    struct RaycastResult
    {
        bool hit;                       //  ヒットしたか
        DirectX::XMFLOAT3 hitPoint;     //  ヒット位置
        DirectX::XMFLOAT3 hitNormal;    //  ヒット法線
        Enemy* hitEnemy;                //  ヒットした敵(nullptr = 壁)
    };

    //  === Bullet Physics ===
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
    std::map<int, btRigidBody*> m_enemyPhysicsBodies;

    void InitPhysics();
    void CleanupPhysics();
    void UpdatePhysics(float deltaTime);
    RaycastResult RaycastPhysics(
        DirectX::XMFLOAT3 start,
        DirectX::XMFLOAT3 direction,
        float maxDistance
    );
    void AddEnemyPhysicsBody(Enemy& enemy);
    void UpdateEnemyPhysicsBody(Enemy& enemy);
    void RemoveEnemyPhysicsBody(int enemyID);

    // === ポストプロセス用ヘルパー関数 ===
    void CreateBlurResources();        // ブラー用リソース作成
    void DrawFullscreenQuad();         // フルスクリーン四角形描画
    ID3D11VertexShader* LoadVertexShader(const wchar_t* filename);
    ID3D11PixelShader* LoadPixelShader(const wchar_t* filename);
};
