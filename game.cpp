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

    //カメラポジション
    m_cameraPos(0.0f, 0.5f, -0.5f),
    m_cameraRot(0.0f, 0.0f, 0.0f),
    m_firstMouse(true),
    m_lastMouseX(0),
    m_lastMouseY(0),
    m_cube(nullptr),
    m_lastMouseState(false),
    m_mouseCaptured(false),
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
    m_currentWave(1),
    m_enemiesPerWave(6),
    m_enemiesKilledThisWave(0),
    m_totalEnemiesThisWave(6),
    m_betweenWaves(true),
    m_waveStartTimer(5.0f),
    m_enemySpawnTimer(0.0f),
    m_playerHealth(100),
    m_damageTimer(0.0f),
    m_isDamaged(false),
    m_points(500),
    m_weaponSystem(std::make_unique<WeaponSystem>()),
    m_enemySystem(std::make_unique<EnemySystem>())
{
    static bool firstFrame = true;
    if (firstFrame && m_gameState == GameState::PLAYING)
    {
        firstFrame = false;
        m_betweenWaves = true;
        m_waveStartTimer = 3.0f;  // 3秒後に最初のウェーブ開始
    }
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


    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Blue);
    // Colors::Red, Colors::Green, Colors::Black なども試してみよう

    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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

    m_weaponModel = DirectX::GeometricPrimitive::CreateBox(m_d3dContext.Get(),
        DirectX::XMFLOAT3(0.1f, 0.05f, 0.4f));

    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_d3dContext.Get());

    // フォントファイルを読み込む
    //m_font = std::make_unique<DirectX::SpriteFont>(m_d3dDevice.Get(), L"Arial.spritefont");

}

void Game::CreateExplosion(DirectX::XMFLOAT3 position)
{
    // 爆発エフェクト用パーティクル生成
    for (int i = 0; i < 20; i++)
    {
        Particle particle;
        particle.position = position;

        // ランダムな方向に飛び散る
        float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159f;
        float speed = 5.0f + (float)rand() / RAND_MAX * 10.0f;

        particle.velocity.x = cosf(angle) * speed;
        particle.velocity.y = 5.0f + (float)rand() / RAND_MAX * 10.0f;
        particle.velocity.z = sinf(angle) * speed;

        particle.color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

        particle.lifetime = 1.0f + (float)rand() / RAND_MAX * 2.0f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }
}

void Game::CreateMuzzleFlash()
{
    m_showMuzzleFlash = true;
    m_muzzleFlashTimer = 0.05f;

    CreateMuzzleParticles();
}
void Game::CreateMuzzleParticles()
{
    // 銃口位置を計算
    DirectX::XMFLOAT3 muzzlePosition;
    muzzlePosition.x = m_cameraPos.x + 0.45f;
    muzzlePosition.y = m_cameraPos.y - 0.15f;
    muzzlePosition.z = m_cameraPos.z + 0.8f;

    // 火花パーティクル生成（少数で短時間）
    // 白熱した火花（高温の金属片）
    for (int i = 0; i < 6; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        float spreadAngle = ((float)rand() / RAND_MAX - 0.5f) * 0.8f;
        float speed = 20.0f + (float)rand() / RAND_MAX * 15.0f;

        particle.velocity.x = sinf(m_cameraRot.y + spreadAngle) * speed;
        particle.velocity.y = -sinf(m_cameraRot.x) * speed + ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particle.velocity.z = cosf(m_cameraRot.y + spreadAngle) * speed;

        // 白～薄黄色（高温の金属）
        particle.color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.9f + (float)rand() / RAND_MAX * 0.1f, 1.0f);

        particle.lifetime = 0.15f + (float)rand() / RAND_MAX * 0.1f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }

    // 燃焼ガス（オレンジ色）
    for (int i = 0; i < 4; i++)
    {
        Particle particle;
        particle.position = muzzlePosition;

        float spreadAngle = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        float speed = 8.0f + (float)rand() / RAND_MAX * 8.0f;

        particle.velocity.x = sinf(m_cameraRot.y + spreadAngle) * speed;
        particle.velocity.y = -sinf(m_cameraRot.x) * speed + ((float)rand() / RAND_MAX) * 5.0f;
        particle.velocity.z = cosf(m_cameraRot.y + spreadAngle) * speed;

        // オレンジ～赤色（燃焼ガス）
        particle.color = DirectX::XMFLOAT4(1.0f, 0.4f + (float)rand() / RAND_MAX * 0.4f, 0.1f, 1.0f);

        particle.lifetime = 0.3f + (float)rand() / RAND_MAX * 0.2f;
        particle.maxLifetime = particle.lifetime;

        m_particles.push_back(particle);
    }
}


void Game::DrawGrid()
{
    // 3D描画の準備
    auto context = m_d3dContext.Get();

    // ビュー行列（カメラ位置・向き）の作成
    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&m_cameraPos);

    // カメラが向いている方向を計算
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        m_cameraPos.x + sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
        m_cameraPos.y - sinf(m_cameraRot.x),
        m_cameraPos.z + cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
        0.0f
    );

    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    // プロジェクション行列（透視投影）
    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    // エフェクトに行列を設定
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    // 描画開始
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // プリミティブバッチで線描画
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // グリッド線の描画
    DirectX::XMFLOAT4 gridColor(0.5f, 0.5f, 0.5f, 1.0f); // グレー

    // Z方向の線（手前から奥へ）
    for (int x = -10; x <= 10; x += 2)
    {
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, 0, -10), gridColor),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(x, 0, 10), gridColor)
        );
    }

    // X方向の線（左から右へ）
    for (int z = -10; z <= 10; z += 2)
    {
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(-10, 0, z), gridColor),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(10, 0, z), gridColor)
        );
    }

    primitiveBatch->End();
}

//void Game::DrawCubes()
//{
//    // ビュー・プロジェクション行列はDrawGridと同じものを使用
//    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&m_cameraPos);
//    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
//        m_cameraPos.x + sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
//        m_cameraPos.y - sinf(m_cameraRot.x),
//        m_cameraPos.z + cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
//        0.0f
//    );
//    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);
//
//    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
//    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
//        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
//    );
//
//
//    // キューブ1：赤色(破壊されていなければ描画)
//    if (!m_cubesDestroyed[0])
//    {
//        DirectX::XMMATRIX world1 = DirectX::XMMatrixTranslation(5.0f, 1.0f, 5.0f);
//        m_cube->Draw(world1, viewMatrix, projectionMatrix, DirectX::Colors::Red);
//    }
//
//    if (!m_cubesDestroyed[1])
//    {
//        // キューブ2：青色
//        DirectX::XMMATRIX world2 = DirectX::XMMatrixTranslation(-3.0f, 1.0f, 8.0f);
//        m_cube->Draw(world2, viewMatrix, projectionMatrix, DirectX::Colors::Blue);
//    }
//
//    if (!m_cubesDestroyed[2])
//    {
//        // キューブ3：緑色
//        DirectX::XMMATRIX world3 = DirectX::XMMatrixTranslation(0.0f, 1.0f, 15.0f);
//        m_cube->Draw(world3, viewMatrix, projectionMatrix, DirectX::Colors::Green);
//    }
//}

void Game::DrawEnemies()
{
    // ビュー・プロジェクション行列の計算
    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&m_cameraPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        m_cameraPos.x + sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
        m_cameraPos.y - sinf(m_cameraRot.x),
        m_cameraPos.z + cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    // 敵を描画
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive)
            continue;

        DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(
            enemy.position.x, enemy.position.y, enemy.position.z
        );

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(&enemy.color);
        m_cube->Draw(world, viewMatrix, projectionMatrix, color);
    }

    // HPバーを描画
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

        // HPバーの位置（敵の上）
        float barWidth = 1.0f;
        float barHeight = 0.1f;
        float healthPercent = (float)enemy.health / enemy.maxHealth;

        DirectX::XMFLOAT3 barCenter = enemy.position;
        barCenter.y += 2.5f;

        // 背景（赤）
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

        // HP部分（明るい赤）
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

bool Game::CheckRayHitsKube(DirectX::XMFLOAT3 rayStart, DirectX::XMFLOAT3 rayDir, DirectX::XMFLOAT3 cubePos)
{
    // レイ（光線）とキューブの当たり判定
    // キューブのサイズ（1x1x1と仮定）
    float cubeSize = 2.0f;

    // キューブの境界を計算
    float minX = cubePos.x - cubeSize * 0.5f;
    float maxX = cubePos.x + cubeSize * 0.5f;
    float minY = cubePos.y - cubeSize * 0.5f;
    float maxY = cubePos.y + cubeSize * 0.5f;
    float minZ = cubePos.z - cubeSize * 0.5f;
    float maxZ = cubePos.z + cubeSize * 0.5f;

    // レイとボックスの交点計算
    float tMin = (minX - rayStart.x) / rayDir.x;
    float tMax = (maxX - rayStart.x) / rayDir.x;

    if (tMin > tMax) std::swap(tMin, tMax);

    float tyMin = (minY - rayStart.y) / rayDir.y;
    float tyMax = (maxY - rayStart.y) / rayDir.y;

    if (tyMin > tyMax) std::swap(tyMin, tyMax);

    if (tMin > tyMax || tyMin > tMax) return false;

    if (tyMin > tMin) tMin = tyMin;
    if (tyMax < tMax) tMax = tyMax;

    float tzMin = (minZ - rayStart.z) / rayDir.z;
    float tzMax = (maxZ - rayStart.z) / rayDir.z;

    if (tzMin > tzMax) std::swap(tzMin, tzMax);

    if (tMin > tzMax || tzMin > tMax) return false;

    return tMin > 0; // レイが前方向に進んでいることを確認
}



void Game::DrawBillboard()
{
    if (!m_showDamageDisplay)
        return;

    // 既存のDrawCubesと同じ描画設定
    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&m_cameraPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        m_cameraPos.x + sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
        m_cameraPos.y - sinf(m_cameraRot.x),
        m_cameraPos.z + cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
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
    if (m_particles.empty())
        return;

    auto context = m_d3dContext.Get();

    // ビュー・プロジェクション行列の計算
    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&m_cameraPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        m_cameraPos.x + sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
        m_cameraPos.y - sinf(m_cameraRot.x),
        m_cameraPos.z + cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
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

    for (const auto& particle : m_particles)
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
    // カメラ相対座標で武器を配置（HUD的な表示）
    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&m_cameraPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        m_cameraPos.x + sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
        m_cameraPos.y - sinf(m_cameraRot.x),
        m_cameraPos.z + cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
        0.0f
    );
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(70.0f), aspectRatio, 0.1f, 1000.0f
    );

    // カメラの前方・右方・上方ベクトルを計算
    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
        -sinf(m_cameraRot.x),
        cosf(m_cameraRot.y) * cosf(m_cameraRot.x),
        0.0f
    );

    DirectX::XMVECTOR right = DirectX::XMVectorSet(
        cosf(m_cameraRot.y),
        0.0f,
        -sinf(m_cameraRot.y),
        0.0f
    );

    DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

    // カメラからの相対位置で武器位置を計算
    DirectX::XMVECTOR weaponPos = cameraPosition +
        right * (0.3f + m_weaponSwayX * 0.1f) +      // 右に0.3
        up * (-0.2f + m_weaponSwayY * 0.1f) +      // 下に0.2
        forward * 0.4f;     // 前に0.4

    // 武器をカメラの向きに合わせて回転
    DirectX::XMMATRIX weaponWorld = DirectX::XMMatrixRotationRollPitchYaw(
        m_cameraRot.x, m_cameraRot.y, 0.0f) *
        DirectX::XMMatrixTranslationFromVector(weaponPos);

    m_weaponModel->Draw(weaponWorld, viewMatrix, projectionMatrix, DirectX::Colors::Black);


    //// 銃口フラッシュの描画
    //if (m_showMuzzleFlash)
    //{
    //    // 武器の先端位置を計算
    //    DirectX::XMVECTOR muzzlePos = cameraPosition +
    //        right * (0.35f + m_weaponSwayX * 0.1f) +
    //        up * (-0.15f + m_weaponSwayY * 0.1f) +
    //        forward * 0.8f;

    //    DirectX::XMMATRIX flashWorld = DirectX::XMMatrixTranslationFromVector(muzzlePos);

    //    m_muzzleFlashModel->Draw(flashWorld, viewMatrix, projectionMatrix, DirectX::Colors::Yellow);
    //}


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
    // 最初に3D空間のものをすべて描画する
    DrawParticles();
    DrawGrid();
    DrawEnemies();
    DrawBillboard();
    DrawWeapon();

    // 最後にUIをすべて手前に描画する
    DrawUI();

    // ダメージエフェクトを一番上に重ねる
    if (m_isDamaged)
    {
        RenderDamageFlash();
    }
}

// =================================================================
// 【UI】すべてのUIを描画する司令塔
// =================================================================
void Game::DrawUI()
{
    // --- UI描画のための共通設定 ---
    auto context = m_d3dContext.Get();
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicOffCenterLH(0.0f, (float)m_outputWidth, (float)m_outputHeight, 0.0f, 0.1f, 1.0f);

    m_effect->SetProjection(projection);
    m_effect->SetView(DirectX::XMMatrixIdentity());
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // --- ここから各UI要素の描画処理 ---

    // (1) 体力バーの描画 (左下)
    {
        float barWidth = 200.0f, barHeight = 20.0f, padding = 50.0f;
        float startX = padding, startY = m_outputHeight - padding - barHeight;
        DirectX::XMFLOAT4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
        for (float i = 0; i < barHeight; ++i) {
            primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + i, 1.0f), bgColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + i, 1.0f), bgColor));
        }
        float healthPercent = (float)m_playerHealth / 100.0f;
        float currentBarWidth = barWidth * healthPercent;
        if (currentBarWidth > 0) {
            DirectX::XMFLOAT4 healthColor;
            if (healthPercent > 0.6f) healthColor = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
            else if (healthPercent > 0.3f) healthColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
            else healthColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
            for (float i = 0; i < barHeight; ++i) {
                primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + i, 1.0f), healthColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + currentBarWidth, startY + i, 1.0f), healthColor));
            }
        }
        DirectX::XMFLOAT4 borderColor(1.0f, 1.0f, 1.0f, 1.0f);
        primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY, 1.0f), borderColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY, 1.0f), borderColor));
        primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + barHeight, 1.0f), borderColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + barHeight, 1.0f), borderColor));
        primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY, 1.0f), borderColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX, startY + barHeight, 1.0f), borderColor));
        primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY, 1.0f), borderColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(startX + barWidth, startY + barHeight, 1.0f), borderColor));
    }

    // (2) クロスヘアの描画 (中央)
    {
        DirectX::XMFLOAT4 crosshairColor(1.0f, 1.0f, 1.0f, 1.0f);
        float centerX = m_outputWidth / 2.0f, centerY = m_outputHeight / 2.0f, size = 20.0f;
        primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX, centerY - size, 1.0f), crosshairColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX, centerY + size, 1.0f), crosshairColor));
        primitiveBatch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX - size, centerY, 1.0f), crosshairColor), DirectX::VertexPositionColor(DirectX::XMFLOAT3(centerX + size, centerY, 1.0f), crosshairColor));
    }

    // (3) ウェーブ数の描画 (上中央)
    {
        DirectX::XMFLOAT4 color(1.0f, 1.0f, 0.0f, 1.0f); float digitWidth = 15.0f; float digitSpacing = 20.0f;
        int wave = m_currentWave;
        if (wave == 0) { DrawSimpleNumber(primitiveBatch.get(), 0, (m_outputWidth - digitWidth) / 2.0f, 50.0f, color); }
        else {
            std::vector<int> digits; while (wave > 0) { digits.push_back(wave % 10); wave /= 10; }
            int numDigits = digits.size(); float totalWidth = numDigits * digitWidth + (numDigits - 1) * (digitSpacing - digitWidth);
            float startX = (m_outputWidth - totalWidth) / 2.0f; float startY = 50.0f;
            for (int i = 0; i < numDigits; ++i) { DrawSimpleNumber(primitiveBatch.get(), digits[numDigits - 1 - i], startX + i * digitSpacing, startY, color); }
        }
    }

    // (4) ポイントの描画 (右上)
    {
        DirectX::XMFLOAT4 color(0.1f, 1.0f, 1.0f, 1.0f); float digitWidth = 15.0f; float digitSpacing = 20.0f; float padding = 50.0f;
        int points = m_points;
        if (points == 0) { DrawSimpleNumber(primitiveBatch.get(), 0, m_outputWidth - padding - digitWidth, padding, color); }
        else {
            std::vector<int> digits; while (points > 0) { digits.push_back(points % 10); points /= 10; }
            int numDigits = digits.size(); float totalWidth = numDigits * digitWidth + (numDigits - 1) * (digitSpacing - digitWidth);
            float startX = m_outputWidth - padding - totalWidth; float startY = padding;
            for (int i = 0; i < numDigits; ++i) { DrawSimpleNumber(primitiveBatch.get(), digits[numDigits - 1 - i], startX + i * digitSpacing, startY, color); }
        }
    }

    // (5) 弾薬の描画 (右下)
    {
        DirectX::XMFLOAT4 color(1.0f, 1.0f, 1.0f, 1.0f);
        DirectX::XMFLOAT4 reloadingColor(1.0f, 0.2f, 0.2f, 1.0f);
        float digitHeight = 25.0f;
        float digitWidth = 15.0f;
        float digitSpacing = 20.0f;
        float separatorWidth = 20.0f;
        float padding = 50.0f;

        // リロード中なら赤色に変更
        DirectX::XMFLOAT4 currentColor = m_weaponSystem->IsReloading() ? reloadingColor : color;

        // 弾薬数を取得
        std::string currentAmmoStr = std::to_string(m_weaponSystem->GetCurrentAmmo());
        std::string reserveAmmoStr = std::to_string(m_weaponSystem->GetReserveAmmo());

        float currentWidth = currentAmmoStr.length() * digitWidth + (currentAmmoStr.length() - 1) * (digitSpacing - digitWidth);
        float reserveWidth = reserveAmmoStr.length() * digitWidth + (reserveAmmoStr.length() - 1) * (digitSpacing - digitWidth);
        float totalWidth = currentWidth + separatorWidth + reserveWidth;
        float startX = m_outputWidth - padding - totalWidth;
        float startY = m_outputHeight - padding - digitHeight;

        // デバッグ：リロード状態を確認
        bool isReloading = m_weaponSystem->IsReloading();
        char debugMsg[256];
        sprintf_s(debugMsg, "DrawUI時点でリロード中: %s\n", isReloading ? "YES" : "NO");
        OutputDebugStringA(debugMsg);

        // 現在弾数を描画（リロード中なら赤）
        float currentX = startX;
        for (char c : currentAmmoStr) {
            DrawSimpleNumber(primitiveBatch.get(), c - '0', currentX, startY, currentColor);  // ← currentColor使用
            currentX += digitSpacing;
        }

        // スラッシュ（白）
        currentX += (separatorWidth - digitSpacing) / 2;
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(currentX, startY + digitHeight, 1.0f), color),
            DirectX::VertexPositionColor(DirectX::XMFLOAT3(currentX + 10.0f, startY, 1.0f), color)
        );

        // 予備弾数を描画（常に白）
        currentX += separatorWidth - (separatorWidth - digitSpacing) / 2;
        for (char c : reserveAmmoStr) {
            DrawSimpleNumber(primitiveBatch.get(), c - '0', currentX, startY, color);  // ← color使用
            currentX += digitSpacing;
        }
    }

    // (6) 現在の武器名表示（中央下）
    {
        DirectX::XMFLOAT4 weaponColor(1.0f, 1.0f, 1.0f, 1.0f);
        float centerX = m_outputWidth / 2.0f;
        float bottomY = m_outputHeight - 120.0f;

        // 現在の武器番号を取得
        int weaponNum = (int)m_weaponSystem->GetCurrentWeapon() + 1;
        DrawSimpleNumber(primitiveBatch.get(), weaponNum, centerX - 30, bottomY, weaponColor);
    }

    primitiveBatch->End();
}

// =================================================================
// 【部品】数字を描画するためのヘルパー関数
// =================================================================
void Game::DrawSimpleNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int digit, float x, float y, DirectX::XMFLOAT4 color) {
    float w = 15.0f; float h = 25.0f;
    auto DrawThickLine = [&](float x1, float y1, float x2, float y2) { batch->DrawLine(DirectX::VertexPositionColor(DirectX::XMFLOAT3(x1, y1, 1.0f), color), DirectX::VertexPositionColor(DirectX::XMFLOAT3(x2, y2, 1.0f), color)); };
    switch (digit) {
    case 0: DrawThickLine(x, y, x + w, y); DrawThickLine(x, y, x, y + h); DrawThickLine(x + w, y, x + w, y + h); DrawThickLine(x, y + h, x + w, y + h); break;
    case 1: DrawThickLine(x + w, y, x + w, y + h); break;
    case 2: DrawThickLine(x, y, x + w, y); DrawThickLine(x + w, y, x + w, y + h / 2); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x, y + h / 2, x, y + h); DrawThickLine(x, y + h, x + w, y + h); break;
    case 3: DrawThickLine(x, y, x + w, y); DrawThickLine(x + w, y, x + w, y + h); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x, y + h, x + w, y + h); break;
    case 4: DrawThickLine(x, y, x, y + h / 2); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x + w, y, x + w, y + h); break;
    case 5: DrawThickLine(x, y, x + w, y); DrawThickLine(x, y, x, y + h / 2); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x + w, y + h / 2, x + w, y + h); DrawThickLine(x, y + h, x + w, y + h); break;
    case 6: DrawThickLine(x, y, x + w, y); DrawThickLine(x, y, x, y + h); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x + w, y + h / 2, x + w, y + h); DrawThickLine(x, y + h, x + w, y + h); break;
    case 7: DrawThickLine(x, y, x + w, y); DrawThickLine(x + w, y, x + w, y + h); break;
    case 8: DrawThickLine(x, y, x + w, y); DrawThickLine(x, y, x, y + h); DrawThickLine(x + w, y, x + w, y + h); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x, y + h, x + w, y + h); break;
    case 9: DrawThickLine(x, y, x + w, y); DrawThickLine(x, y, x, y + h / 2); DrawThickLine(x + w, y, x + w, y + h); DrawThickLine(x, y + h / 2, x + w, y + h / 2); DrawThickLine(x, y + h, x + w, y + h); break;
    }
}

void Game::UpdatePlaying()
{
    char debug[256];
    sprintf_s(debug, "Wave:%d | Points:%d | Health:%d | Reloading:%s",
        m_currentWave, m_points, m_playerHealth,
        m_weaponSystem->IsReloading() ? "YES" : "NO");  // ← リロード状態を表示
    SetWindowTextA(m_window, debug);

    //  前後移動
    if (GetAsyncKeyState('W') & 0x8000)
    {
        float forwardX = sinf(m_cameraRot.y);   //  X方向成分
        float forwardZ = cosf(m_cameraRot.y);   //  Z方向成分

        float moveSpeed = 0.1f;
        m_cameraPos.x += forwardX * moveSpeed;
        m_cameraPos.z += forwardZ * moveSpeed;
    }

    if (GetAsyncKeyState('S') & 0x8000)
    {
        float forwardX = sinf(m_cameraRot.y);
        float forwardZ = cosf(m_cameraRot.y);

        float moveSpeed = 0.1f;
        m_cameraPos.x -= forwardX * moveSpeed;
        m_cameraPos.z -= forwardZ * moveSpeed;
    }


    //  左右移動
    if (GetAsyncKeyState('A') & 0x8000)
    {
        float leftX = sinf(m_cameraRot.y - 1.57f);
        float leftZ = cosf(m_cameraRot.y - 1.57f);

        float moveSpeed = 0.1f;
        m_cameraPos.x += leftX * moveSpeed;
        m_cameraPos.z += leftZ * moveSpeed;
    }

    if (GetAsyncKeyState('D') & 0x8000)
    {
        float rightX = sinf(m_cameraRot.y + 1.57f);
        float rightZ = cosf(m_cameraRot.y + 1.57f);

        float moveSpeed = 0.1f;
        m_cameraPos.x += rightX * moveSpeed;
        m_cameraPos.z += rightZ * moveSpeed;
    }

    //  マウス固定制御
    if (GetAsyncKeyState(VK_TAB) & 0x8000)
    {
        static bool tabPressed = false;
        if (!tabPressed)
        {
            m_mouseCaptured = !m_mouseCaptured;
            if (m_mouseCaptured)
            {
                ShowCursor(FALSE);  //  カーソル非表示

                //  ウィンドウ中央にマスを移動
                RECT rect;
                GetClientRect(m_window, &rect);
                POINT center = { rect.right / 2, rect.bottom / 2 };
                ClientToScreen(m_window, &center);
                SetCursorPos(center.x, center.y);

                SetWindowTextA(m_window, "マウス固定: ON (Tabで解除)");
            }
            else
            {
                ShowCursor(TRUE);
                SetWindowTextA(m_window, "マウス固定: OFF (Tabで有効)");
            }
            tabPressed = true;
        }
    }
    else
    {
        static bool tabPressed = false;
        tabPressed = false;
    }



    //  マウス
    POINT mousePos;
    GetCursorPos(&mousePos);                //  画面上の座標
    ScreenToClient(m_window, &mousePos);    //  ウィンドウ内座標に変換

    if (m_mouseCaptured)
    {
        if (!m_firstMouse) {
            //  マウスの移動量を計算
            int deltaX = mousePos.x - m_lastMouseX;
            int deltaY = mousePos.y - m_lastMouseY;

            //  回転速度を更新
            float mouseSensitivity = 0.002f;

            m_cameraRot.y += deltaX * mouseSensitivity;
            m_cameraRot.x += deltaY * mouseSensitivity;
        }
        else
        {
            m_firstMouse = false;
        }

        RECT rect;
        GetClientRect(m_window, &rect);
        m_lastMouseX = rect.right / 2;
        m_lastMouseY = rect.bottom / 2;

        POINT center = { m_lastMouseX, m_lastMouseY };
        ClientToScreen(m_window, &center);
        SetCursorPos(center.x, center.y);
    }

    else
    {
        m_lastMouseX = mousePos.x;
        m_lastMouseY = mousePos.y;
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
        m_weaponSystem->BuyWeapon(WeaponType::SHOTGUN, m_points);
    }
    if (IsFirstKeyPress('3') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::RIFLE, m_points);
    }
    if (IsFirstKeyPress('4') && !m_weaponSystem->IsReloading()) {
        m_weaponSystem->BuyWeapon(WeaponType::SNIPER, m_points);
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
    float rotationDeltaX = m_cameraRot.x - m_lastCameraRotX;
    float rotationDeltaY = m_cameraRot.y - m_lastCameraRotY;

    //  スウェイ強度調整
    float swayStrength = 0.5f;
    m_weaponSwayX += rotationDeltaY * swayStrength; //  左右回転で横揺れ
    m_weaponSwayY += rotationDeltaX * swayStrength; //  上下回転で横揺れ

    //  減衰効果
    m_weaponSwayX *= 0.9f;
    m_weaponSwayY *= 0.9f;

    //  前フレームの回転を保持
    m_lastCameraRotX = m_cameraRot.x;
    m_lastCameraRotY = m_cameraRot.y;


    //  連射タイマー更新
    if (m_weaponSystem->GetFireRateTimer() > 0.0f) {
        m_weaponSystem->SetFireRateTimer(m_weaponSystem->GetFireRateTimer() - 1.0f / 60.0f);
    }

    //射撃処理
    if (m_mouseCaptured)
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

            CreateMuzzleFlash();
            

            // ショットガンは複数の弾を発射
            int pellets = (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN) ? 8 : 1;
            for (int p = 0; p < pellets; p++)
            {
                // 射撃方向を計算
                DirectX::XMFLOAT3 rayStart = m_cameraPos;
                DirectX::XMFLOAT3 rayDir(
                    sinf(m_cameraRot.y) * cosf(m_cameraRot.x),
                    -sinf(m_cameraRot.x),
                    cosf(m_cameraRot.y) * cosf(m_cameraRot.x)
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

                        //float distance = // 距離計算
                        //    if (distance > weapon.range) continue;  // 射程外

                        if (CheckRayHitsKube(rayStart, rayDir, enemy.position))
                        {
                            hit = true;

                            //  ダメージを与える(ヘッドショット判定)
                            bool isHeadshot = (rayDir.y < -0.3f);
                            int damage = isHeadshot ? 100 : 30;

                            enemy.health -= weapon.damage;

                            if (enemy.health <= 0)
                            {
                                //  敵を倒した
                                enemy.isAlive = false;
                                CreateExplosion(enemy.position);
                                m_points += isHeadshot ? 100 : 60;  //  ヘッドショットボーナス
                                m_enemiesKilledThisWave++;

                                m_showDamageDisplay = true;
                                m_damageDisplayTimer = 2.0f;
                                m_damageDisplayPos = enemy.position;
                                m_damageDisplayPos.y += 2.0f;
                                m_damageValue = isHeadshot ? 100 : 60;

                                char debug[256];
                                sprintf_s(debug, "敵撃破！スコア:%d", m_score);
                                SetWindowTextA(m_window, debug);
                            }

                            else
                            {
                                enemy.color.x = 1.0f;
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


    UpdateParticles();

    //  m_cameraPOs
    //  【型】DirectX::XMFLOAT3
    //  【意味】プレイヤーの現在位置
    //  【用途】敵がプレイヤーに向かって動くため
    m_enemySystem->Update(1.0f / 60.0f, m_cameraPos);

    //  死んだ敵を削除 毎フレーム敵を削除しないと、配列か肥大化
    m_enemySystem->ClearDeadEnemies();  //  追加
  

    // === 接触ダメージ処理（ここに追加）===
// 【目的】敵に触れられたらプレイヤーがダメージを受ける
// 【仕組み】全ての敵をチェックして、接触していたらHP減少
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        // 生きていて、接触していて、無敵時間が終わっている
        if (enemy.isAlive && enemy.touchingPlayer && m_damageTimer <= 0.0f)
        {
            m_playerHealth -= 10;        // HP を10減らす
            m_damageTimer = 1.0f;        // 1秒間の無敵時間
            m_isDamaged = true;          // ダメージエフェクト表示フラグ

            // HP が 0 以下になったらゲームオーバー
            if (m_playerHealth <= 0)
            {
                m_gameState = GameState::GAMEOVER;
            }

            break;  // 1フレームに1体だけダメージを与える
        }
    }


    //  ウェーブ管理
    float deltaTime = 1.0f / 60.0f;

    if (m_betweenWaves)
    {
        //  ウェーブ間の準備期間
        m_waveStartTimer -= deltaTime;
        if (m_waveStartTimer <= 0.0f)
        {
            //  新ウェーブ開始
            m_betweenWaves = false;
            m_enemiesKilledThisWave = 0;    //  このウェーブで倒した敵の数
            m_totalEnemiesThisWave = m_enemiesPerWave;  //  このウェーブの敵総数 = ウェーブごとの敵の数

            //  初期スポーン(2 - 4体)
            int initialSpawn = min(2 + (m_currentWave - 1), 4);
            initialSpawn = min(initialSpawn, m_totalEnemiesThisWave);
            for (int i = 0; i < initialSpawn; i++)
            {
                m_enemySystem->SpawnEnemy(m_cameraPos);
            }
        }
    }

    else
    {
        //  ウェーブ中のスポーン管理

        //  現在の敵の数をカウント
        int currentEnemyCount = 0;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.isAlive) currentEnemyCount++;
        }

        //  スポーン敵が必要かどうか
        int totalSpawned = m_enemiesKilledThisWave + currentEnemyCount;
        if (totalSpawned < m_totalEnemiesThisWave &&
            currentEnemyCount < m_enemySystem->GetMaxEnemies())
        {
            m_enemySpawnTimer += deltaTime;

            //  ウェーブが進むにつれてスポーン速度アップ
            float spawnInterval = 2.0f - (m_currentWave * 0.1f);
            spawnInterval = max(0.5f, spawnInterval);

            if (m_enemySpawnTimer >= spawnInterval)
            {
                m_enemySystem->SpawnEnemy(m_cameraPos);
                m_enemySpawnTimer = 0.0f;
            }
        }

        //  ウェーブクリア判定
        if (m_enemiesKilledThisWave >= m_totalEnemiesThisWave)
        {
            m_currentWave++;
            m_enemiesPerWave = 6 + (m_currentWave - 1) * 3; //毎ウェーブ3体追加
            m_betweenWaves = true;
            m_waveStartTimer = 10.0f;
            m_points += 100;    //  ウェーブクリアボーナス
        }
    }

    // ダメージタイマー更新
    if (m_damageTimer > 0.0f)
    {
        m_damageTimer -= 1.0f / 60.0f;
        if (m_damageTimer <= 0.0f)
        {
            m_isDamaged = false;
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
        m_cameraPos = DirectX::XMFLOAT3(0.0f, 2.0f, -5.0f);
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

void Game::UpdateParticles()
{
    float deltaTime = 1.0f / 60.0f; // 60FPS想定

    // パーティクルの更新
    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        // 位置更新
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;

        // 重力適用
        it->velocity.y -= 9.8f * deltaTime;

        // ライフタイム更新
        it->lifetime -= deltaTime;

        // フェードアウト
        float alpha = it->lifetime / it->maxLifetime;
        it->color.w = alpha;

        // 寿命切れのパーティクルを削除
        if (it->lifetime <= 0.0f)
        {
            it = m_particles.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 銃口フラッシュタイマー更新
    if (m_showMuzzleFlash)
    {
        m_muzzleFlashTimer -= deltaTime;
        if (m_muzzleFlashTimer <= 0.0f)
        {
            m_showMuzzleFlash = false;
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

    float alpha = m_damageTimer;  // タイマーに応じてフェードアウト
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
