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
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <set>


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
#include "StyleRankSystem.h"
#include "Furrenderer.h"

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

    //  スタイルランクシステム
    std::unique_ptr<StyleRankSystem> m_styleRank;

    //  ===========================
    //  === キャラクターモデル   ===
    //  ===========================
    std::unique_ptr<Model> m_weaponModel;   //  武器モデル
    std::unique_ptr<Model> m_shieldModel;

    std::unique_ptr<Model> m_testModel;     //  テストモデル

    //  ==================
    //  === 敵モデル    ===
    //  ==================
    std::unique_ptr<Model> m_enemyModel;    //  NORMAL用モデル
    std::unique_ptr<Model> m_runnerModel;   //  RUNNER用モデル
    std::unique_ptr<Model> m_tankModel;     //  TANK用モデル
    std::unique_ptr<Model> m_midBossModel;     //  MIDBOSS用モデル
    std::unique_ptr<Model> m_bossModel;


    std::unique_ptr<MapSystem> m_mapSystem;
    std::unique_ptr<FurRenderer> m_furRenderer;
    bool m_furReady= false;

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
    void DrawShield();
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

    //  ランクテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_rankTextures[7];
    bool m_rankTexturesLoaded = false;

    //  コンボテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_comboDigitTex[10]; // 0 - 9
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_comboLabelTex;
    bool m_comboTexturesLoaded = false;

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
    Effekseer::ManagerRef m_effekseerManager;
    EffekseerRendererDX11::RendererRef m_effekseerRenderer;

    Effekseer::EffectRef m_effectShieldTrail;   //  盾トレイルえふぇっくと
    Effekseer::Handle m_shieldTrailHandle = -1; //  再生ハンドル
    float m_shieldTrailSpawnTimer = 0.0f;

    // === ボスエフェクト ===
    Effekseer::EffectRef m_effectSlashRed;       // 赤斬撃（パリィ不可）
    Effekseer::EffectRef m_effectSlashGreen;     // 緑斬撃（パリィ可）
    Effekseer::EffectRef m_effectGroundSlam;     // 地面叩きつけ衝撃波
    Effekseer::EffectRef m_effectBeamRed;    // 赤ビーム（ガードのみ）
    Effekseer::EffectRef m_effectBeamGreen;  // 緑ビーム（パリィ可）
    Effekseer::Handle m_beamHandle = -1;         // ビームのハンドル（継続再生用）

    // === パリィ＆ボス攻撃チューニング ===
    // パリィウィンドウ
    // m_parryWindowDuration は既存（0.15f）→ スライダー化

    // ジャンプ叩きつけ
    float m_slamRadiusBoss = 8.0f;       // BOSS叩き範囲
    float m_slamRadiusMidBoss = 6.0f;    // MIDBOSS叩き範囲
    float m_slamDamageBoss = 40.0f;      // BOSSダメージ
    float m_slamDamageMidBoss = 25.0f;   // MIDBOSSダメージ
    float m_slamStunDamage = 60.0f;      // パリィ時スタン量

    // 斬撃プロジェクタイル
    float m_slashSpeed = 15.0f;          // 弾速
    float m_slashDamage = 30.0f;         // ダメージ
    float m_slashHitRadius = 1.2f;       // 当たり判定幅
    float m_slashStunOnParry = 50.0f;    // パリィ時スタン量

    // ビーム
    float m_beamWidth = 2.0f;            // ビーム幅
    float m_beamLength = 20.0f;          // ビーム長さ
    float m_beamDPS = 15.0f;             // 秒間ダメージ
    float m_beamStunOnParry = 30.0f;     // パリィ時スタン量

    // ボスAIタイミング（秒）
    float m_jumpWindupTime = 0.5f;
    float m_jumpAirTime = 0.6f;
    float m_slamRecoveryTime = 1.5f;
    float m_slashWindupTime = 0.8f;
    float m_slashRecoveryTime = 1.0f;
    float m_roarWindupTime = 2.5f;
    float m_roarFireTime = 3.0f;
    float m_roarRecoveryTime = 2.0f;
    float m_bossAttackCooldownBase = 3.0f;

    // デバッグ表示
    float m_lastParryAttemptTime = 0.0f;   // 最後にパリィ入力した時間
    float m_lastParryResultTime = 0.0f;    // 最後にパリィ判定された時間
    bool  m_lastParryWasSuccess = false;   // 最後のパリィ成功したか
    int   m_parrySuccessCount = 0;         // パリィ成功回数
    int   m_parryFailCount = 0;            // パリィ失敗回数
    float m_gameTime = 0.0f;              // ゲーム経過時間

    ////  読み込んだエフェクトデータ
    //Effekseer::EffectRef m_effectBlood; //  血のエフェクト用(テスト)

    // --- シールドの状態 ---
    enum class ShieldState {
        Idle,       // 盾を下ろしている（通常状態）
        Parrying,   // パリィ受付中（右クリ押した瞬間?短時間）
        Guarding,   // ガード中（右クリ押し続け）
        Charging,   //  チャージ(右クリック中左クリック)
        Throwing,
        Broken      // 盾が壊れた（耐久値ゼロ、一時使用不能）
    };
    ShieldState m_shieldState = ShieldState::Idle;

    // --- パリィ（ジャストガード）---
    float m_parryWindowTimer = 0.0f;        // パリィ受付の残り時間
    float m_parryWindowDuration = 0.15f;    // パリィ受付時間（短い！）
    bool  m_parrySuccess = false;           // パリィ成功フラグ
    float m_parryFlashTimer = 0.0f;         // パリィ成功エフェクトタイマー

    // --- ガード（長押し）---
    bool  m_isGuarding = false;             // 現在ガード中か
    float m_guardDamageReduction = 0.7f;    // ガード時のダメージカット率（70%カット）

    // --- シールド耐久値 ---
    float m_shieldHP = 100.0f;              // 現在の耐久値
    float m_shieldMaxHP = 100.0f;           // 最大耐久値
    float m_shieldRegenRate = 15.0f;        // 毎秒の回復量（非ガード時）
    float m_shieldRegenDelay = 1.5f;        // 被弾後のリジェネ開始までの遅延
    float m_shieldRegenDelayTimer = 0.0f;   // リジェネ遅延タイマー
    float m_shieldBrokenTimer = 0.0f;       // 壊れた後の復帰時間
    float m_shieldBrokenDuration = 3.0f;    // 壊れてから復帰までの秒数

    // --- 盾アニメーション ---
    float m_shieldBashTimer = 0.0f;         // 盾アニメーション進行
    float m_shieldBashDuration = 0.5f;      // 盾アニメーションの長さ
    float m_shieldGuardBlend = 0.0f;        // ガード構えのブレンド値（0=下ろす、1=構え）

    float m_shieldDisplayHP = 100.0f;       // 表示用HP（滑らかに追従）
    float m_shieldGlowIntensity = 0.0f;     // グロウの強さ（0?1）

    // --- シールドチャージ（突進） ---
    float m_chargeTimer = 0.0f;              // チャージ経過時間
    float m_chargeDuration = 0.5f;           // チャージ持続時間（秒）
    float m_chargeSpeed = 30.0f;             // 突進速度（通常移動の約50倍）
    DirectX::XMFLOAT3 m_chargeDirection = { 0,0,0 };  // 突進方向ベクトル
    DirectX::XMFLOAT3 m_chargeTarget = { 0,0,0 };     // ロックオン対象の位置
    float m_chargeDamage = 150.0f;           // チャージダメージ（雑魚即死級）
    float m_chargeKnockback = 25.0f;         // ノックバック力
    float m_chargeHitRadius = 3.0f;          // 突進の当たり判定半径
    //  === チャージエネルギー   ===
    float m_chargeEnergy = 0.0f;    //  現在のエネルギー 0 - 1
    float m_chargeEnergyRate = 0.7f;    //  一秒あたりの田丸亮
    bool m_chargeReady = false; //  満タンフラグ

    //  === チャージ演出  ===
    float m_currentFOV = 70.0f;  //  現在のFOV
    float m_targetFOV = 70.0f;  //  目標FOV
    float m_speedLineAlpha = 0.0f;  //  スピードラインの透明度

    void RenderSpeedLines();    //  スピードライン描画

    bool m_chargeHasTarget = false;          // ロックオン対象がいるか
    int m_chargeTargetEnemyID = -1;          // ロックオン対象の敵ID

    // --- シールドスロー（盾投げ） ---
    DirectX::XMFLOAT3 m_thrownShieldPos = { 0,0,0 };    // 飛んでる盾の位置
    DirectX::XMFLOAT3 m_thrownShieldDir = { 0,0,0 };    // 飛ぶ方向
    float m_thrownShieldDist = 0.0f;                    // 飛んだ距離
    float m_thrownShieldMaxDist = 30.0f;                // 最大飛距離
    float m_thrownShieldSpeed = 40.0f;                  // 飛ぶ速度
    bool  m_thrownShieldReturning = false;              // 戻り中フラグ
    float m_thrownShieldDamage = 80.0f;                 // ヒットダメージ
    float m_thrownShieldHitRadius = 2.0f;               // 当たり判定半径
    float m_thrownShieldSpin = 0.0f;                    // 回転角度（見た目用

    

    std::set<int> m_thrownShieldHitEnemies;             // 行きで当たった敵（重複防止）
    std::set<int> m_thrownShieldReturnHitEnemies;       // 帰りで当たった敵（重複防止）

    //  ボスプロジェクトタイル
    std::vector<BossProjectile> m_bossProjectiles;

    // --- シールドHUDテクスチャ ---
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
    float m_rayStartY = 1.8f;   //  球の出る位置

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
    EnemyTypeConfig m_midbossConfigDebug;
    EnemyTypeConfig m_bossConfigDebug;
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

    // === 肉片物理 ===
    struct Gib
    {
        btRigidBody* body;          // Bullet剛体
        btCollisionShape* shape;    // 形状（解放用に保持）
        float lifetime;             // 残り寿命（秒）
        float size;                 // サイズ（スケール）
        DirectX::XMFLOAT4 color;   // 色（肉片の色）
        bool hasLanded = false;
    };
    std::vector<Gib> m_gibs;                            // 全肉片

    void SpawnGibs(DirectX::XMFLOAT3 position, int count, float power);  // 肉片生成
    void UpdateGibs(float deltaTime);                                     // 肉片更新
    void DrawGibs(DirectX::XMMATRIX view, DirectX::XMMATRIX proj);      // 肉片描画

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
