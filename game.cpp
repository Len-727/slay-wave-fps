// Game.cpp - 実装（基盤部分）
#include "Game.h"
#include <string>
//#include <stdexcept>
//#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// DirectXTK用
#pragma comment(lib, "DirectXTK.lib")


using namespace DirectX;
using Microsoft::WRL::ComPtr;

Game::Game() noexcept :
    m_window(nullptr),
    m_outputWidth(1280),
    m_outputHeight(720),

    m_cube(nullptr),
    m_lastMouseState(false),
    m_score(0),
    m_damageDisplayTimer(0.0f),
    m_showDamageDisplay(false),
    m_damageValue(0),
    m_gameState(GameState::TITLE),
    m_fadeAlpha(0.0f),
    m_fadingIn(false),
    m_fadeActive(false),
    m_weaponSwayX(0.0f),
    m_weaponSwayY(0.0f),
    m_lastCameraRotX(0.0f),
    m_lastCameraRotY(0.0f),
    m_showMuzzleFlash(false),
    m_muzzleFlashTimer(0.0f),
    m_weaponSystem(std::make_unique<WeaponSystem>()),
    m_enemySystem(std::make_unique<EnemySystem>()),
    m_particleSystem(std::make_unique<ParticleSystem>()),
    m_waveManager(std::make_unique<WaveManager>()),
    m_player(std::make_unique<Player>()),
    m_uiSystem(std::make_unique<UISystem>(1280, 720)),
    m_shadow(nullptr)
{
   
}


void Game::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;

    // DirectX初期化（複雑なのでコピペOK）
    CreateDevice();
    CreateResources();

    CreateRenderResources();  // 3D描画用の初期化
}

void Game::Tick()
{
    // ゲームの1フレーム処理
    Update();  // ゲームロジック（自分で書く部分）
    Render();  // 描画処理
}

void Game::Update()
{

    switch (m_gameState)
    {
    case GameState::TITLE:
        UpdateTitle();
        break;

    case GameState::PLAYING:
        UpdatePlaying();
        break;

    case GameState::GAMEOVER:
        UpdateGameOver();
        break;
    }

    UpdateFade();





}

void Game::Clear()
{
    // 画面を青でクリア
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Blue);

    // ★重要: 深度バッファ(DEPTH) と ステンシルバッファ(STENCIL) の両方をクリアする
    m_d3dContext->ClearDepthStencilView(
        m_depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, // 両方のフラグを立てる
        1.0f, // Depth初期値 (1.0 = 最奥)
        0     // Stencil初期値 (0 = 履歴なし)
    );

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
        m_depthStencilView.Get());

    CD3D11_VIEWPORT viewport(0.0f, 0.0f,
        static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = (width > 1) ? width : 1;
    m_outputHeight = (height > 1) ? height : 1;
    CreateResources();
    m_uiSystem->OnScreenSizeChanged(m_outputWidth, m_outputHeight);
}


void Game::CreateDevice()
{
    UINT creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;  // デバッグ情報有効
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,  // DirectX 11.1
        D3D_FEATURE_LEVEL_11_0,  // DirectX 11.0
        D3D_FEATURE_LEVEL_10_1,  // DirectX 10.1（フォールバック）
        D3D_FEATURE_LEVEL_10_0,  // DirectX 10.0（フォールバック）
    };

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // デフォルトアダプター
        D3D_DRIVER_TYPE_HARDWARE,   // ハードウェア（GPU）使用
        nullptr,
        creationFlags,
        featureLevels,
        4,
        D3D11_SDK_VERSION,
        device.GetAddressOf(),
        nullptr,
        context.GetAddressOf()
    );

    if (FAILED(hr))
        throw std::runtime_error("D3D11CreateDevice failed");

    device.As(&m_d3dDevice);
    context.As(&m_d3dContext);
}

void Game::CreateResources()
{
    // リソースクリア
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(1, nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);

    if (m_swapChain)
    {
        // スワップチェーンリサイズ
        HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight,
            DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        if (FAILED(hr)) return;
    }
    else
    {
        // スワップチェーン作成
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = backBufferWidth;
        swapChainDesc.BufferDesc.Height = backBufferHeight;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = m_window;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;

        ComPtr<IDXGIDevice> dxgiDevice;
        m_d3dDevice.As(&dxgiDevice);

        ComPtr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());

        ComPtr<IDXGIFactory> dxgiFactory;
        dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));

        HRESULT hr = dxgiFactory->CreateSwapChain(m_d3dDevice.Get(),
            &swapChainDesc, m_swapChain.GetAddressOf());
        if (FAILED(hr))
            throw std::runtime_error("CreateSwapChain failed");
    }

    // レンダーターゲット作成
    ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (FAILED(hr)) throw std::runtime_error("GetBuffer failed");

    hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr,
        m_renderTargetView.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateRenderTargetView failed");

    // 深度バッファ作成
    CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,
        backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateTexture2D failed");

    hr = m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), nullptr,
        m_depthStencilView.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("CreateDepthStencilView failed");
}

void Game::CreateRenderResources()
{
    // DirectXTKの基本オブジェクトを作成
    m_states = std::make_unique<DirectX::CommonStates>(m_d3dDevice.Get());
    m_effect = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());

    // エフェクトの設定
    m_effect->SetVertexColorEnabled(true);  // 頂点カラーを使用

    // 入力レイアウトの作成（GPU用の設定）
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    HRESULT hr = m_d3dDevice->CreateInputLayout(
        DirectX::VertexPositionColor::InputElements,
        DirectX::VertexPositionColor::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_inputLayout.GetAddressOf()
    );

    if (FAILED(hr))
        throw std::runtime_error("CreateInputLayout failed");

    m_cube = DirectX::GeometricPrimitive::CreateCube(m_d3dContext.Get());

    m_weaponModel = std::make_unique<Model>();
    if (!m_weaponModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/PISTOL.fbx"))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to load weapon model (M1911)\n");
    }
    else
    {
        OutputDebugStringA("Game::CreateRenderResources - Weapon model (M1911) loaded successfully!\n");
    }

    //  壁武器用モデル読み込み
    m_pistolModel = std::make_unique<Model>();
    if (!m_pistolModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Gun/PISTOL.fbx"))
    {
        OutputDebugStringA("Failed to load M1911 model for wall weapon\n");
    }


    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_d3dContext.Get());

  /*  m_testModel = std::make_unique<Model>();*/

    ////  プレイヤーモデルファイルを読み込む
    //if (!m_testModel->LoadFromFile(m_d3dDevice.Get(), "Assets/X_Bot.fbx"))
    //{
    //    OutputDebugStringA("Game::CreateRenderResources - Failed to load model!\n");
    //}
    //else
    //{
    //    OutputDebugStringA("Game::CreateRenderResources - Model loaded successfully!\n");
    //}

    //  敵プレイヤーモデル
    m_enemyModel = std::make_unique<Model>();
    if (!m_enemyModel->LoadFromFile(m_d3dDevice.Get(), "Assets/Models/Y_Bot/Y_Bot.fbx"))
    {
        OutputDebugStringA("Failed to load enemy model!\n");
        //throw std::runtime_error("Failed to load enemy model");
    }
    else
    {
        OutputDebugStringA("Enemy model loaded successfully!\n");
    }

    OutputDebugStringA("=== Loading animations  ===\n");

    //  --- 歩行アニメーション   ---
    if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Walk.fbx", "Walk"))
    {
        OutputDebugStringA("Failed to load Walk animation\n");
    }

    //  --- 待機アニメーション   ---
    if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Idle.fbx", "Idle"))
    {
        OutputDebugStringA("Failed to load Idle animation\n");
    }

    //  --- 走りアニメーション   ---
    if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Run.fbx", "Run"))
    {
        OutputDebugStringA("Failed to load Run animation\n");
    }

    //  --- 攻撃アニメーション   ---
    if (!m_enemyModel->LoadAnimation("Assets/Models/Y_Bot/Zombie_Attack.fbx", "Attack"))
    {
        OutputDebugStringA("Failed to load Attack animation");
    }


    //  === MapSystem   初期化 ===
    m_mapSystem = std::make_unique<MapSystem>();
    if (!m_mapSystem->Initialize(m_d3dContext.Get()))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to initialize MapSystem\n");
    }
    else
    {
        m_mapSystem->CreateDefaultMap();
        OutputDebugStringA("Game::CreateRenderResources - MapSystem initialized successfully\n");
    }


    //  === WeaponSystem 初期化    2025/11/14  ===
    m_weaponSpawnSystem = std::make_unique<WeaponSpawnSystem>();
    m_weaponSpawnSystem->CreateDefaultSpawns();

    //  --- テクスチャ読み込み   2025/11/19  ---
    if (!m_weaponSpawnSystem->InitializeTextures(m_d3dDevice.Get(), m_d3dContext.Get()))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to initialize weapon textures\n");
    }

    m_nearbyWeaponSpawn = nullptr;

    OutputDebugStringA("Game::CreateRenderResources - WeaponSpawnSystem initialized\n");
    // フォントファイルを読み込む
    //m_font = std::make_unique<DirectX::SpriteFont>(m_d3dDevice.Get(), L"Arial.spritefont");

    //  === Shadow を初期化  ===
    m_shadow = std::make_unique<Shadow>();
    if (!m_shadow->Initialize(m_d3dDevice.Get()))
    {
        OutputDebugStringA("Game::CreateRenderResources - Failed to initialize Shadow System!\n");
        // エラー処理
    }
    else
    {
        // 影の色を黒の半透明に設定
        m_shadow->SetShadowColor(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f));
    }

}




//void Game::DrawGrid()
//{
//    // 3D描画の準備
//    auto context = m_d3dContext.Get();
//
//    // ビュー行列（カメラ位置・向き）の作成
//    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
//    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
//
//    // カメラが向いている方向を計算
//    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
//        m_player->GetPosition().x + sinf(m_player->GetRotation().y) * cosf(m_player->GetRotation().x),
//        m_player->GetPosition().y - sinf(m_player->GetRotation().x),
//        m_player->GetPosition().z + cosf(m_player->GetRotation().y) * cosf(m_player->GetRotation().x),
//        0.0f
//    );
//
//    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);
//
//    // プロジェクション行列（透視投影）
//    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
//    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
//        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
//    );
//
//    // エフェクトに行列を設定
//    m_effect->SetView(viewMatrix);
//    m_effect->SetProjection(projectionMatrix);
//    m_effect->SetWorld(DirectX::XMMatrixIdentity());
//
//    // 描画開始
//    m_effect->Apply(context);
//    context->IASetInputLayout(m_inputLayout.Get());
//
//    // プリミティブバッチで線描画
//    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
//    primitiveBatch->Begin();
//
//    // グリッド線の描画
//    DirectX::XMFLOAT4 gridColor(0.5f, 0.5f, 0.5f, 1.0f); // グレー
//
//    // Z方向の線（手前から奥へ）
//    for (int x = -10; x <= 10; x += 2)
//    {
//        primitiveBatch->DrawLine(
//            DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, 0, -10), gridColor),
//            DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, 0, 10), gridColor)
//        );
//    }
//
//    // X方向の線（左から右へ）
//    for (int z = -10; z <= 10; z += 2)
//    {
//        primitiveBatch->DrawLine(
//            DirectX::VertexPositionColor(DirectX::XMFLOAT3(-10, 0, z), gridColor),
//            DirectX::VertexPositionColor(DirectX::XMFLOAT3(10, 0, z), gridColor)
//        );
//    }
//
//    primitiveBatch->End();
//}


void Game::DrawEnemies()
{
    //	========================================================================
    //	ビュー・プロジェクション行列の計算
    //	========================================================================
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);

    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    //	========================================================================
    //	デバッグ出力（1秒ごと）
    //	========================================================================
    static int frameCount = 0;
    frameCount++;

    if (frameCount % 60 == 0)  // 1秒ごと
    {
        auto& enemies = m_enemySystem->GetEnemies();



        char debugMsg[512];
        sprintf_s(debugMsg, "=== Enemies: %zu ===\n", enemies.size());
        OutputDebugStringA(debugMsg);

        for (size_t i = 0; i < enemies.size(); i++)
        {
            const auto& enemy = enemies[i];
            sprintf_s(debugMsg, "Enemy[%zu]: alive=%d, pos=(%.1f,%.1f,%.1f), rot=%.2f, anim=%s, time=%.2f\n",
                i, enemy.isAlive,
                enemy.position.x, enemy.position.y, enemy.position.z,
                enemy.rotationY,
                enemy.currentAnimation.c_str(),
                enemy.animationTime);
            OutputDebugStringA(debugMsg);
        }
    }


    DirectX::XMFLOAT3 lightDir = { 1.0f, -1.0f, 1.0f };

    //	========================================================================
    //	敵を描画（敵モデル）
    //	========================================================================
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive)
            continue;

        //	====================================================================
        //	ワールド行列（スケール→回転→位置）
        //	====================================================================

        //	---	スケール	---
        //	Y_Bot は 1/100 サイズに縮小
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);

        //	---	回転	---
        //	enemy.rotationY をそのまま使う（EnemySystem で計算済み）
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);

        //	---	位置	---
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
            enemy.position.x,
            enemy.position.y,
            enemy.position.z
        );

        //	---	合成	---
        //	【重要】余計な回転を追加しない
        DirectX::XMMATRIX world = scale * rotation * translation;

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(&enemy.color);

        //  === 影描画 ===
        // 【光の方向】デフォルトのライトと同じ方向 (斜め上から)
        if (m_shadow)
        {
            // 新しい RenderShadow は引数が 7個 あります
            m_shadow->RenderShadow(
                m_d3dContext.Get(),
                enemy.position,     // 敵の足元の位置
                1.5f,               // 影のサイズ
                viewMatrix,         // ビュー行列
                projectionMatrix,   // プロジェクション行列
                lightDir,           // ★追加: 光の方向
                1.1f                // ★追加: 地面の高さ (Y=0)
            );
        }
        //if (m_shadow)
        //{
        //    // ★修正1：影用に「位置だけの行列」を作る
        //    // 敵の回転(rotation)や縮小(scale)を無視し、敵の足元(translation)に影を置く
        //    DirectX::XMMATRIX shadowPos = DirectX::XMMatrixTranslation(
        //        enemy.position.x,
        //        enemy.position.y,
        //        enemy.position.z
        //    );

        //    // ※もしShadow.cpp側でスケール調整をしていないなら、ここで影の大きさを決める
        //    // float shadowSize = 0.5f; // 影の大きさ（50cmくらい）
        //    // shadowPos = XMMatrixScaling(shadowSize, 1.0f, shadowSize) * shadowPos;

        //    m_shadow->RenderShadow(
        //        m_d3dContext.Get(),
        //        shadowPos,  // ← world ではなく、位置だけの行列を渡す
        //        viewMatrix,
        //        projectionMatrix,
        //        lightDir,
        //        0.01f       // ← ★修正2：プラスの値（地面より少し上）にする
        //    );
        //}

        //	====================================================================
        //	モデル描画
        //	====================================================================
        if (m_enemyModel)
        {
            if (m_enemyModel->HasAnimation(enemy.currentAnimation))
            {
                //	アニメーション付き描画
                m_enemyModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world,
                    viewMatrix,
                    projectionMatrix,
                    color,
                    enemy.currentAnimation,
                    enemy.animationTime
                );
            }
            else
            {
                //	静的モデル描画
                m_enemyModel->Draw(
                    m_d3dContext.Get(),
                    world,
                    viewMatrix,
                    projectionMatrix,
                    color
                );
            }
        }
        else
        {
            //	フォールバック：キューブで代用
            m_cube->Draw(world, viewMatrix, projectionMatrix, color);
        }
    }

    //	========================================================================
    //	HPバーを描画
    //	========================================================================
    auto context = m_d3dContext.Get();
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.health >= enemy.maxHealth)
            continue;

        //	HPバーの位置（敵の上）
        float barWidth = 1.0f;
        float barHeight = 0.1f;
        float healthPercent = (float)enemy.health / enemy.maxHealth;

        DirectX::XMFLOAT3 barCenter = enemy.position;
        barCenter.y += 2.5f;  // 敵の頭上

        //	背景（暗い赤）
        DirectX::XMFLOAT4 bgColor(0.5f, 0.0f, 0.0f, 1.0f);
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x - barWidth / 2, barCenter.y, barCenter.z),
                bgColor
            ),
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x + barWidth / 2, barCenter.y, barCenter.z),
                bgColor
            )
        );

        //	HP部分（明るい赤）
        DirectX::XMFLOAT4 hpColor(1.0f, 0.0f, 0.0f, 1.0f);
        float currentBarWidth = barWidth * healthPercent;
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x - barWidth / 2, barCenter.y + 0.05f, barCenter.z),
                hpColor
            ),
            DirectX::VertexPositionColor(
                DirectX::XMFLOAT3(barCenter.x - barWidth / 2 + currentBarWidth, barCenter.y + 0.05f, barCenter.z),
                hpColor
            )
        );
    }

    primitiveBatch->End();
}

float Game::CheckRayIntersection(DirectX::XMFLOAT3 rayStart,
    DirectX::XMFLOAT3 rayDir,
    DirectX::XMFLOAT3 enemyPos)
{
    float width = 0.6f;     // 幅(肩幅くらい)
    float height = 1.0f;    // 高さ

    float minX = enemyPos.x - width / 2.0f;
    float minY = enemyPos.y;
    float minZ = enemyPos.z - width / 2.0f;

    float maxX = enemyPos.x + width / 2.0f;
    float maxY = enemyPos.y + height;
    float maxZ = enemyPos.z + width / 2.0f;

    // X軸
    float tMin = (minX - rayStart.x) / rayDir.x;
    float tMax = (maxX - rayStart.x) / rayDir.x;
    if (tMin > tMax) std::swap(tMin, tMax);

    // Y軸
    float tyMin = (minY - rayStart.y) / rayDir.y;
    float tyMax = (maxY - rayStart.y) / rayDir.y;
    if (tyMin > tyMax) std::swap(tyMin, tyMax);

    if (tMin > tyMax || tyMin > tMax) return -1.0f;

    if (tyMin > tMin) tMin = tyMin;
    if (tyMax < tMax) tMax = tyMax;

    // Z軸
    float tzMin = (minZ - rayStart.z) / rayDir.z;
    float tzMax = (maxZ - rayStart.z) / rayDir.z;
    if (tzMin > tzMax) std::swap(tzMin, tzMax);

    if (tMin > tzMax || tzMin > tMax) return -1.0f;

    if (tMin < 0) return -1.0f;

    return tMin;  //    敵までの距離
}



void Game::DrawBillboard()
{
    if (!m_showDamageDisplay)
        return;

    // 既存のDrawCubesと同じ描画設定
   // 位置と回転を変数に保存
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    // 小さな黄色いキューブをダメージ表示位置に描画
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f) *  // 小さくする
        DirectX::XMMatrixTranslation(m_damageDisplayPos.x,
            m_damageDisplayPos.y,
            m_damageDisplayPos.z);

    m_cube->Draw(world, viewMatrix, projectionMatrix, DirectX::Colors::Yellow);
}

void Game::DrawParticles()
{
    if (m_particleSystem->IsEmpty())
        return;

    auto context = m_d3dContext.Get();

    // ビュー・プロジェクション行列の計算
    // 位置と回転を変数に保存
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    // 重要：頂点カラーを有効化
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    for (const auto& particle : m_particleSystem->GetParticles())
    {
        float size = 0.1f; // サイズを大きくして見やすく 

        // より見やすい十字形で描画
        DirectX::XMFLOAT3 center = particle.position;

        // 横線
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x - size, center.y, center.z), particle.color),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x + size, center.y, center.z), particle.color)
        );

        // 縦線
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x, center.y - size, center.z), particle.color),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(center.x, center.y + size, center.z), particle.color)
        );
    }

    primitiveBatch->End();
}


void Game::UpdateTitle()
{
    // デバッグ：フェード値を表示
    char debug[256];
    sprintf_s(debug, "TITLE - Press SPACE - Fade:%.2f", m_fadeAlpha);
    SetWindowTextA(m_window, debug);

    // タイトル画面：Spaceキーでゲーム開始
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        m_gameState = GameState::PLAYING;
        m_fadeAlpha = 1.0f;
        m_fadingIn = true;
        m_fadeActive = true;
    }


}

void Game::DrawWeapon()
{
    if (!m_weaponModel)
    {
        return;
    }

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        sinf(playerRot.y) * cosf(playerRot.x),
        -sinf(playerRot.x),
        cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );

    DirectX::XMVECTOR right = DirectX::XMVectorSet(
        cosf(playerRot.y),
        0.0f,
        -sinf(playerRot.y),
        0.0f
    );

    DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

    // === FPS風の位置（右下）===
    DirectX::XMVECTOR weaponPos = cameraPosition +
        right * (0.25f + m_weaponSwayX * 0.1f) +
        up * (-0.35f + m_weaponSwayY * 0.1f) +
        forward * 0.5f;

    // === スケール ===
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.25f, 0.25f, 0.25f);

    // === 回転 ===
    DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2);  // Y軸で90度
    DirectX::XMMATRIX standUp = DirectX::XMMatrixRotationZ(-DirectX::XM_PIDIV2);   // Z軸で90度回転（立てる）
    DirectX::XMMATRIX tilt = DirectX::XMMatrixRotationX(-0.1f);                   // 少し傾ける
    DirectX::XMMATRIX modelRotation = DirectX::XMMatrixRotationY(DirectX::XM_PI);
    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(weaponPos);

    // 合成（standUp を追加）
    DirectX::XMMATRIX weaponWorld = scale * modelFix * standUp * tilt * modelRotation * cameraRotation * translation;

    m_weaponModel->Draw(
        m_d3dContext.Get(),
        weaponWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

// =================================================================
// 【最上位】描画全体の司令塔
// =================================================================
void Game::Render()
{
    Clear();

    switch (m_gameState)
    {
    case GameState::TITLE:
        RenderTitle();
        break;
    case GameState::PLAYING:
        RenderPlaying();
        break;
    case GameState::GAMEOVER:
        RenderGameOver();
        break;
    }

    RenderFade();
    m_swapChain->Present(1, 0);
}

// =================================================================
// 【ゲーム中】の描画処理
// =================================================================
void Game::RenderPlaying()
{
    //  === ビュー・プロジェクション行列の計算   ===
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
    DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    //  === マップ描画   ===
    if (m_mapSystem)
    {
        m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
    }


    //  === 壁武器テクスチャ描画  ===
    if (m_weaponSpawnSystem)
    {
        m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
    }

    // 最初に3D空間のものをすべて描画する
    DrawParticles();
    //DrawGrid();
    DrawEnemies();
    DrawBillboard();
    DrawWeapon();
    DrawWeaponSpawns();

    /*const auto& enemies = m_enemySystem->GetEnemies();
    for (const auto& enemy : enemies)
    {
        if (!enemy.isAlive) continue;

        DirectX::XMMATRIX testWorld =
            DirectX::XMMatrixScaling(3.0f, 0.1f, 3.0f) *
            DirectX::XMMatrixTranslation(enemy.position.x, 0.05f, enemy.position.z);

        m_cube->Draw(testWorld, viewMatrix, projectionMatrix, DirectX::Colors::Yellow);
    }*/
    
    // 最後にUIをすべて手前に描画する
    DrawUI();

    // ダメージエフェクトを一番上に重ねる
    if (m_player->IsDamaged())
    {
        RenderDamageFlash();
    }
}

// 【UI】すべてのUIを描画する
void Game::DrawUI()
{
    // --- UI描画のための共通設定 ---
    auto context = m_d3dContext.Get();
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicOffCenterLH(
        0.0f, (float)m_outputWidth, (float)m_outputHeight, 0.0f, 0.1f, 1.0f);

    m_effect->SetProjection(projection);
    m_effect->SetView(DirectX::XMMatrixIdentity());
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // UISystemに全ての描画を委譲
    m_uiSystem->DrawAll(primitiveBatch.get(),
        m_player.get(),
        m_weaponSystem.get(),
        m_waveManager.get());

    // === 壁武器の購入UI ===
    if (m_nearbyWeaponSpawn != nullptr)
    {
        // すでに持っている武器か？
        bool alreadyOwned = false;
        if (m_weaponSystem->GetPrimaryWeapon() == m_nearbyWeaponSpawn->weaponType ||
            (m_weaponSystem->HasSecondaryWeapon() &&
                m_weaponSystem->GetSecondaryWeapon() == m_nearbyWeaponSpawn->weaponType))
        {
            alreadyOwned = true;
        }

        m_uiSystem->DrawWeaponPrompt(
            primitiveBatch.get(),
            m_nearbyWeaponSpawn,
            m_player->GetPoints(),
            alreadyOwned
        );
    }


    primitiveBatch->End();
}


void Game::UpdatePlaying()
{
    char debug[256];
    sprintf_s(debug, "Wave:%d | Points:%d | Health:%d | Reloading:%s",
        m_waveManager->GetCurrentWave(), m_player->GetPoints(), m_player->GetHealth(),
        m_weaponSystem->IsReloading() ? "YES" : "NO");  // ← リロード状態を表示
    SetWindowTextA(m_window, debug);

    //  === プレイヤー移動 + 壁の当たり判定
    
    //  --- 移動前の位置を保存   ---
    DirectX::XMFLOAT3 oldPosition = m_player->GetPosition();
    //  プレイヤーの移動・回転
    m_player->Update(m_window);
    //移動後の位置を取得
    DirectX::XMFLOAT3 newPosition = m_player->GetPosition();
    //  壁判定用の半径
    const float playerRadius = 0.5f;

    //  X方向の移動をチェック
    DirectX::XMFLOAT3 testPositionX = oldPosition; 
    testPositionX.x = newPosition.x;

    //  --- 壁・箱との当たり判定チェック  X---
    if (m_mapSystem && m_mapSystem->CheckCollision(testPositionX, playerRadius))
    {
        newPosition.x = oldPosition.x;
    }

    //  Z
    DirectX::XMFLOAT3 testPositionZ = oldPosition;
    testPositionZ.z = newPosition.z;

    if (m_mapSystem && m_mapSystem->CheckCollision(testPositionZ, playerRadius))
    {
        newPosition.z = oldPosition.z;
    }

    m_player->SetPosition(newPosition);


    //  === 壁武器購入システム
    m_nearbyWeaponSpawn = m_weaponSpawnSystem->CheckPlayerNearWeapon(
        m_player->GetPosition()
    );

    //  --- Eキーで入力  ---
    static bool eKeyPressed = false;
    if (GetAsyncKeyState('E') & 0x8000)
    {
        if (!eKeyPressed && m_nearbyWeaponSpawn != nullptr)
        {
            //  === 購入処理    ===

            //  既に持っている武器化
            bool already0wned = false;
            if (m_weaponSystem->GetPrimaryWeapon() == m_nearbyWeaponSpawn->weaponType ||
                (m_weaponSystem->HasSecondaryWeapon() &&
                    m_weaponSystem->GetSecondaryWeapon() == m_nearbyWeaponSpawn->weaponType))
            {
                already0wned = true;
            }

            if (already0wned)
            {
                //  --- 弾薬補充 ---
                int ammoCost = m_nearbyWeaponSpawn->cost / 2;

                if (m_player->GetPoints() >= ammoCost)
                {
                    //  ポイント消費
                    m_player->AddPoints(-ammoCost);

                    char msg[128];
                    sprintf_s(msg, "Ammo refilled! -%d points\n", ammoCost);
                    OutputDebugStringA(msg);
                }
                else
                {
                    OutputDebugStringA("Not enough points for ammo!\n");
                }
            }

            else
            {
                //  === 武器購入    ===
                int cost = m_nearbyWeaponSpawn->cost;

                if (m_player->GetPoints() >= cost)
                {
                    m_player->AddPoints(-cost);

                    m_weaponSystem->BuyWeapon(
                    m_nearbyWeaponSpawn->weaponType,
                        m_player->GetPointsRef()
                    );

                    char msg[128];
                    sprintf_s(msg, "Weapon purchased! -%d points\n", cost);
                    OutputDebugStringA(msg);
                }
                else
                {
                    OutputDebugStringA("Not enough points!\n");
                }

            }

            eKeyPressed = true;

        }
    }
    else
    {
        eKeyPressed = false;
    }



    // --- 1回押した時だけ反応するキー入力の管理 ---
    static std::map<int, bool> keyWasPressed;
    auto IsFirstKeyPress = [&](int vk_code) {
        bool isPressed = GetAsyncKeyState(vk_code) & 0x8000;
        if (isPressed && !keyWasPressed[vk_code]) {
            keyWasPressed[vk_code] = true;
            return true;
        }
        if (!isPressed) {
            keyWasPressed[vk_code] = false;
        }
        return false;
        };

    if (IsFirstKeyPress('1') && !m_weaponSystem->IsReloading())
    {
        // プレイヤーがピストルを所持しているか確認し、持っていればそのスロットに切り替える
        if (m_weaponSystem->GetPrimaryWeapon() == WeaponType::PISTOL)
        {
            m_weaponSystem->SetCurrentWeaponSlot(0);
            m_weaponSystem->SwitchWeapon(WeaponType::PISTOL);
        }
        else if (m_weaponSystem->HasSecondaryWeapon() &&
                 m_weaponSystem->GetSecondaryWeapon() == WeaponType::PISTOL)
        {
            m_weaponSystem->SetCurrentWeaponSlot(1);
            m_weaponSystem->SwitchWeapon(WeaponType::PISTOL);
        }
        // 注: ピストルを両方のスロットから売ってしまった場合は何も起こらない
    }
    if (IsFirstKeyPress('2') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::SHOTGUN, m_player->GetPointsRef());
    }
    if (IsFirstKeyPress('3') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::RIFLE, m_player->GetPointsRef());
    }
    if (IsFirstKeyPress('4') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::SNIPER, m_player->GetPointsRef());
    }

    // Qキーで武器スワップ
    if (IsFirstKeyPress('Q') && m_weaponSystem->HasSecondaryWeapon() && !m_weaponSystem->IsReloading()) {
        int newSlot = 1 - m_weaponSystem->GetCurrentWeaponSlot();
        m_weaponSystem->SetCurrentWeaponSlot(newSlot);
        WeaponType newWeapon = (newSlot == 0) ?
            m_weaponSystem->GetPrimaryWeapon() :
            m_weaponSystem->GetSecondaryWeapon();
        m_weaponSystem->SwitchWeapon(newWeapon);
    }


    //  カメラ回転の変化量から武器の揺れを計算
    float rotationDeltaX = m_player->GetRotation().x - m_lastCameraRotX;
    float rotationDeltaY = m_player->GetRotation().y - m_lastCameraRotY;

    //  スウェイ強度調整
    float swayStrength = 0.5f;
    m_weaponSwayX += rotationDeltaY * swayStrength; //  左右回転で横揺れ
    m_weaponSwayY += rotationDeltaX * swayStrength; //  上下回転で横揺れ

    //  減衰効果
    m_weaponSwayX *= 0.9f;
    m_weaponSwayY *= 0.9f;

    //  前フレームの回転を保持
    m_lastCameraRotX = m_player->GetRotation().x;
    m_lastCameraRotY = m_player->GetRotation().y;


    //  連射タイマー更新
    if (m_weaponSystem->GetFireRateTimer() > 0.0f) {
        m_weaponSystem->SetFireRateTimer(m_weaponSystem->GetFireRateTimer() - 1.0f / 60.0f);
    }

    //射撃処理
    if (m_player->IsMouseCaptured())
    {
        bool currentMouseState = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (currentMouseState && !m_lastMouseState &&
            !m_weaponSystem->IsReloading() &&
            m_weaponSystem->GetCurrentAmmo() > 0 &&
            m_weaponSystem->GetFireRateTimer() <= 0.0f)
        {
            auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());

            //  弾を消費
            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() - 1);
            m_weaponSystem->SetFireRateTimer(weapon.fireRate);
            m_weaponSystem->SaveAmmoStatus();   //  弾薬状態を保存

            // 銃口位置を計算
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            DirectX::XMFLOAT3 muzzlePosition;
            muzzlePosition.x = playerPos.x + 0.45f;
            muzzlePosition.y = playerPos.y - 0.15f;
            muzzlePosition.z = playerPos.z + 0.8f;

            m_particleSystem->CreateMuzzleFlash(muzzlePosition, m_player->GetRotation());

            // ショットガンは複数の弾を発射
            int pellets = (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN) ? 8 : 1;
            for (int p = 0; p < pellets; p++)
            {
                // 射撃方向を計算
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

                DirectX::XMFLOAT3 rayStart = playerPos;
                DirectX::XMFLOAT3 rayDir(
                    sinf(playerRot.y)* cosf(playerRot.x),
                    -sinf(playerRot.x),
                    cosf(playerRot.y)* cosf(playerRot.x)
                );

                // 散弾の場合はランダムに広がる
                DirectX::XMFLOAT3 shotDir = rayDir;
                if (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN)
                {
                    float spread = 0.1f;
                    shotDir.x += ((float)rand() / RAND_MAX - 0.5f) * spread;
                    shotDir.y += ((float)rand() / RAND_MAX - 0.5f) * spread;
                    shotDir.z += ((float)rand() / RAND_MAX - 0.5f) * spread;
                }

                //  当たり判定のチェック
                bool hit = false;

                if (!hit)
                {
                    for (auto& enemy : m_enemySystem->GetEnemies())
                    {
                        if (!enemy.isAlive)
                            continue;

                        float hitDistance = CheckRayIntersection(rayStart, shotDir, enemy.position);


                        if (hitDistance > 0.0f)
                        {
                            hit = true;

                            //  ここも「shotDir」を使うのが正解
                            float hitY = rayStart.y + shotDir.y * hitDistance;

                            float heightFromBase = hitY - enemy.position.y;
                            bool isHeadShot = (heightFromBase > 1.5f);

                            int damage = isHeadShot ? 100 : 30;

                            enemy.health -= weapon.damage;

                            if (enemy.health <= 0)
                            {
                                //  敵を倒した
                                enemy.isAlive = false;
                                m_particleSystem->CreateExplosion(enemy.position);
                                
                                //  WaveManagerに通知してボーナス取得
                                int waveBonus = m_waveManager->OnEnemyKilled();
                                int totalPoints = (isHeadShot ? 100 : 60) + waveBonus;
                                m_player->AddPoints(totalPoints);

                                m_showDamageDisplay = true;
                                m_damageDisplayTimer = 2.0f;
                                m_damageDisplayPos = enemy.position;
                                m_damageDisplayPos.y += 2.0f;
                                m_damageValue = isHeadShot ? 100 : 60;

                                char debug[256];
                                sprintf_s(debug, isHeadShot ? "HEADSHOT!! Points:%d" : "Hit! Points:%d",
                                    m_player->GetPoints());
                                SetWindowTextA(m_window, debug);
                            }


                            break;

                        }
                    }
                }


                if (!hit)
                {
                    char debug[256];
                    sprintf_s(debug, "射撃...外れ スコア:%d", m_score);
                    SetWindowTextA(m_window, debug);
                }
            }
        }
        m_lastMouseState = currentMouseState;
    }

    // Rキーでリロード開始
    if (IsFirstKeyPress('R') &&
        m_weaponSystem->GetCurrentAmmo() < m_weaponSystem->GetMaxAmmo() &&
        m_weaponSystem->GetReserveAmmo() > 0 &&
        !m_weaponSystem->IsReloading())
    {
        m_weaponSystem->SetReloading(true);
        auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());
        m_weaponSystem->SetReloadTimer(weapon.reloadTime);
    }

    // リロードタイマー更新
    if (m_weaponSystem->IsReloading())
    {
        float newTimer = m_weaponSystem->GetReloadTimer() - 1.0f / 60.0f;
        m_weaponSystem->SetReloadTimer(newTimer);
        if (newTimer <= 0.0f)
        {
            // リロード完了
            int needed = m_weaponSystem->GetMaxAmmo() - m_weaponSystem->GetCurrentAmmo();
            int reload = min(needed, m_weaponSystem->GetReserveAmmo());

            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() + reload);
            m_weaponSystem->SetReserveAmmo(m_weaponSystem->GetReserveAmmo() - reload);
            m_weaponSystem->SetReloading(false);
            m_weaponSystem->SaveAmmoStatus();
        }
    }


    if (m_showDamageDisplay)
    {
        m_damageDisplayTimer -= (1.0f / 60.0f); // 60FPS想定
        if (m_damageDisplayTimer <= 0.0f)
        {
            m_showDamageDisplay = false;
        }
    }


    m_particleSystem->Update(1.0f / 60.0f);

    //  m_cameraPOs
    //  【型】DirectX::XMFLOAT3
    //  【意味】プレイヤーの現在位置
    //  【用途】敵がプレイヤーに向かって動くため
    m_enemySystem->Update(1.0f / 60.0f, m_player->GetPosition());

    //  === アニメーション更新   ===
    auto& enemies = m_enemySystem->GetEnemies();
    for (auto& enemy : enemies)
    {
        //  --- 生きている敵だけ処理  ---
        if (!enemy.isAlive)
            continue;

        //  --- アニメーション時刻を進める   ---
        enemy.animationTime += 1.0f / 60.0f;


        //  --- ループ処理   ---
        if (m_enemyModel && m_enemyModel->HasAnimation(enemy.currentAnimation))
        {
            //  アニメーションの長さを取得
            float duration = m_enemyModel->GetAnimationDuration(enemy.currentAnimation);

            //  終端を超えたかチェック
            if (enemy.animationTime >= duration)
            {
                //  最初に戻す
                enemy.animationTime = 0.0f;
            }
        }
    }


    //  死んだ敵を削除 毎フレーム敵を削除しないと、配列か肥大化
    m_enemySystem->ClearDeadEnemies();  //  追加

    //  ウェーブ管理
    m_waveManager->Update(1.0f / 60.0f, m_player->GetPosition(), m_enemySystem.get());
  

    // === 接触ダメージ処理 ===
// 【目的】敵に触れられたらプレイヤーがダメージを受ける
// 【仕組み】全ての敵をチェックして、接触していたらHP減少
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        // 生きていて、接触していて、無敵時間が終わっている
        if (enemy.isAlive && enemy.touchingPlayer)
        {
            bool died = m_player->TakeDamage(10);

            if (died)
            {
                m_gameState = GameState::GAMEOVER;
            }

            break;
        }
    }

}


void Game::UpdateGameOver()
{
    // ゲームオーバー画面：Rキーでリスタート
    if (GetAsyncKeyState('R') & 0x8000)
    {
        // ゲーム状態をリセット
        m_score = 0;
        for (int i = 0; i < 3; i++)
        {
            //m_cubesDestroyed[i] = false;
        }
        
        m_gameState = GameState::TITLE;
    }

    char gameOverText[256];
    sprintf_s(gameOverText, "GAME OVER - Final Score: %d - Press R to Restart", m_score);
    SetWindowTextA(m_window, gameOverText);
}

void Game::UpdateFade()
{
    if (!m_fadeActive)
        return;

    float fadeSpeed = 2.0f * (1.0f / 60.0f);

    if (m_fadingIn)
    {
        m_fadeAlpha -= fadeSpeed;
        if (m_fadeAlpha <= 0.0f)
        {
            m_fadeAlpha = 0.0f;
            m_fadeActive = false;
        }
    }
    else
    {
        m_fadeAlpha += fadeSpeed;
        if (m_fadeAlpha >= 1.0f)
        {
            m_fadeAlpha = 1.0f;
            m_fadeActive = false;
        }
    }
}


void Game::RenderDamageFlash()
{
    auto context = m_d3dContext.Get();

    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    float alpha = m_player->GetDamageTimer();  // タイマーに応じてフェードアウト
    DirectX::XMFLOAT4 bloodColor(0.8f, 0.0f, 0.0f, alpha * 0.3f);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面端を赤く
    float borderSize = 100.0f;

    // 上
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), bloodColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight - borderSize, 1.0f), bloodColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(-halfWidth, halfHeight - borderSize, 1.0f), bloodColor);

    primitiveBatch->DrawQuad(v1, v2, v3, v4);

    // 他の3辺も同様に...（省略）

    primitiveBatch->End();
}


void Game::RenderTitle()
{

}

void Game::RenderGameOver()
{

}

void Game::RenderFade()
{
    if (m_fadeAlpha <= 0.0f)
        return;

    // フェード用の2D描画設定
    auto context = m_d3dContext.Get();

    // 2D用の単位行列
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    // 頂点カラーを無効化して単色描画モードに
    m_effect->SetVertexColorEnabled(false);
    DirectX::XMVECTOR diffuseColor = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    m_effect->SetDiffuseColor(diffuseColor);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // フルスクリーンの四角形を描画
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    DirectX::XMFLOAT4 fadeColor(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // 画面全体を覆う四角形
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), fadeColor);

    // 三角形2つで四角形を構成
    primitiveBatch->DrawTriangle(v1, v2, v3);
    primitiveBatch->DrawTriangle(v1, v3, v4);
 
    primitiveBatch->End();

    // 頂点カラーを再度有効化
    m_effect->SetVertexColorEnabled(true);
}

void Game::DrawWeaponSpawns()
{
    if (!m_weaponSpawnSystem)
        return;

    //  ビュー・プロジェクション行列
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y -  sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
    DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );


    //  === 全ての壁武器を描画   ===
    for (const auto& spawn : m_weaponSpawnSystem->GetSpawns())
    {
        //  ワールド行列
        DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            DirectX::XMMatrixTranslation(spawn.position.x, spawn.position.y, spawn.position.z);

        //  色(武騎手によって変える)
        DirectX::XMVECTOR color;
        switch (spawn.weaponType)
        {
        case WeaponType::SHOTGUN:
            color = DirectX::Colors::Orange;
            break;

        case WeaponType::RIFLE:
            color = DirectX::Colors::Green;
            break;

        case WeaponType::SNIPER:
            color = DirectX::Colors::Blue;
            break;

        default:
            color = DirectX::Colors::Gray;
            break;
        }

        m_cube->Draw(world, viewMatrix, projectionMatrix, color);
    }

}
