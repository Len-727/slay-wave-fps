// ============================================================
//  Game.h ? Gameクラス定義
//
//  Gothic Swarm のメインクラス。
//  DirectX 11デバイスの管理、ゲームループの制御、
//  全サブシステムの統括を担当する。
//
//  【ファイル構成】
//  実装は8つの.cppファイルに分割されている：
//    GameCore.cpp    ? 初期化 / Tick / リソース作成
//    GamePlay.cpp    ? ゲームプレイロジック
//    GameRender.cpp  ? 3D描画パイプライン
//    GameScene.cpp   ? シーン管理（タイトル/GO/ランキング）
//    GameHUD.cpp     ? HUD描画
//    GamePhysics.cpp ? Bullet Physics統合
//    GameSlice.cpp   ? メッシュ切断 / 肉片物理
//    GameDebug.cpp   ? デバッグUI（ImGui）
// ============================================================
#pragma once

// -----------------------------------------------
//  標準ライブラリ
// -----------------------------------------------
#include <windows.h>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <string>
#include <algorithm>
#include <stdexcept>

// -----------------------------------------------
//  DirectX 11
// -----------------------------------------------
#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl/client.h>

// -----------------------------------------------
//  DirectXTK
// -----------------------------------------------
#include <CommonStates.h>
#include <Effects.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <GeometricPrimitive.h>
#include <SpriteBatch.h>
#include <SpriteFont.h>
#include <SimpleMath.h>
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

// -----------------------------------------------
//  Effekseer（パーティクルエフェクト）
// -----------------------------------------------
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

// -----------------------------------------------
//  Dear ImGui（デバッグUI）
// -----------------------------------------------
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

// -----------------------------------------------
//  Bullet Physics（物理演算）
// -----------------------------------------------
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

// -----------------------------------------------
//  プロジェクト内ヘッダー
// -----------------------------------------------
#include "Entities.h"
#include "InstanceData.h"
#include "TitleScene.h"
#include "Player.h"
#include "WeaponSystem.h"
#include "EnemySystem.h"
#include "WaveManager.h"
#include "StyleRankSystem.h"
#include "RankingSystem.h"
#include "UISystem.h"
#include "Model.h"
#include "MapSystem.h"
#include "NavGrid.h"
#include "MeshSlicer.h"
#include "WeaponSpawn.h"
#include "ParticleSystem.h"
#include "BloodSystem.h"
#include "GPUParticleSystem.h"
#include "Shadow.h"
#include "FurRenderer.h"
#include "StunRing.h"
#include "KeyPrompt.h"
#include "TargetMarker.h"


// ============================================================
//  Gameクラス
// ============================================================
class Game
{
public:
    Game() noexcept;
    ~Game();

    // コピー禁止（DirectXリソースは複製不可）
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // --- ライフサイクル ---
    void Initialize(HWND window, int width, int height);
    void Tick();
    void OnWindowSizeChanged(int width, int height);
    void ResetGame();

private:

    // =========================================================
    //  内部構造体・列挙型（メソッド宣言より前に定義が必要）
    // =========================================================

    // レイキャスト結果
    struct RaycastResult
    {
        bool hit = false;
        DirectX::XMFLOAT3 hitPoint = { 0, 0, 0 };
        DirectX::XMFLOAT3 hitNormal = { 0, 0, 0 };
        Enemy* hitEnemy = nullptr;
    };

    // 切断ピース（盾投げで敵を切った後の破片）
    struct SlicedPiece
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount = 0;
        DirectX::XMFLOAT3 position = { 0, 0, 0 };
        DirectX::XMFLOAT3 velocity = { 0, 0, 0 };
        DirectX::XMFLOAT3 rotation = { 0, 0, 0 };
        DirectX::XMFLOAT3 angularVel = { 0, 0, 0 };
        float lifetime = 5.0f;
        bool active = false;
        ID3D11ShaderResourceView* texture = nullptr;
    };

    // 肉片（敵死亡時に飛び散るキューブ）
    struct Gib
    {
        btRigidBody* body = nullptr;
        btCollisionShape* shape = nullptr;
        float lifetime = 0.0f;
        float size = 0.0f;
        DirectX::XMFLOAT4 color = { 1, 0, 0, 1 };
        bool hasLanded = false;
        DirectX::XMFLOAT3 finalPos = { 0, 0, 0 };
        DirectX::XMFLOAT4 finalRot = { 0, 0, 0, 1 };
    };

    // 弾道トレーサー（射撃時の光る軌跡）
    struct BulletTrace
    {
        DirectX::XMFLOAT3 start;
        DirectX::XMFLOAT3 end;
        float lifetime;
        float maxLifetime;
        DirectX::XMFLOAT4 color;
        float width;
    };

    // ブラーパラメータ（ポストプロセス用）
    struct BlurParams
    {
        DirectX::XMFLOAT2 texelSize;
        float blurStrength;
        float focalDepth;
    };

    // 盾の状態
    enum class ShieldState {
        Idle,       // 通常
        Parrying,   // パリィ受付中
        Guarding,   // ガード中
        Charging,   // チャージ中
        Throwing,   // 盾投擲中
        Broken      // 盾破損
    };

    // =========================================================
    //  メソッド宣言（実装ファイル別に整理）
    // =========================================================

    // --- GameCore.cpp: エンジンコア ---
    void Update();
    void Render();
    void Clear();
    void CreateDevice();
    void CreateResources();
    void CreateRenderResources();
    void CreateBlurResources();
    ID3D11VertexShader* LoadVertexShader(const wchar_t* filename);
    ID3D11PixelShader* LoadPixelShader(const wchar_t* filename);
    void DrawFullscreenQuad();

    // --- GamePlay.cpp: ゲームプレイロジック ---
    void UpdatePlaying();
    void UpdatePlayerMovement(float deltaTime);
    void UpdateShooting(float deltaTime);
    void UpdateAmmoAndReload(float deltaTime);
    void UpdateEnemyAI(float deltaTime);
    void UpdateWaveAndEffects(float deltaTime);
    void UpdateShieldSystem(float deltaTime);
    void UpdateBossAttacks(float deltaTime);
    void UpdateGloryKillAnimation(float deltaTime);
    void PerformMeleeAttack();
    void SpawnHealthPickup();
    void UpdateHealthPickups(float deltaTime);
    void DrawHealthPickups(DirectX::XMMATRIX view, DirectX::XMMATRIX proj);
    void SpawnScorePopup(int points, ScorePopupType type);
    void UpdateScorePopups(float deltaTime);
    void DrawScorePopups();

    // --- GameRender.cpp: 3D描画パイプライン ---
    void RenderPlaying();
    void DrawEnemies(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix, bool skipGloryKillTarget = false);
    void DrawKeyPrompt(DirectX::XMMATRIX view, DirectX::XMMATRIX proj);
    void DrawSingleEnemy(const Enemy& enemy, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix);
    float CheckRayIntersection(DirectX::XMFLOAT3 rayStart, DirectX::XMFLOAT3 rayDir,
        DirectX::XMFLOAT3 enemyPos, float enemyRotationY, EnemyType enemyType);
    void DrawBillboard();
    void DrawParticles();
    void DrawWeapon();
    void DrawShield();
    void DrawWeaponSpawns();
    void DrawUI();

    // --- GameScene.cpp: シーン管理 ---
    void UpdateTitle();
    void UpdateLoading();
    void UpdateGameOver();
    void UpdateFade();
    void UpdateRanking();
    void RenderTitle();
    void RenderLoading();
    void RenderGameOver();
    void RenderFade();
    void RenderRanking();

    // --- GameHUD.cpp: HUD描画 ---
    void RenderDamageFlash();
    void RenderLowHealthVignette();
    void RenderSpeedLines();
    void RenderDashOverlay();
    void RenderReloadWarning();
    void RenderWaveBanner();
    void RenderScoreHUD();
    void RenderClawDamage();
    void RenderGloryKillFlash();

    // --- GamePhysics.cpp: 物理演算 ---
    void InitPhysics();
    void CleanupPhysics();
    void UpdatePhysics(float deltaTime);
    RaycastResult RaycastPhysics(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 direction, float maxDistance);
    void AddEnemyPhysicsBody(Enemy& enemy);
    void UpdateEnemyPhysicsBody(Enemy& enemy);
    void RemoveEnemyPhysicsBody(int enemyID);
    bool CheckMeshCollision(DirectX::XMFLOAT3 position, float radius);
    float GetMeshFloorHeight(float x, float z, float defaultY = 0.0f);
    float GetFloorHeightBelow(float x, float startY, float z);
    void BuildNavGrid();
    void DrawNavGridDebug(DirectX::XMMATRIX view, DirectX::XMMATRIX proj);
    void TestMeshSlice();

    // --- GameSlice.cpp: メッシュ切断 ---
    void InitSliceRendering();
    SlicedPiece CreateSlicedPiece(const std::vector<SliceVertex>& vertices,
        const std::vector<uint32_t>& indices, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 velocity);
    void ExecuteSliceTest();
    void SliceEnemyWithShield(Enemy& enemy, DirectX::XMFLOAT3 shieldDir);
    void UpdateSlicedPieces(float deltaTime);
    void DrawSlicedPieces(DirectX::XMMATRIX view, DirectX::XMMATRIX proj);
    void SpawnGibs(DirectX::XMFLOAT3 position, int count, float power);
    void UpdateGibs(float deltaTime);
    void DrawGibs(DirectX::XMMATRIX view, DirectX::XMMATRIX proj);

    // --- GameDebug.cpp: デバッグUI ---
    void InitImGui();
    void ShutdownImGui();
    void DrawDebugUI();
    void DrawHitboxes();
    void DrawBulletTracers();
    void DrawCapsule(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        const DirectX::XMFLOAT3& center, float radius, float cylinderHeight,
        const DirectX::XMFLOAT4& color);

    // =========================================================
    //  メンバ変数（カテゴリ別に整理）
    // =========================================================

    // --- DirectX 11 デバイス ---
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>      m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>           m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_depthStencilView;

    // --- ウィンドウ ---
    HWND m_window = nullptr;
    int  m_outputWidth = 0;
    int  m_outputHeight = 0;

    // --- 描画ヘルパー ---
    std::unique_ptr<DirectX::CommonStates>       m_states;
    std::unique_ptr<DirectX::BasicEffect>        m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>    m_inputLayout;
    std::unique_ptr<DirectX::GeometricPrimitive> m_cube;
    std::unique_ptr<DirectX::SpriteBatch>        m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>         m_font;
    std::unique_ptr<DirectX::SpriteFont>         m_fontLarge;


    // --- ゲームシステム ---
    std::unique_ptr<Player>          m_player;
    std::unique_ptr<WeaponSystem>    m_weaponSystem;
    std::unique_ptr<EnemySystem>     m_enemySystem;
    std::unique_ptr<WaveManager>     m_waveManager;
    std::unique_ptr<StyleRankSystem> m_styleRank;
    std::unique_ptr<UISystem>        m_uiSystem;
    std::unique_ptr<RankingSystem>   m_rankingSystem;
    std::unique_ptr<ParticleSystem>  m_particleSystem;
    std::unique_ptr<BloodSystem>     m_bloodSystem;
    std::unique_ptr<GPUParticleSystem> m_gpuParticles;
    std::unique_ptr<MapSystem>       m_mapSystem;
    std::unique_ptr<Shadow>          m_shadow;
    static constexpr float MAP_SCALE = 0.4f;  // マップの大きさ（1.0 = 原寸）
    std::unique_ptr<FurRenderer>     m_furRenderer;
    std::unique_ptr<WeaponSpawnSystem> m_weaponSpawnSystem;
    std::unique_ptr<StunRing>        m_stunRing;
    std::unique_ptr<KeyPrompt>       m_keyPrompt;
    std::unique_ptr<TargetMarker>    m_targetMarker;
    std::unique_ptr<TitleScene>      m_titleScene;
    bool m_enemiesInitialized = false;
    bool m_furReady = false;

    // --- 3Dモデル ---
    std::unique_ptr<Model> m_weaponModel;          // 武器
    std::unique_ptr<Model> m_shieldModel;           // 盾
    std::unique_ptr<Model> m_enemyModel;            // NORMAL
    std::unique_ptr<Model> m_runnerModel;           // RUNNER
    std::unique_ptr<Model> m_tankModel;             // TANK
    std::unique_ptr<Model> m_midBossModel;          // MIDBOSS
    std::unique_ptr<Model> m_bossModel;             // BOSS
    std::unique_ptr<Model> m_testModel;             // テスト用
    std::unique_ptr<Model> m_pistolModel;           // 壁武器: ピストル
    std::unique_ptr<Model> m_shotgunModel;          // 壁武器: ショットガン
    std::unique_ptr<Model> m_rifleModel;            // 壁武器: ライフル
    std::unique_ptr<Model> m_sniperModel;           // 壁武器: スナイパー
    std::unique_ptr<Model> m_gloryKillArmModel;     // グローリーキル: 腕
    std::unique_ptr<Model> m_gloryKillKnifeModel;   // グローリーキル: ナイフ
    WeaponSpawn* m_nearbyWeaponSpawn = nullptr;     // 近くの壁武器（非所有ポインタ）

    // --- タイミング ---
    LARGE_INTEGER m_lastFrameTime = {};
    LARGE_INTEGER m_performanceFrequency = {};
    float m_deltaTime = 0.0f;
    float m_gameTime = 0.0f;
    float m_timeScale = 1.0f;          // スローモーション倍率
    float m_slowMoTimer = 0.0f;
    float m_accumulatedAnimTime = 0.0f;
    float m_fpsTimer = 0.0f;
    int   m_frameCount = 0;
    float m_currentFPS = 0.0f;
    float m_typeWalkTimer[5] = {};     // タイプ別アニメタイマー
    float m_typeAttackTimer[5] = {};

    // --- ゲーム状態 ---
    GameState m_gameState = GameState::TITLE;
    float m_fadeAlpha = 0.0f;
    bool  m_fadingIn = false;
    bool  m_fadeActive = false;
    int   m_score = 0;

    // --- 武器表示 ---
    float m_weaponSwayX = 0.0f;
    float m_weaponSwayY = 0.0f;
    float m_lastCameraRotX = 0.0f;
    float m_lastCameraRotY = 0.0f;
    float m_weaponScale = 0.2f;
    float m_weaponTiltX = -0.1f;
    float m_weaponRotX = 1.47f;
    float m_weaponRotY = 1.57f;
    float m_weaponRotZ = -0.1f;
    float m_weaponOffsetRight = 0.1f;
    float m_weaponOffsetUp = -0.09f;
    float m_weaponOffsetForward = 0.16f;
    float m_weaponBobTimer = 0.0f;
    float m_weaponBobSpeed = 10.0f;
    float m_weaponBobStrength = 0.02f;
    float m_weaponBobAmount = 0.0f;
    float m_weaponLandingImpact = 0.0f;
    float m_prevWeaponBobSin = 0.0f;
    bool  m_isPlayerMoving = false;

    // --- 射撃リコイル ---
    float m_weaponKickBack = 0.0f;
    float m_weaponKickUp = 0.0f;
    float m_weaponKickRot = 0.0f;
    float m_cameraRecoilX = 0.0f;
    float m_cameraRecoilVelocity = 0.0f;
    bool  m_showMuzzleFlash = false;
    float m_muzzleFlashTimer = 0.0f;
    bool  m_mouseClicked = false;
    bool  m_lastMouseState = false;

    // --- カメラエフェクト ---
    float m_cameraShake = 0.0f;
    float m_cameraShakeTimer = 0.0f;
    float m_hitStopTimer = 0.0f;
    float m_currentFOV = 70.0f;
    float m_targetFOV = 70.0f;

    float m_speedLineAlpha = 0.0f;
    float m_damageFlashAlpha = 0.0f;

    // --- ビルボード / ダメージ表示 ---
    bool  m_showDamageDisplay = false;
    float m_damageDisplayTimer = 0.0f;
    DirectX::XMFLOAT3 m_damageDisplayPos = {};
    int   m_damageValue = 0;


    // --- グローリーキル ---
    int   m_gloryKillTargetID = -1;
    float m_gloryKillRange = 0.0f;
    bool  m_gloryKillActive = false;
    float m_gloryKillInvincibleTimer = 0.0f;
    float m_gloryKillFlashTimer = 0.0f;
    float m_gloryKillFlashAlpha = 0.0f;
    Enemy* m_gloryKillTargetEnemy = nullptr;
    bool  m_gloryKillCameraActive = false;
    DirectX::XMFLOAT3 m_gloryKillCameraPos = {};
    DirectX::XMFLOAT3 m_gloryKillCameraTarget = {};
    float m_gloryKillCameraLerpTime = 0.0f;
    bool  m_gloryKillArmAnimActive = false;
    float m_gloryKillArmAnimTime = 0.0f;
    DirectX::XMFLOAT3 m_gloryKillArmPos = {};
    DirectX::XMFLOAT3 m_gloryKillArmRot = {};
    static constexpr float GLORY_KILL_ANIM_DURATION = 1.2f;

    // グローリーキルモデル調整（デバッグ用）
    bool  m_debugShowGloryKillArm = false;
    float m_gloryKillArmScale = 0.010f;
    DirectX::XMFLOAT3 m_gloryKillArmOffset = { 0.48f, -0.238f, 0.217f };
    DirectX::XMFLOAT3 m_gloryKillArmBaseRot = { 0.392f, 1.472f, 0.0f };
    DirectX::XMFLOAT3 m_gloryKillArmAnimRot = { 0.0f, 0.0f, 0.0f };
    float m_gloryKillKnifeScale = 0.01f;
    DirectX::XMFLOAT3 m_gloryKillKnifeOffset = { 0.265f, 0.099f, -0.041f };
    DirectX::XMFLOAT3 m_gloryKillKnifeBaseRot = { -0.196f, -0.098f, -0.229f };
    DirectX::XMFLOAT3 m_gloryKillKnifeAnimRot = { 0.0f, -0.098f, -0.196f };
    std::unique_ptr<DirectX::GeometricPrimitive> m_gloryKillArm;
    std::unique_ptr<DirectX::GeometricPrimitive> m_gloryKillKnife;

    // --- シールドシステム ---
    ShieldState m_shieldState = ShieldState::Idle;
    float m_parryWindowTimer = 0.0f;
    float m_parryWindowDuration = 0.15f;
    float m_parryCooldown = 0.0f;   // パリィクールダウン残り時間
    static constexpr float PARRY_COOLDOWN = 0.4f;   // パリィ後の再パリィ不可時間
    bool  m_parrySuccess = false;
    float m_parryFlashTimer = 0.0f;
    bool  m_isGuarding = false;
    float m_guardDamageReduction = 0.7f;
    float m_shieldHP = 100.0f;
    float m_shieldMaxHP = 100.0f;
    float m_shieldRegenRate = 15.0f;
    float m_shieldRegenDelay = 1.5f;
    float m_shieldRegenDelayTimer = 0.0f;
    float m_shieldBrokenTimer = 0.0f;
    float m_shieldBrokenDuration = 3.0f;
    float m_shieldBashTimer = 0.0f;
    float m_shieldBashDuration = 0.5f;
    float m_shieldGuardBlend = 0.0f;
    float m_shieldDisplayHP = 100.0f;
    float m_shieldGlowIntensity = 0.0f;
    int   m_guardLockTargetID = -1;
    DirectX::XMFLOAT3 m_guardLockTargetPos = {};

    // シールドチャージ（突進攻撃）
    float m_chargeTimer = 0.0f;
    float m_chargeDuration = 0.5f;
    float m_chargeSpeed = 30.0f;
    DirectX::XMFLOAT3 m_chargeDirection = {};
    DirectX::XMFLOAT3 m_chargeTarget = {};
    float m_chargeDamage = 150.0f;
    float m_chargeKnockback = 25.0f;
    float m_chargeHitRadius = 3.0f;
    float m_chargeEnergy = 0.0f;
    float m_chargeEnergyRate = 0.7f;
    bool  m_chargeReady = false;
    bool  m_chargeHasTarget = false;
    int   m_chargeTargetEnemyID = -1;

    // シールドスロー（盾投げ）
    DirectX::XMFLOAT3 m_thrownShieldPos = {};
    DirectX::XMFLOAT3 m_thrownShieldDir = {};
    float m_thrownShieldDist = 0.0f;
    float m_thrownShieldMaxDist = 30.0f;
    float m_thrownShieldSpeed = 40.0f;
    bool  m_thrownShieldReturning = false;
    float m_thrownShieldDamage = 80.0f;
    float m_thrownShieldHitRadius = 2.0f;
    float m_thrownShieldSpin = 0.0f;
    std::set<int> m_thrownShieldHitEnemies;
    std::set<int> m_thrownShieldReturnHitEnemies;

    // 近接チャージ（Dark Ages式）
    int   m_meleeCharges = 3;
    int   m_meleeMaxCharges = 3;
    float m_meleeRechargeTimer = 0.0f;
    float m_meleeRechargeTime = 5.0f;
    int   m_meleeAmmoRefill = 5;


    // --- ボス攻撃パラメータ ---
    float m_slamRadiusBoss = 8.0f;
    float m_slamRadiusMidBoss = 6.0f;
    float m_slamDamageBoss = 20.0f;
    float m_slamDamageMidBoss = 15.0f;
    float m_slamStunDamage = 60.0f;
    float m_slashSpeed = 15.0f;
    float m_slashDamage = 30.0f;
    float m_slashHitRadius = 2.0f;
    float m_slashStunOnParry = 50.0f;
    float m_beamWidth = 2.0f;
    float m_beamLength = 20.0f;
    float m_beamDPS = 15.0f;
    float m_beamStunOnParry = 30.0f;
    float m_jumpWindupTime = 0.5f;
    float m_jumpAirTime = 0.6f;
    float m_slamRecoveryTime = 1.5f;
    float m_slashWindupTime = 0.8f;
    float m_slashRecoveryTime = 1.0f;
    float m_roarWindupTime = 2.5f;
    float m_roarFireTime = 3.0f;
    float m_roarRecoveryTime = 2.0f;
    float m_bossAttackCooldownBase = 3.0f;
    std::vector<BossProjectile> m_bossProjectiles;

    // --- ダッシュ演出 ---
    bool  m_isSprinting = false;
    float m_dashOverlayAlpha = 0.0f;

    // --- 弾薬 / リロード ---
    float m_reloadWarningTimer = 0.0f;
    float m_reloadWarningAlpha = 0.0f;
    float m_reloadAnimProgress = 0.0f;
    float m_reloadAnimOffset = 0.0f;
    float m_reloadAnimTilt = 0.0f;

    // --- ウェーブバナー ---
    int   m_lastWaveNumber = 0;
    float m_waveBannerTimer = 0.0f;
    float m_waveBannerDuration = 2.5f;
    int   m_waveBannerNumber = 0;
    bool  m_waveBannerIsBoss = false;

    // --- スコアHUD ---
    float m_scoreDisplayValue = 0.0f;
    float m_scoreFlashTimer = 0.0f;
    float m_scoreShakeTimer = 0.0f;
    int   m_lastDisplayedScore = 0;

    // --- 爪痕ダメージ ---
    float m_clawTimer = 0.0f;
    float m_clawDuration = 0.6f;

    // --- ボスHPバー ---
    float m_bossHpDisplay = 1.0f;
    float m_bossHpTrail = 1.0f;

    // --- ローディング画面 ---
    float m_loadingTimer = 0.0f;
    float m_loadingDuration = 3.5f;
    int   m_loadingPhase = 0;
    float m_loadingBarTarget = 0.0f;
    float m_loadingBarCurrent = 0.0f;

    // --- ゲームオーバー演出 ---
    float m_gameOverTimer = 0.0f;
    int   m_gameOverWave = 0;
    int   m_gameOverScore = 0;
    float m_gameOverCountUp = 0.0f;
    int   m_gameOverRank = 0;           // 0=C, 1=B, 2=A, 3=S
    float m_gameOverNoiseT = 0.0f;

    // --- パリィデバッグ ---
    float m_lastParryAttemptTime = 0.0f;
    float m_lastParryResultTime = 0.0f;
    bool  m_lastParryWasSuccess = false;
    int   m_parrySuccessCount = 0;
    int   m_parryFailCount = 0;

    // --- プレイ中スタッツ ---
    int   m_statKills = 0;
    int   m_statHeadshots = 0;
    int   m_statMeleeKills = 0;
    int   m_statMaxCombo = 0;
    int   m_statDamageDealt = 0;
    int   m_statDamageTaken = 0;
    int   m_statMaxStyleRank = 0;
    float m_statSurvivalTime = 0.0f;

    // --- ゲームオーバー時のスタッツ保存 ---
    int   m_goKills = 0;
    int   m_goHeadshots = 0;
    int   m_goMeleeKills = 0;
    int   m_goMaxCombo = 0;
    int   m_goParryCount = 0;
    int   m_goDamageDealt = 0;
    int   m_goDamageTaken = 0;
    int   m_goMaxStyleRank = 0;
    float m_goSurvivalTime = 0.0f;
    int   m_goStatScores[9] = {};
    int   m_goTotalScore = 0;
    float m_goStatCountUp[9] = {};

    // --- ランキング ---
    float m_rankingTimer = 0.0f;
    int   m_newRecordRank = -1;
    bool  m_rankingSaved = false;

    // --- 名前入力 ---
    bool  m_nameInputActive = false;
    char  m_nameBuffer[16] = {};
    int   m_nameLength = 0;
    float m_nameKeyTimer = 0.0f;
    bool  m_nameKeyWasDown = false;


    // --- Effekseer ---
    Effekseer::ManagerRef m_effekseerManager;
    EffekseerRendererDX11::RendererRef m_effekseerRenderer;
    Effekseer::EffectRef m_effectShieldTrail;
    Effekseer::Handle m_shieldTrailHandle = -1;
    float m_shieldTrailSpawnTimer = 0.0f;
    Effekseer::EffectRef m_effectSlashRed;
    Effekseer::EffectRef m_effectSlashGreen;
    Effekseer::EffectRef m_effectGroundSlam;
    Effekseer::EffectRef m_effectBeamRed;
    Effekseer::EffectRef m_effectBeamGreen;
    Effekseer::Handle m_beamHandle = -1;
    Effekseer::EffectRef m_effectGunFire;
    Effekseer::EffectRef m_effectAttackImpact;
    Effekseer::EffectRef m_effectBossSpawn;
    Effekseer::EffectRef m_effectEnemySpawn;
    Effekseer::EffectRef m_effectParry;

    // --- テクスチャ ---
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_armDiffuseTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_dashSpeedlineSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_waveBannerNormalSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_waveBannerBossSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreBackdropSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_clawDamageSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_lowHealthVignetteSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bloodParticleSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whitePixel;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_meleeIconTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rankTextures[7];
    bool m_rankTexturesLoaded = false;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_comboDigitTex[10];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_comboLabelTex;
    bool m_comboTexturesLoaded = false;

    // シールドHUDテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudFrame;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudFillBlue;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudFillDanger;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudFillGuard;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudGlow;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudCrack;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudParryFlash;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shieldHudIcon;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hpHudFrame;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hpHudFillGreen;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hpHudFillCritical;
    bool m_shieldHudLoaded = false;

    // スコアポップアップテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreGlow;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreGlowRed;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreGlowBlue;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreBurst;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreSkull;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_scoreHeadshot;

    // --- ポストプロセス ---
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_offscreenTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_offscreenRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_offscreenSRV;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_sceneCopyTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneCopySRV;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_fullscreenVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_blurPS;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_blurConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_linearSampler;
    bool  m_gloryKillDOFActive = false;
    float m_gloryKillDOFIntensity = 0.0f;
    float m_gloryKillFocalDepth = 0.0f;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthSRV;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_offscreenDepthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_offscreenDepthStencilView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_offscreenDepthSRV;

    // 切断描画用
    std::unique_ptr<DirectX::BasicEffect> m_sliceEffectTex;
    std::unique_ptr<DirectX::BasicEffect> m_sliceEffectNoTex;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_sliceInputLayoutTex;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_sliceInputLayoutNoTex;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>   m_sliceNoCullRS;

    // パーティクル描画用
    std::unique_ptr<DirectX::BasicEffect>           m_particleEffect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_particleInputLayout;

    // --- Bullet Physics ---
    std::unique_ptr<btDefaultCollisionConfiguration>      m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher>                 m_dispatcher;
    std::unique_ptr<btBroadphaseInterface>                 m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver>   m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld>                m_dynamicsWorld;
    std::map<int, btRigidBody*> m_enemyPhysicsBodies;
    btTriangleMesh* m_mapTriMesh = nullptr;
    btBvhTriangleMeshShape* m_mapMeshShape = nullptr;
    btRigidBody* m_mapMeshBody = nullptr;
    NavGrid m_navGrid;
    bool m_debugDrawNavGrid = false;

    // --- 切断/肉片データ ---
    std::vector<SlicedPiece> m_slicedPieces;
    std::vector<Gib> m_gibs;
    std::vector<HealthPickup> m_healthPickups;
    float m_pickupSpawnTimer = 0.0f;
    float m_pickupSpawnInterval = 12.0f;
    int   m_maxPickups = 5;
    float m_pickupRadius = 1.5f;
    std::vector<ScorePopup> m_scorePopups;

    // --- デバッグ（ImGui） ---
    bool  m_showDebugWindow = false;
    bool  m_showHitboxes = false;
    bool  m_showHeadHitboxes = false;
    bool  m_showBulletTrajectory = false;
    bool  m_showPhysicsHitboxes = false;
    bool  m_useDebugHitboxes = false;
    bool  m_showTutorial = true;
    float m_debugRunnerSpeed = 0.0f;
    float m_debugTankSpeed = 0.0f;
    float m_debugRunnerHP = 0.0f;
    float m_debugTankHP = 0.0f;
    float m_debugHeadRadius = 0.0f;
    float m_rayStartY = 1.8f;
    EnemyTypeConfig m_normalConfigDebug;
    EnemyTypeConfig m_runnerConfigDebug;
    EnemyTypeConfig m_tankConfigDebug;
    EnemyTypeConfig m_midbossConfigDebug;
    EnemyTypeConfig m_bossConfigDebug;
    std::vector<BulletTrace> m_bulletTraces;

    // --- インスタンシング用ワークバッファ ---
    std::vector<DirectX::XMMATRIX>  m_instWorlds;
    std::vector<DirectX::XMVECTOR>  m_instColors;
    std::vector<std::string>        m_instAnims;
    std::vector<float>              m_instTimes;
    std::vector<InstanceData> m_normalDead, m_normalDeadHeadless;
    std::vector<InstanceData> m_runnerDead, m_runnerDeadHeadless;
    std::vector<InstanceData> m_tankAttackingHeadless, m_tankDead, m_tankDeadHeadless;
    std::vector<InstanceData> m_midBossWalking, m_midBossAttacking;
    std::vector<InstanceData> m_midBossWalkingHeadless, m_midBossAttackingHeadless;
    std::vector<InstanceData> m_midBossDead, m_midBossDeadHeadless;
    std::vector<InstanceData> m_bossWalking, m_bossAttackingJump, m_bossAttackingSlash;
    std::vector<InstanceData> m_bossAttackingJumpHeadless, m_bossAttackingSlashHeadless;
    std::vector<InstanceData> m_bossWalkingHeadless, m_bossDead, m_bossDeadHeadless;
};

