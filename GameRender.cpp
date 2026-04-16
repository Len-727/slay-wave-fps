// ============================================================
//  GameRender.cpp
//  3D描画パイプライン（敵/武器/盾/パーティクル/ヘルスピックアップ）
//
//  【責務】
//  - メインのRender()ディスパッチャ（シーン別に呼び分け）
//  - RenderPlaying(): プレイ中の全3D/2D描画の統括
//  - 敵の描画（インスタンス描画 + 単体描画）
//  - 武器/盾のFPS視点描画
//  - ビルボード/パーティクル描画
//  - 武器スポーン/ヘルスピックアップの描画
//  - レイ交差判定（武器スポーンのインタラクション用）
//
//  【設計意図】
//  「画面に3Dオブジェクトを描く」処理を全て集約。
//  GameHUD.cpp（2D UI）とは明確に分離。
//  描画順序の管理がこのファイルの最重要責務。
//
//  【描画順序（RenderPlaying内）】
//  1. マップ（不透明）
//  2. 敵（インスタンス描画）
//  3. 切断ピース / 肉片
//  4. 武器スポーン / ヘルスピックアップ
//  5. 武器（FPS視点、深度クリア後）
//  6. 盾
//  7. パーティクル（加算ブレンド）
//  8. ビルボード
//  9. HUD（2D、深度OFF）
//  10. デバッグ表示
// ============================================================

#include "Game.h"

using namespace DirectX;


// ============================================================
//  Render - メイン描画ディスパッチャ
//
//  【呼び出し元】Tick() から毎フレーム1回
//  【役割】現在のGameStateに応じてシーン別の描画を呼び分ける
//  PLAYING以外のシーンはGameScene.cppに委譲済み
// ============================================================
void Game::Render()
{
    Clear();

    switch (m_gameState)
    {
    case GameState::TITLE:
        RenderTitle();
        break;

    case GameState::LOADING:
        RenderLoading();
        break;

    case GameState::PLAYING:
        RenderPlaying();
        break;
    case GameState::GAMEOVER:
        RenderGameOver();
        break;
    case GameState::RANKING:
        RenderRanking();
        break;
    }

    RenderFade();
    m_swapChain->Present(1, 0);
}

// =================================================================
// 【ゲーム中】の描画処理
// =================================================================

// ============================================================
//  RenderPlaying - プレイ中の全描画を統括
//
//  【処理順序が重要！】不透明→半透明→加算ブレンドの順。
//  深度テストの関係で、描画順を間違えると透過が壊れる。
//
//  【約580行ある理由】カメラ行列の構築、各システムへの
//  描画委譲、ブレンド状態の切り替えが全てここに集約。
//  将来的にはRenderPassクラスに分割すべき。
// ============================================================
void Game::RenderPlaying()
{
    // デバッグログ
   /* char debugRender[256];
    sprintf_s(debugRender, "[RENDER] DOFActive=%d, Intensity=%.2f, OffscreenRTV=%p\n",
        m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_offscreenRTV.Get());
    //OutputDebugStringA(debugRender);*/

    if (m_gloryKillDOFActive && m_offscreenRTV)
    {
        // ============================================================
        // === グローリーキル中：ブラー処理 ===
        // ============================================================

        // オフスクリーンに通常描画
        ID3D11RenderTargetView* offscreenRTV = m_offscreenRTV.Get();
        m_d3dContext->OMSetRenderTargets(1, &offscreenRTV, m_offscreenDepthStencilView.Get());  // // 変更

        // オフスクリーンをクリア
        DirectX::XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_d3dContext->ClearRenderTargetView(m_offscreenRTV.Get(), clearColor);
        m_d3dContext->ClearDepthStencilView(
            m_offscreenDepthStencilView.Get(),  // // 変更
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0
        );

        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        //  ビューポート設定
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_outputWidth);
        viewport.Height = static_cast<float>(m_outputHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_d3dContext->RSSetViewports(1, &viewport);

        // カメラ設定
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        playerPos.y += m_player->GetLandingCameraOffset();  // 着地演出：カメラを沈ませる

        DirectX::XMFLOAT3 shakeOffset(0.0f, 0.0f, 0.0f);
        if (m_cameraShakeTimer > 0.0f)
        {
            shakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.z = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        }

        DirectX::XMVECTOR cameraPosition;
        DirectX::XMVECTOR cameraTarget;

        if (m_gloryKillCameraActive)
        {
            cameraPosition = DirectX::XMLoadFloat3(&m_gloryKillCameraPos);
            cameraTarget = DirectX::XMLoadFloat3(&m_gloryKillCameraTarget);
        }
        else
        {
            cameraPosition = DirectX::XMVectorSet(
                playerPos.x + shakeOffset.x,
                playerPos.y + shakeOffset.y,
                playerPos.z + shakeOffset.z,
                0.0f
            );

            cameraTarget = DirectX::XMVectorSet(
                playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
                playerPos.y - sinf(playerRot.x),
                playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
                0.0f
            );
        }

        DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

        float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
        DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
        );

        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        // マップ描画
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }

        DrawNavGridDebug(viewMatrix, projectionMatrix);
        DrawSlicedPieces(viewMatrix, projectionMatrix);

        //  地面の苔を描画
           /* if (m_furRenderer && m_furReady)
            {
                m_furRenderer->DrawGroundMoss(
                    m_d3dContext.Get(),
                    viewMatrix,
                    projectionMatrix,
                    m_accumulatedAnimTime
                );
            }*/

            //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        //// 武器スポーンのテクスチャ描画
        //if (m_weaponSpawnSystem)
        //{
        //    m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        //}
        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        // 3D描画（優先順位順）
        //DrawParticles();
        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        DrawEnemies(viewMatrix, projectionMatrix, true);// trueでターゲット敵をスキップ

        DrawKeyPrompt(viewMatrix, projectionMatrix);

        //  深度テストと深度書き込みを有効化
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);



        //DrawBillboard();
        DrawWeapon();
        DrawShield();



        //DrawWeaponSpawns();

        DrawGibs(viewMatrix, projectionMatrix);

        // グローリーキル腕・ナイフ描画（FBXモデル版）
        if ((m_gloryKillArmAnimActive || m_debugShowGloryKillArm) && m_gloryKillArmModel)
        {
            DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
            forward = DirectX::XMVector3Normalize(forward);

            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
            right = DirectX::XMVector3Normalize(right);
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // === アームの位置計算（調整変数使用） ===
            DirectX::XMVECTOR armWorldPos = cameraPosition;
            armWorldPos += forward * m_gloryKillArmOffset.x;  // 前方
            armWorldPos += up * m_gloryKillArmOffset.y;       // 上下
            armWorldPos += right * m_gloryKillArmOffset.z;    // 左右

            float cameraYaw = atan2f(
                DirectX::XMVectorGetX(forward),
                DirectX::XMVectorGetZ(forward)
            );

            // === アームのワールド行列 ===
            DirectX::XMMATRIX armScale = DirectX::XMMatrixScaling(
                m_gloryKillArmScale, m_gloryKillArmScale, m_gloryKillArmScale);

            // ベース回転（モデルを正しい向きに補正）
            DirectX::XMMATRIX baseRotX = DirectX::XMMatrixRotationX(m_gloryKillArmBaseRot.x);
            DirectX::XMMATRIX baseRotY = DirectX::XMMatrixRotationY(m_gloryKillArmBaseRot.y);
            DirectX::XMMATRIX baseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmBaseRot.z);
            DirectX::XMMATRIX baseRot = baseRotX * baseRotY * baseRotZ;

            // アニメーション回転（グローリーキルの動き）
            DirectX::XMMATRIX animRotX = DirectX::XMMatrixRotationX(m_gloryKillArmAnimRot.x);
            DirectX::XMMATRIX animRotY = DirectX::XMMatrixRotationY(m_gloryKillArmAnimRot.y);
            DirectX::XMMATRIX animRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmAnimRot.z);
            DirectX::XMMATRIX animRot = animRotX * animRotY * animRotZ;

            // カメラ追従回転
            DirectX::XMMATRIX cameraRot = DirectX::XMMatrixRotationY(cameraYaw);
            DirectX::XMMATRIX armTrans = DirectX::XMMatrixTranslationFromVector(armWorldPos);

            // 合成：スケール → ベース補正 → アニメーション → カメラ追従 → 移動
            DirectX::XMMATRIX armWorld = armScale * baseRot * animRot * cameraRot * armTrans;

            m_gloryKillArmModel->SetBoneScaleByPrefix("L_", 0.0f);

            m_d3dContext->RSSetState(m_states->CullNone());

            m_gloryKillArmModel->DrawWithBoneScale(
                m_d3dContext.Get(),
                armWorld,
                viewMatrix,
                projectionMatrix,
                DirectX::Colors::White
            );


            // ============================================
            // === ナイフ描画 ===
            // ============================================
            if (m_gloryKillKnifeModel)
            {
                // オフセットベクトルを作成（ローカル空間）
                DirectX::XMVECTOR offset = DirectX::XMVectorSet(
                    m_gloryKillKnifeOffset.z,   // X = Left/Right
                    m_gloryKillKnifeOffset.y,   // Y = Up/Down  
                    m_gloryKillKnifeOffset.x,   // Z = Forward
                    0.0f
                );

                // アニメーション回転 + カメラ回転でオフセットを変換
                DirectX::XMMATRIX offsetRotation = animRot * cameraRot;
                DirectX::XMVECTOR rotatedOffset = DirectX::XMVector3TransformNormal(offset, offsetRotation);

                // 腕の位置に変換後のオフセットを加える
                DirectX::XMVECTOR knifeWorldPos = DirectX::XMVectorAdd(armWorldPos, rotatedOffset);
                // ナイフのスケール
                DirectX::XMMATRIX knifeScale = DirectX::XMMatrixScaling(
                    m_gloryKillKnifeScale, m_gloryKillKnifeScale, m_gloryKillKnifeScale);

                // ナイフのベース回転
                DirectX::XMMATRIX knifeBaseRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeBaseRot.x);
                DirectX::XMMATRIX knifeBaseRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeBaseRot.y);
                DirectX::XMMATRIX knifeBaseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeBaseRot.z);
                DirectX::XMMATRIX knifeBaseRot = knifeBaseRotX * knifeBaseRotY * knifeBaseRotZ;

                // ナイフのアニメーション回転
                DirectX::XMMATRIX knifeAnimRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeAnimRot.x);
                DirectX::XMMATRIX knifeAnimRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeAnimRot.y);
                DirectX::XMMATRIX knifeAnimRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeAnimRot.z);
                DirectX::XMMATRIX knifeAnimRot = knifeAnimRotX * knifeAnimRotY * knifeAnimRotZ;

                // ナイフの移動行列
                DirectX::XMMATRIX knifeTrans = DirectX::XMMatrixTranslationFromVector(knifeWorldPos);

                // 合成
                DirectX::XMMATRIX knifeWorld = knifeScale * knifeBaseRot * knifeAnimRot * cameraRot * knifeTrans;

                m_gloryKillKnifeModel->Draw(
                    m_d3dContext.Get(),
                    knifeWorld,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::Colors::White
                );
            }

            // === グローリーキルターゲットの敵を最後に描画（腕の後ろに表示） ===
            if (m_gloryKillTargetEnemy)
            {
                DrawSingleEnemy(*m_gloryKillTargetEnemy, viewMatrix, projectionMatrix);
            }

        }
        m_bloodSystem->DrawScreenBlood(m_outputWidth, m_outputHeight);
        DrawHealthPickups(viewMatrix, projectionMatrix);
        m_bloodSystem->DrawBloodDecals(viewMatrix, projectionMatrix, m_player->GetPosition());

        m_gpuParticles->Draw(viewMatrix, projectionMatrix, m_player->GetPosition());
        //  流体レンダリング: シーンをコピーしてからDrawFluid
        /*m_d3dContext->CopyResource(m_sceneCopyTex.Get(), m_offscreenTexture.Get());
        m_gpuParticles->DrawFluid(viewMatrix, projectionMatrix, m_player->GetPosition(),
            nullptr, m_offscreenRTV.Get());*/

        // UI描画（オフスクリーンへ）
        DrawUI();
        DrawScorePopups();
        RenderWaveBanner();
        RenderScoreHUD();

        //  最終画面にブラーを適用して描画
        ID3D11RenderTargetView* finalRTV = m_renderTargetView.Get();
        m_d3dContext->OMSetRenderTargets(1, &finalRTV, nullptr);

        // 最終画面をクリア
        m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

        // ブラーパラメータを設定
        BlurParams params;
        params.texelSize = DirectX::XMFLOAT2(1.0f / m_outputWidth, 1.0f / m_outputHeight);
        params.blurStrength = m_gloryKillDOFIntensity * 5.0f;
        params.focalDepth = m_gloryKillFocalDepth;  // // padding → focalDepthに変更

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        m_d3dContext->Map(m_blurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &params, sizeof(BlurParams));
        m_d3dContext->Unmap(m_blurConstantBuffer.Get(), 0);

        // シェーダーリソースとサンプラーを設定(深度テクスチャ//）
        ID3D11ShaderResourceView* srvs[2] = {
        m_offscreenSRV.Get(),      // t0: カラー
        m_offscreenDepthSRV.Get()  // t1: 深度 
        };
        m_d3dContext->PSSetShaderResources(0, 2, srvs);

        ID3D11SamplerState* sampler = m_linearSampler.Get();
        m_d3dContext->PSSetSamplers(0, 1, &sampler);

        ID3D11Buffer* cb = m_blurConstantBuffer.Get();
        m_d3dContext->PSSetConstantBuffers(0, 1, &cb);

        // レンダーステート設定
        m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
        m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
        m_d3dContext->RSSetState(m_states->CullNone());

        // フルスクリーン四角形を描画（ブラー適用）
        DrawFullscreenQuad();

        // リソースをクリア（深度も含める）
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        m_d3dContext->PSSetShaderResources(0, 2, nullSRVs);
    }
    else
    {
        // ============================================================
        // === 通常時：ブラーなし ===
        // ============================================================

        Clear();

        // カメラ設定
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        playerPos.y += m_player->GetLandingCameraOffset();  // 着地演出：カメラを沈ませる

        DirectX::XMFLOAT3 shakeOffset(0.0f, 0.0f, 0.0f);
        if (m_cameraShakeTimer > 0.0f)
        {
            shakeOffset.x = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.y = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
            shakeOffset.z = ((float)rand() / RAND_MAX - 0.5f) * m_cameraShake;
        }

        DirectX::XMVECTOR cameraPosition;
        DirectX::XMVECTOR cameraTarget;

        if (m_gloryKillCameraActive)
        {
            cameraPosition = DirectX::XMLoadFloat3(&m_gloryKillCameraPos);
            cameraTarget = DirectX::XMLoadFloat3(&m_gloryKillCameraTarget);
        }
        else
        {
            cameraPosition = DirectX::XMVectorSet(
                playerPos.x + shakeOffset.x,
                playerPos.y + shakeOffset.y,
                playerPos.z + shakeOffset.z,
                0.0f
            );

            cameraTarget = DirectX::XMVectorSet(
                playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
                playerPos.y - sinf(playerRot.x),
                playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
                0.0f
            );
        }

        DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

        float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
        DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
            DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
        );

        // マップ描画
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }

        DrawNavGridDebug(viewMatrix, projectionMatrix);
        DrawSlicedPieces(viewMatrix, projectionMatrix);

        ////  地面の苔を描画
        //    if (m_furRenderer && m_furReady)
        //    {
        //        m_furRenderer->DrawGroundMoss(
        //            m_d3dContext.Get(),
        //            viewMatrix,
        //            projectionMatrix,
        //            m_accumulatedAnimTime  // 経過時間（風の揺れ用）
        //        );
        //    }

        //// 武器スポーンのテクスチャ描画
        //if (m_weaponSpawnSystem)
        //{
        //    m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        //}

        // 3D描画（優先順位順）
        //DrawParticles();
        DrawEnemies(viewMatrix, projectionMatrix);

        DrawKeyPrompt(viewMatrix, projectionMatrix);

        //DrawBillboard();
        DrawWeapon();
        DrawShield();



        //  シールドトレイル描画
        //  Effekseer描画
        if (m_effekseerRenderer != nullptr && m_effekseerManager != nullptr)
        {
            Effekseer::Matrix44 efkView;
            Effekseer::Matrix44 efkProj;

            DirectX::XMFLOAT4X4 viewF, projF;
            DirectX::XMStoreFloat4x4(&viewF, viewMatrix);
            DirectX::XMStoreFloat4x4(&projF, projectionMatrix);
            memcpy(&efkView, &viewF, sizeof(float) * 16);
            memcpy(&efkProj, &projF, sizeof(float) * 16);

            m_effekseerRenderer->SetCameraMatrix(efkView);
            m_effekseerRenderer->SetProjectionMatrix(efkProj);

            m_effekseerRenderer->BeginRendering();
            m_effekseerManager->Draw();
            m_effekseerRenderer->EndRendering();
        }

        //DrawWeaponSpawns();

        DrawGibs(viewMatrix, projectionMatrix);

        // グローリーキル腕・ナイフ描画（FBXモデル版）
        if ((m_gloryKillArmAnimActive || m_debugShowGloryKillArm) && m_gloryKillArmModel)
        {
            DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
            forward = DirectX::XMVector3Normalize(forward);

            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
            right = DirectX::XMVector3Normalize(right);
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // === アームの位置計算（調整変数使用） ===
            DirectX::XMVECTOR armWorldPos = cameraPosition;
            armWorldPos += forward * m_gloryKillArmOffset.x;  // 前方
            armWorldPos += up * m_gloryKillArmOffset.y;       // 上下
            armWorldPos += right * m_gloryKillArmOffset.z;    // 左右

            float cameraYaw = atan2f(
                DirectX::XMVectorGetX(forward),
                DirectX::XMVectorGetZ(forward)
            );

            // === アームのワールド行列 ===
            DirectX::XMMATRIX armScale = DirectX::XMMatrixScaling(
                m_gloryKillArmScale, m_gloryKillArmScale, m_gloryKillArmScale);

            // ベース回転（モデルを正しい向きに補正）
            DirectX::XMMATRIX baseRotX = DirectX::XMMatrixRotationX(m_gloryKillArmBaseRot.x);
            DirectX::XMMATRIX baseRotY = DirectX::XMMatrixRotationY(m_gloryKillArmBaseRot.y);
            DirectX::XMMATRIX baseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmBaseRot.z);
            DirectX::XMMATRIX baseRot = baseRotX * baseRotY * baseRotZ;

            // アニメーション回転（グローリーキルの動き）
            DirectX::XMMATRIX animRotX = DirectX::XMMatrixRotationX(m_gloryKillArmAnimRot.x);
            DirectX::XMMATRIX animRotY = DirectX::XMMatrixRotationY(m_gloryKillArmAnimRot.y);
            DirectX::XMMATRIX animRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmAnimRot.z);
            DirectX::XMMATRIX animRot = animRotX * animRotY * animRotZ;

            // カメラ追従回転
            DirectX::XMMATRIX cameraRot = DirectX::XMMatrixRotationY(cameraYaw);
            DirectX::XMMATRIX armTrans = DirectX::XMMatrixTranslationFromVector(armWorldPos);

            // 合成：スケール → ベース補正 → アニメーション → カメラ追従 → 移動
            DirectX::XMMATRIX armWorld = armScale * baseRot * animRot * cameraRot * armTrans;

            m_gloryKillArmModel->SetBoneScaleByPrefix("L_", 0.0f);

            m_d3dContext->RSSetState(m_states->CullNone());

            m_gloryKillArmModel->DrawWithBoneScale(
                m_d3dContext.Get(),
                armWorld,
                viewMatrix,
                projectionMatrix,
                DirectX::Colors::White
            );

            // ============================================
            // === ナイフ描画 ===
            // ============================================
            if (m_gloryKillKnifeModel)
            {
                // オフセットベクトルを作成（ローカル空間）
                DirectX::XMVECTOR offset = DirectX::XMVectorSet(
                    m_gloryKillKnifeOffset.z,   // X = Left/Right
                    m_gloryKillKnifeOffset.y,   // Y = Up/Down  
                    m_gloryKillKnifeOffset.x,   // Z = Forward
                    0.0f
                );

                // アニメーション回転 + カメラ回転でオフセットを変換
                DirectX::XMMATRIX offsetRotation = animRot * cameraRot;
                DirectX::XMVECTOR rotatedOffset = DirectX::XMVector3TransformNormal(offset, offsetRotation);

                // 腕の位置に変換後のオフセットを加える
                DirectX::XMVECTOR knifeWorldPos = DirectX::XMVectorAdd(armWorldPos, rotatedOffset);

                // ナイフのスケール
                DirectX::XMMATRIX knifeScale = DirectX::XMMatrixScaling(
                    m_gloryKillKnifeScale, m_gloryKillKnifeScale, m_gloryKillKnifeScale);

                // ナイフのベース回転
                DirectX::XMMATRIX knifeBaseRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeBaseRot.x);
                DirectX::XMMATRIX knifeBaseRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeBaseRot.y);
                DirectX::XMMATRIX knifeBaseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeBaseRot.z);
                DirectX::XMMATRIX knifeBaseRot = knifeBaseRotX * knifeBaseRotY * knifeBaseRotZ;

                // ナイフのアニメーション回転
                DirectX::XMMATRIX knifeAnimRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeAnimRot.x);
                DirectX::XMMATRIX knifeAnimRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeAnimRot.y);
                DirectX::XMMATRIX knifeAnimRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeAnimRot.z);
                DirectX::XMMATRIX knifeAnimRot = knifeAnimRotX * knifeAnimRotY * knifeAnimRotZ;

                // ナイフの移動行列
                DirectX::XMMATRIX knifeTrans = DirectX::XMMatrixTranslationFromVector(knifeWorldPos);

                // 合成
                DirectX::XMMATRIX knifeWorld = knifeScale * knifeBaseRot * knifeAnimRot * cameraRot * knifeTrans;

                m_gloryKillKnifeModel->Draw(
                    m_d3dContext.Get(),
                    knifeWorld,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::Colors::White
                );
            }

            if (m_gloryKillTargetEnemy)
            {
                DrawSingleEnemy(*m_gloryKillTargetEnemy, viewMatrix, projectionMatrix);
            }

        }
        m_bloodSystem->DrawScreenBlood(m_outputWidth, m_outputHeight);
        DrawHealthPickups(viewMatrix, projectionMatrix);
        m_bloodSystem->DrawBloodDecals(viewMatrix, projectionMatrix, m_player->GetPosition());

        m_gpuParticles->Draw(viewMatrix, projectionMatrix, m_player->GetPosition());        //  流体レンダリング: バックバッファをコピーしてからDrawFluid
        /* m_gpuParticles->DrawFluid(viewMatrix, projectionMatrix, m_player->GetPosition(),
             nullptr, m_renderTargetView.Get());*/

        // UI描画
        DrawUI();
        DrawScorePopups();
        RenderWaveBanner();
        RenderScoreHUD();

    }


    //  瀕死日ネット
    RenderLowHealthVignette();

    RenderClawDamage();

    //  スピードライン
    RenderSpeedLines();

    //  ダッシュオーバーレイ
    //RenderDashOverlay();

    //  弾切れ警告
    RenderReloadWarning();

    // デバッグ描画
    DrawHitboxes();
    DrawBulletTracers();
    DrawDebugUI();
}

// 【UI】すべてのUIを描画する

// ============================================================
//  DrawEnemies - 全敵の描画（インスタンス描画対応）
//
//  【描画方式】
//  - Normal/Runner/Tank: インスタンスバッファで一括描画
//    （同じモデルの敵を1回のDrawCallで全て描画）
//  - MidBoss/Boss: 単体描画（DrawSingleEnemy）
//  - グローリーキル対象: skipGloryKillTargetで制御
//
//  【約1,260行ある理由】
//  インスタンスバッファ構築 + アニメーション更新 +
//  タイプ別のシェーダー設定が全てこの関数に詰まっている。
//  将来的にはEnemyRendererクラスに分離すべき。
// ============================================================
void Game::DrawEnemies(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix, bool skipGloryKillTarget)
{
    static int frameCount = 0;
    frameCount++;

    ////	===	1秒ごとにデバッグ情報を出力	===
    //if (frameCount % 60 == 0)
    //{
    //    auto& enemies = m_enemySystem->GetEnemies();

    //    char debugMsg[512];
    //    sprintf_s(debugMsg, "=== Enemies: %zu ===\n", enemies.size());
    //    //OutputDebugStringA(debugMsg);
    //}

    //  深度書き込みを強制的に有効化
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    DirectX::XMFLOAT3 lightDir = { 1.0f, -1.0f, 1.0f };

    //	========================================================================
    //  インスタンスデータを作成
    //	========================================================================

    m_normalDead.clear();
    m_normalDeadHeadless.clear();
    m_runnerDead.clear();
    m_runnerDeadHeadless.clear();
    m_tankAttackingHeadless.clear();
    m_tankDead.clear();
    m_tankDeadHeadless.clear();
    m_midBossWalking.clear();
    m_midBossAttacking.clear();
    m_midBossWalkingHeadless.clear();
    m_midBossAttackingHeadless.clear();
    m_midBossDead.clear();
    m_midBossDeadHeadless.clear();
    m_bossWalking.clear();
    m_bossAttackingJump.clear();
    m_bossAttackingSlash.clear();
    m_bossAttackingJumpHeadless.clear();
    m_bossAttackingSlashHeadless.clear();
    m_bossWalkingHeadless.clear();
    m_bossDead.clear();
    m_bossDeadHeadless.clear();

    //  === 死亡アニメーション再生時間の取得    ===
    float deathDuration = m_enemyModel->GetAnimationDuration("Death");

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive && !enemy.isDying)
            continue;

        if (enemy.isExploded)
            continue;

        if (skipGloryKillTarget && m_gloryKillTargetEnemy && enemy.id == m_gloryKillTargetEnemy->id)
            continue;

        // ワールド行列を計算
        float s = 0.01f;
        if (enemy.type == EnemyType::BOSS) s = 0.015f;  // 1.5倍
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(s, s, s);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
            enemy.position.x,
            enemy.position.y,
            enemy.position.z
        );
        DirectX::XMMATRIX world = scale * rotation * translation;

        // ========================================================================
        //  死亡中の処理分岐
        // ========================================================================
        if (enemy.isDying)
        {
            // アニメーション終了判定（終端から0.1秒以内なら終了とみなす）
            bool animationFinished = (enemy.animationTime >= deathDuration - 0.1f);

            if (animationFinished)
            {
                // MIDBOSS/BOSSだけインスタンスデータが必要
                if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS ||
                    (enemy.type == EnemyType::TANK && enemy.headDestroyed && enemy.currentAnimation == "Attack"))
                {
                    InstanceData instance;
                    DirectX::XMStoreFloat4x4(&instance.world, world);

                    if (enemy.isStaggered)
                    {
                        //  リムライト用: Color.a にスタガー情報を仕込む
                         // a > 1.5 → PSでスタガーと判定
                         // frac(a) → パルスアニメーションの位相
                        float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                        float rimBase = 2.0f;
                        switch (enemy.type)
                        {
                        case EnemyType::NORMAL:  rimBase = 2.0f; break;
                        case EnemyType::RUNNER:  rimBase = 2.0f; break;
                        case EnemyType::TANK:    rimBase = 3.0f; break;
                        case EnemyType::MIDBOSS: rimBase = 4.0f; break;
                        case EnemyType::BOSS:    rimBase = 5.0f; break;
                        }
                        float rimAlpha = rimBase + phase * 0.1f;
                        // 色は通常色を維持（紫のリムライトはシェーダーが足す）
                        instance.color = DirectX::XMFLOAT4(
                            enemy.color.x,
                            enemy.color.y,
                            enemy.color.z,
                            rimAlpha
                        );
                    }
                    else
                    {
                        instance.color = enemy.color;
                    }

                    switch (enemy.type)
                    {
                    case EnemyType::TANK:
                        m_tankAttackingHeadless.push_back(instance);
                        break;
                    case EnemyType::MIDBOSS:
                        if (enemy.headDestroyed)
                        {
                            if (enemy.currentAnimation == "Attack")
                                m_midBossAttackingHeadless.push_back(instance);
                            else
                                m_midBossWalkingHeadless.push_back(instance);
                        }
                        else
                        {
                            if (enemy.currentAnimation == "Attack")
                                m_midBossAttacking.push_back(instance);
                            else
                                m_midBossWalking.push_back(instance);
                        }
                        break;
                    case EnemyType::BOSS:
                        if (enemy.headDestroyed)
                        {
                            if (enemy.currentAnimation == "AttackJump")
                                m_bossAttackingJumpHeadless.push_back(instance);
                            else if (enemy.currentAnimation == "AttackSlash")
                                m_bossAttackingSlashHeadless.push_back(instance);
                            else
                                m_bossWalkingHeadless.push_back(instance);
                        }
                        else
                        {
                            if (enemy.currentAnimation == "AttackJump")
                                m_bossAttackingJump.push_back(instance);
                            else if (enemy.currentAnimation == "AttackSlash")
                                m_bossAttackingSlash.push_back(instance);
                            else
                                m_bossWalking.push_back(instance);
                        }
                        break;
                    }
                }


            }
            else
            {
                // ========================================================
                // アニメーション再生中 → 個別描画
                // ========================================================
                float headScale = enemy.headDestroyed ? 0.0f : 1.0f;
                m_enemyModel->SetBoneScale("Head", headScale);

                m_enemyModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::XMLoadFloat4(&enemy.color),
                    "Death",
                    enemy.animationTime  // ← 個別の時間
                );

                m_enemyModel->SetBoneScale("Head", 1.0f);
            }

            // 影を描画（近い敵だけ）
            if (m_shadow)
            {
                float sdx = enemy.position.x - playerPos.x;
                float sdz = enemy.position.z - playerPos.z;
                float shadowDist2 = sdx * sdx + sdz * sdz;
                if (shadowDist2 < 400.0f)  // 20m以内だけ影描画
                {
                    m_shadow->RenderShadow(
                        m_d3dContext.Get(),
                        enemy.position,
                        1.5f,
                        viewMatrix,
                        projectionMatrix,
                        lightDir,
                        0.0f
                    );
                }
            }



            continue;  // 生きている敵の処理はスキップ
        }
        //=== スタンリング描画 ===
        if (m_stunRing && enemy.isStaggered && enemy.isAlive && !enemy.isDying)
        {
            float sdx = enemy.position.x - playerPos.x;
            float sdz = enemy.position.z - playerPos.z;
            float distSq = sdx * sdx + sdz * sdz;

            //  20m以上は描画しない、15?20mはフェードアウト
            if (distSq < 300.0f)  // 20m以内
            {
                float ringSize = 1.5f;
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  ringSize = 1.2f;  break;
                case EnemyType::RUNNER:  ringSize = 1.0f;  break;
                case EnemyType::TANK:    ringSize = 1.8f;  break;
                case EnemyType::MIDBOSS: ringSize = 2.2f;  break;
                case EnemyType::BOSS:    ringSize = 3.0f;  break;
                }

                //  15?20mで徐々に縮小（フェードアウト）
                float dist = sqrtf(distSq);
                float fadeStart = 12.0f;  // ここからフェード開始
                float fadeEnd = 17.3f;    // ここで完全に消える
                if (dist > fadeStart)
                {
                    float fade = 1.0f - (dist - fadeStart) / (fadeEnd - fadeStart);
                    ringSize *= fade;  // 遠いほど小さく→自然に消える
                }

                DirectX::XMFLOAT3 ringPos = enemy.position;
                ringPos.y += 0.9f;

                m_stunRing->Render(
                    m_d3dContext.Get(),
                    ringPos,
                    ringSize,
                    enemy.staggerTimer,
                    viewMatrix,
                    projectionMatrix
                );
            }

            // 敵タイプ別のリングサイズ
        }

        // ========================================================================
        // 生きている敵はインスタンスリストへ//
        // ========================================================================
        InstanceData instance;
        DirectX::XMStoreFloat4x4(&instance.world, world);

        //  === よろめき状態なら点滅させる   ===
        if (enemy.isStaggered)
        {
            //  リムライト用: Color.a にスタガー情報を仕込む
             // a > 1.5 → PSでスタガーと判定
             // frac(a) → パルスアニメーションの位相
            float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
            float rimBase = 2.0f;
            switch (enemy.type)
            {
            case EnemyType::NORMAL:  rimBase = 2.0f; break;
            case EnemyType::RUNNER:  rimBase = 2.0f; break;
            case EnemyType::TANK:    rimBase = 3.0f; break;
            case EnemyType::MIDBOSS: rimBase = 4.0f; break;
            case EnemyType::BOSS:    rimBase = 5.0f; break;
            }
            float rimAlpha = rimBase + phase * 0.1f;
            // 色は通常色を維持（紫のリムライトはシェーダーが足す）
            instance.color = DirectX::XMFLOAT4(
                enemy.color.x,
                enemy.color.y,
                enemy.color.z,
                rimAlpha
            );
        }
        else
        {
            instance.color = enemy.color;
        }

        switch (enemy.type)
        {
        case EnemyType::NORMAL:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                {
                }//normalAttackingHeadless.push_back(instance);
                else
                {
                }//}normalWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                {
                }//}normalAttacking.push_back(instance);
                else
                {
                }//normalWalking.push_back(instance);
            }
            break;

        case EnemyType::RUNNER:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                {
                }//runnerAttackingHeadless.push_back(instance);
                else
                {
                }//runnerWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                {
                }//runnerAttacking.push_back(instance);
                else
                {
                }//runnerWalking.push_back(instance);
            }
            break;

        case EnemyType::TANK:
            if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                    m_tankAttackingHeadless.push_back(instance);
                else
                {
                }//tankWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                {
                }//tankAttacking.push_back(instance);
                else
                {
                }//tankWalking.push_back(instance);
            }
            break;

            //MIDBOSS
        case EnemyType::MIDBOSS:
            //  スタガー中は個別描画
            if (enemy.isStaggered)
            {
                std::string stunAnim = m_midBossModel->HasAnimation("Stun") ? "Stun" : "Idle";
                float animTime = m_midBossModel->HasAnimation("Stun")
                    ? enemy.animationTime
                    : 0.0f;
                m_midBossModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world, viewMatrix, projectionMatrix,
                    DirectX::XMLoadFloat4(&instance.color),
                    stunAnim, animTime
                );
            }
            else if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "Attack")
                    m_midBossAttackingHeadless.push_back(instance);
                else
                    m_midBossWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "Attack")
                    m_midBossAttacking.push_back(instance);
                else
                    m_midBossWalking.push_back(instance);
            }
            break;

        case EnemyType::BOSS:
            //  スタガー中はインスタンシングせず個別描画（Idleで静止）
            if (enemy.isStaggered)
            {
                std::string stunAnim = m_bossModel->HasAnimation("Stun") ? "Stun" : "Idle";
                float animTime = m_bossModel->HasAnimation("Stun")
                    ? enemy.animationTime
                    : 0.0f;  // Idleの先頭フレームで固定
                m_bossModel->DrawAnimated(
                    m_d3dContext.Get(),
                    world, viewMatrix, projectionMatrix,
                    DirectX::XMLoadFloat4(&instance.color),
                    stunAnim, animTime
                );
            }
            else if (enemy.headDestroyed)
            {
                if (enemy.currentAnimation == "AttackJump")
                    m_bossAttackingJumpHeadless.push_back(instance);
                else if (enemy.currentAnimation == "AttackSlash")
                    m_bossAttackingSlashHeadless.push_back(instance);
                else
                    m_bossWalkingHeadless.push_back(instance);
            }
            else
            {
                if (enemy.currentAnimation == "AttackJump")
                    m_bossAttackingJump.push_back(instance);
                else if (enemy.currentAnimation == "AttackSlash")
                    m_bossAttackingSlash.push_back(instance);
                else
                    m_bossWalking.push_back(instance);
            }
            break;
        }

        // 影を描画（近い敵だけ）
        if (m_shadow)
        {
            float sdx = enemy.position.x - playerPos.x;
            float sdz = enemy.position.z - playerPos.z;
            float shadowDist2 = sdx * sdx + sdz * sdz;
            if (shadowDist2 < 400.0f)  // 20m以内だけ影描画
            {
                m_shadow->RenderShadow(
                    m_d3dContext.Get(),
                    enemy.position,
                    1.5f,
                    viewMatrix,
                    projectionMatrix,
                    lightDir,
                    0.0f
                );
            }
        }
    }

    // ========================================================================
    // インスタンシング描画（タイプ別）
    // ========================================================================

    // ====================================================================
    // NORMAL（Y_Bot）の描画
    // ====================================================================
    if (m_enemyModel)
    {
        // ================================================================
        // [PERF] NORMAL: Head ON -  インスタンシング描画
        // ================================================================
        m_enemyModel->SetBoneScale("Head", 1.0f);
        {
            {
                m_instWorlds.clear();   // サイズ0にするだけ。メモリは解放しない！
                m_instColors.clear();
                m_instAnims.clear();
                m_instTimes.clear();

                for (const auto& enemy : m_enemySystem->GetEnemies())
                {
                    if (!enemy.isAlive || enemy.isDying) continue;
                    if (enemy.type != EnemyType::NORMAL) continue;
                    if (enemy.headDestroyed) continue;

                    float s = 0.01f;
                    m_instWorlds.push_back(
                        DirectX::XMMatrixScaling(s, s, s)
                        * DirectX::XMMatrixRotationY(enemy.rotationY)
                        * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                    );

                    DirectX::XMFLOAT4 finalColor = enemy.color;
                    if (enemy.isStaggered)
                    {
                        float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                        finalColor.w = 2.0f + phase;
                    }
                    m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                    m_instAnims.push_back(enemy.currentAnimation);
                    m_instTimes.push_back(enemy.animationTime);
                }

                if (!m_instWorlds.empty())
                {
                    m_enemyModel->DrawInstanced_Custom(
                        m_d3dContext.Get(), viewMatrix, projectionMatrix,
                        m_instWorlds, m_instColors, m_instAnims, m_instTimes);
                }
            }
        }

        // Dead - head on (instanced)
        if (!m_normalDead.empty())
        {
            float finalTime = deathDuration - 0.001f;
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), m_normalDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_enemyModel->SetBoneScale("Head", 0.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::NORMAL) continue;
                if (!enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_enemyModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - headless (instanced)
        if (!m_normalDeadHeadless.empty())
        {
            float finalTime = deathDuration - 0.001f;
            m_enemyModel->DrawInstanced(m_d3dContext.Get(), m_normalDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_enemyModel->SetBoneScale("Head", 1.0f);
    }


    // ====================================================================
    // RUNNER
    // ====================================================================
    if (m_runnerModel)
    {
        // ================================================================
        // [PERF] RUNNER: Head ON - walking+attacking unified loop
        // ================================================================
        m_runnerModel->SetBoneScale("Head", 1.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::RUNNER) continue;
                if (enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_runnerModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - head on (instanced)
        if (!m_runnerDead.empty())
        {
            float finalTime = m_runnerModel->GetAnimationDuration("Death") - 0.001f;
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), m_runnerDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // ================================================================
        // [PERF] RUNNER: Headless - walking+attacking unified loop
        // ================================================================
        m_runnerModel->SetBoneScale("Head", 0.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::RUNNER) continue;
                if (!enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_runnerModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - headless (instanced)
        if (!m_runnerDeadHeadless.empty())
        {
            float finalTime = m_runnerModel->GetAnimationDuration("Death") - 0.001f;
            m_runnerModel->DrawInstanced(m_d3dContext.Get(), m_runnerDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_runnerModel->SetBoneScale("Head", 1.0f);
    }


    // ====================================================================
    // TANK
    // ====================================================================
    if (m_tankModel)
    {
        // ================================================================
        // [PERF] TANK: Head ON - walking+attacking unified loop
        // ================================================================
        m_tankModel->SetBoneScale("Head", 1.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::TANK) continue;
                if (enemy.headDestroyed) continue;

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_tankModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Dead - head on (instanced)
        if (!m_tankDead.empty())
        {
            float finalTime = m_tankModel->GetAnimationDuration("Death") - 0.001f;
            m_tankModel->DrawInstanced(m_d3dContext.Get(), m_tankDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // ================================================================
        // [PERF] TANK: Headless - walking unified loop
        // ================================================================
        m_tankModel->SetBoneScale("Head", 0.0f);
        {
            m_instWorlds.clear();
            m_instColors.clear();
            m_instAnims.clear();
            m_instTimes.clear();

            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type != EnemyType::TANK) continue;
                if (!enemy.headDestroyed) continue;
                if (enemy.currentAnimation == "Attack") continue;  // // 元のロジック維持

                float s = 0.01f;
                m_instWorlds.push_back(
                    DirectX::XMMatrixScaling(s, s, s)
                    * DirectX::XMMatrixRotationY(enemy.rotationY)
                    * DirectX::XMMatrixTranslation(enemy.position.x, enemy.position.y, enemy.position.z)
                );

                DirectX::XMFLOAT4 finalColor = enemy.color;
                if (enemy.isStaggered)
                {
                    float phase = fmodf(enemy.staggerFlashTimer * 1.5f, 1.0f);
                    finalColor.w = 2.0f + phase;
                }
                m_instColors.push_back(DirectX::XMLoadFloat4(&finalColor));
                m_instAnims.push_back(enemy.currentAnimation);
                m_instTimes.push_back(enemy.animationTime);
            }

            if (!m_instWorlds.empty())
            {
                m_tankModel->DrawInstanced_Custom(
                    m_d3dContext.Get(), viewMatrix, projectionMatrix,
                    m_instWorlds, m_instColors, m_instAnims, m_instTimes);
            }
        }

        // Attacking headless (instanced with shared timer)
        if (!m_tankAttackingHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[2],
                m_tankModel->GetAnimationDuration("Attack"));
            m_tankModel->DrawInstanced(m_d3dContext.Get(), m_tankAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - headless (instanced)
        if (!m_tankDeadHeadless.empty())
        {
            float finalTime = m_tankModel->GetAnimationDuration("Death") - 0.001f;
            m_tankModel->DrawInstanced(m_d3dContext.Get(), m_tankDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_tankModel->SetBoneScale("Head", 1.0f);
    }

    // ====================================================================
    // MIDBOSS の描画
    // ====================================================================
    if (m_midBossModel)
    {
        m_midBossModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!m_midBossWalking.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[3],
                m_midBossModel->GetAnimationDuration("Walk"));
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - 頭あり
        if (!m_midBossAttacking.empty())
        {
            float duration = m_midBossModel->GetAnimationDuration("Attack");
            float attackTime = fmod(m_typeAttackTimer[3], duration);

            // ビーム発射中 or リカバリー中 → 最終フレーム固定
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive &&
                    (enemy.bossPhase == BossAttackPhase::ROAR_FIRE ||
                        enemy.bossPhase == BossAttackPhase::ROAR_RECOVERY))
                {
                    attackTime = duration - 0.01f;  // 最後のフレーム
                    break;
                }
            }

            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - 頭あり
        if (!m_midBossDead.empty())
        {
            float finalTime = m_midBossModel->GetAnimationDuration("Death") - 0.001f;
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // 頭なし描画
        m_midBossModel->SetBoneScale("Head", 0.0f);

        if (!m_midBossAttacking.empty())
        {
            float duration = m_midBossModel->GetAnimationDuration("Attack");
            float attackTime = fmod(m_typeAttackTimer[3], duration);

            // ビーム発射中 or リカバリー中 → 最終フレーム固定
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive &&
                    (enemy.bossPhase == BossAttackPhase::ROAR_FIRE ||
                        enemy.bossPhase == BossAttackPhase::ROAR_RECOVERY))
                {
                    attackTime = duration - 0.01f;  // 最後のフレーム
                    break;
                }
            }

            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        if (!m_midBossAttackingHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[3],
                m_midBossModel->GetAnimationDuration("Attack"));
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttackingHeadless,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        if (!m_midBossDeadHeadless.empty())
        {
            float finalTime = m_midBossModel->GetAnimationDuration("Death") - 0.001f;
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_midBossModel->SetBoneScale("Head", 1.0f);
    }

    // ====================================================================
    // BOSS の描画
    // ====================================================================
    if (m_bossModel)
    {
        m_bossModel->SetBoneScale("Head", 1.0f);

        // Walking - 頭あり
        if (!m_bossWalking.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[4],
                m_bossModel->GetAnimationDuration("Walk"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // ジャンプ叩き
        if (!m_bossAttackingJump.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackJump"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingJump,
                viewMatrix, projectionMatrix, "AttackJump", attackTime);
        }

        // 殴り上げ斬撃
        if (!m_bossAttackingSlash.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackSlash"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingSlash,
                viewMatrix, projectionMatrix, "AttackSlash", attackTime);
        }

        // Dead - 頭あり
        if (!m_bossDead.empty())
        {
            float finalTime = m_bossModel->GetAnimationDuration("Death") - 0.001f;
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // --- Headless variants ---
        m_bossModel->SetBoneScale("Head", 0.0f);

        if (!m_bossWalkingHeadless.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[4],
                m_bossModel->GetAnimationDuration("Walk"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossWalkingHeadless,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        if (!m_bossAttackingJumpHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackJump"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingJumpHeadless,
                viewMatrix, projectionMatrix, "AttackJump", attackTime);
        }

        if (!m_bossAttackingSlashHeadless.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackSlash"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingSlashHeadless,
                viewMatrix, projectionMatrix, "AttackSlash", attackTime);
        }


        if (!m_bossDeadHeadless.empty())
        {
            float finalTime = m_bossModel->GetAnimationDuration("Death") - 0.001f;
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossDeadHeadless,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        m_bossModel->SetBoneScale("Head", 1.0f);
    }

    // === ターゲットマーカー描画 ===               // ★ここに//（4377行目）
    if (m_targetMarker && m_shieldState == ShieldState::Guarding && m_guardLockTargetID >= 0)
    {
        float markerSize = 1.5f;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.id == m_guardLockTargetID && enemy.isAlive)
            {
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  markerSize = 1.2f;  break;
                case EnemyType::RUNNER:  markerSize = 1.0f;  break;
                case EnemyType::TANK:    markerSize = 1.8f;  break;
                case EnemyType::MIDBOSS: markerSize = 2.5f;  break;
                case EnemyType::BOSS:    markerSize = 3.5f;  break;
                }
                break;
            }
        }

        DirectX::XMFLOAT3 markerPos = m_guardLockTargetPos;
        markerPos.y += 1.0f;

        m_targetMarker->Render(
            m_d3dContext.Get(),
            markerPos,
            markerSize,
            m_gameTime,
            viewMatrix,
            projectionMatrix
        );
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

    // 両面描画（ビルボードの裏表問題を回避）
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;  // カリングしない
    rsDesc.DepthClipEnable = TRUE;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> noCullRS;
    m_d3dDevice->CreateRasterizerState(&rsDesc, &noCullRS);
    context->RSSetState(noCullRS.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // カメラの右方向・上方向を計算（ビルボード用）
    //DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMFLOAT3 pRot = m_player->GetRotation();
    DirectX::XMVECTOR camDir = DirectX::XMVectorSet(
        sinf(pRot.y) * cosf(pRot.x),
        -sinf(pRot.x),
        cosf(pRot.y) * cosf(pRot.x), 0.0f
    );
    DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR camRight = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(worldUp, camDir)
    );
    DirectX::XMVECTOR camUp = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(camDir, camRight)
    );

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying || enemy.health >= enemy.maxHealth
            || enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
            continue;
        if (enemy.isExploded)
            continue;

        float barWidth = 1.0f;
        float barHeight = 0.08f;
        float healthPercent = (float)enemy.health / enemy.maxHealth;

        // バーの中心（敵の頭上）
        DirectX::XMVECTOR center = DirectX::XMVectorSet(
            enemy.position.x, enemy.position.y + 2.5f, enemy.position.z, 0.0f
        );

        // 右方向と上方向のオフセット
        DirectX::XMVECTOR halfRight = DirectX::XMVectorScale(camRight, barWidth * 0.5f);
        DirectX::XMVECTOR halfUp = DirectX::XMVectorScale(camUp, barHeight * 0.5f);

        // === 背景バー（暗い赤、全幅）===
        DirectX::XMFLOAT3 bg0, bg1, bg2, bg3;
        DirectX::XMStoreFloat3(&bg0, DirectX::XMVectorSubtract(DirectX::XMVectorSubtract(center, halfRight), halfUp));
        DirectX::XMStoreFloat3(&bg1, DirectX::XMVectorSubtract(DirectX::XMVectorAdd(center, halfRight), halfUp));
        DirectX::XMStoreFloat3(&bg2, DirectX::XMVectorAdd(DirectX::XMVectorAdd(center, halfRight), halfUp));
        DirectX::XMStoreFloat3(&bg3, DirectX::XMVectorAdd(DirectX::XMVectorSubtract(center, halfRight), halfUp));

        DirectX::XMFLOAT4 bgColor(0.2f, 0.0f, 0.0f, 0.8f);
        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(bg0, bgColor),
            DirectX::VertexPositionColor(bg1, bgColor),
            DirectX::VertexPositionColor(bg2, bgColor)
        );
        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(bg0, bgColor),
            DirectX::VertexPositionColor(bg2, bgColor),
            DirectX::VertexPositionColor(bg3, bgColor)
        );

        // === HPバー（赤→黄→緑のグラデ、HP割合分の幅）===
       // 背景より手前に描画（Zファイティング回避）
        DirectX::XMVECTOR hpCenter = DirectX::XMVectorSubtract(center, DirectX::XMVectorScale(camDir, 0.01f));

        DirectX::XMVECTOR hpLeftBottom = DirectX::XMVectorSubtract(DirectX::XMVectorSubtract(hpCenter, halfRight), halfUp);
        DirectX::XMVECTOR hpRightBottom = DirectX::XMVectorSubtract(DirectX::XMVectorAdd(DirectX::XMVectorSubtract(hpCenter, halfRight), DirectX::XMVectorScale(camRight, barWidth * healthPercent)), halfUp);
        DirectX::XMVECTOR hpRightTop = DirectX::XMVectorAdd(DirectX::XMVectorAdd(DirectX::XMVectorSubtract(hpCenter, halfRight), DirectX::XMVectorScale(camRight, barWidth * healthPercent)), halfUp);
        DirectX::XMVECTOR hpLeftTop = DirectX::XMVectorAdd(DirectX::XMVectorSubtract(hpCenter, halfRight), halfUp);

        DirectX::XMFLOAT3 hp0, hp1, hp2, hp3;
        DirectX::XMStoreFloat3(&hp0, hpLeftBottom);
        DirectX::XMStoreFloat3(&hp1, hpRightBottom);
        DirectX::XMStoreFloat3(&hp2, hpRightTop);
        DirectX::XMStoreFloat3(&hp3, hpLeftTop);

        // HP割合で色を変える（高=緑、中=黄、低=赤）
        DirectX::XMFLOAT4 hpColor;
        if (healthPercent > 0.5f)
        {
            float t = (healthPercent - 0.5f) * 2.0f;
            hpColor = { 1.0f - t, 1.0f, 0.0f, 1.0f };  // 黄→緑
        }
        else
        {
            float t = healthPercent * 2.0f;
            hpColor = { 1.0f, t, 0.0f, 1.0f };  // 赤→黄
        }

        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(hp0, hpColor),
            DirectX::VertexPositionColor(hp1, hpColor),
            DirectX::VertexPositionColor(hp2, hpColor)
        );
        primitiveBatch->DrawTriangle(
            DirectX::VertexPositionColor(hp0, hpColor),
            DirectX::VertexPositionColor(hp2, hpColor),
            DirectX::VertexPositionColor(hp3, hpColor)
        );

        // === 枠線（白） ===
        DirectX::XMFLOAT4 frameColor(0.8f, 0.8f, 0.8f, 0.6f);
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg0, frameColor),
            DirectX::VertexPositionColor(bg1, frameColor)
        );
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg1, frameColor),
            DirectX::VertexPositionColor(bg2, frameColor)
        );
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg2, frameColor),
            DirectX::VertexPositionColor(bg3, frameColor)
        );
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(bg3, frameColor),
            DirectX::VertexPositionColor(bg0, frameColor)
        );
    }

    // === 収縮リング式・攻撃予告 ===
    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying || enemy.isExploded)
            continue;

        if ((enemy.currentAnimation == "Attack" || enemy.currentAnimation == "AttackSlash") && !enemy.attackJustLanded)
        {
            float hitTime;
            switch (enemy.type)
            {
            case EnemyType::RUNNER:  hitTime = m_enemySystem->m_runnerAttackHitTime; break;
            case EnemyType::TANK:    hitTime = m_enemySystem->m_tankAttackHitTime;   break;
            case EnemyType::MIDBOSS:
            case EnemyType::BOSS:    hitTime = m_enemySystem->m_bossMeleeHitTime;    break;
            default:                 hitTime = m_enemySystem->m_normalAttackHitTime;  break;
            }

            float timeToHit = hitTime - enemy.animationTime;
            float warningWindow = 1.0f;

            if (timeToHit > 0.0f && timeToHit < warningWindow)
            {
                float urgency = 1.0f - (timeToHit / warningWindow);

                // --- リング中心（敵の胸あたり） ---
                DirectX::XMVECTOR ringCenter = DirectX::XMVectorSet(
                    enemy.position.x, enemy.position.y + 1.5f, enemy.position.z, 0.0f
                );

                // --- 収縮リングの半径（大→小に縮む） ---
                float maxRadius = 2.5f;
                float minRadius = 0.5f;  // 敵の体に重なるサイズ
                float ringRadius = maxRadius - urgency * (maxRadius - minRadius);

                // --- 色の遷移 ---
                DirectX::XMFLOAT4 ringColor;
                float alpha;
                float parryZone = 0.25f;

                if (timeToHit < parryZone)
                {
                    // // 緑 = 「今パリィ！」
                    float pulse = (sinf(m_gameTime * 30.0f) + 1.0f) * 0.5f;
                    alpha = 0.8f + pulse * 0.2f;
                    ringColor = { 0.0f, 1.0f, 0.3f, alpha };
                }
                else if (timeToHit < 0.5f)
                {
                    // 赤 = 「もうすぐ！」
                    float pulse = (sinf(m_gameTime * 16.0f) + 1.0f) * 0.5f;
                    alpha = 0.5f + urgency * 0.3f + pulse * 0.2f;
                    ringColor = { 1.0f, 0.1f, 0.0f, alpha };
                }
                else
                {
                    // 暗い赤 = 「攻撃が来るぞ」
                    alpha = 0.2f + urgency * 0.3f;
                    ringColor = { 0.8f, 0.2f, 0.0f, alpha };
                }

                // --- 六角形リング描画（カメラ正対ビルボード） ---
                const int SEGMENTS = 6;
                float rotation = m_gameTime * 1.5f;  // ゆっくり回転

                DirectX::XMFLOAT3 ringVerts[SEGMENTS];
                for (int s = 0; s < SEGMENTS; s++)
                {
                    float angle = rotation + (float)s / SEGMENTS * 6.2832f;
                    DirectX::XMVECTOR pt = ringCenter;
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camRight, cosf(angle) * ringRadius));
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camUp, sinf(angle) * ringRadius));
                    DirectX::XMStoreFloat3(&ringVerts[s], pt);
                }

                // 外周リング線
                for (int s = 0; s < SEGMENTS; s++)
                {
                    int next = (s + 1) % SEGMENTS;
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(ringVerts[s], ringColor),
                        DirectX::VertexPositionColor(ringVerts[next], ringColor)
                    );
                }

                // --- 内側固定リング（ターゲットサイズ = パリィタイミング表示）---
                DirectX::XMFLOAT4 innerColor = { ringColor.x * 0.4f, ringColor.y * 0.4f, ringColor.z * 0.4f, alpha * 0.3f };
                DirectX::XMFLOAT3 innerVerts[SEGMENTS];
                for (int s = 0; s < SEGMENTS; s++)
                {
                    float angle = rotation + (float)s / SEGMENTS * 6.2832f;
                    DirectX::XMVECTOR pt = ringCenter;
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camRight, cosf(angle) * minRadius));
                    pt = DirectX::XMVectorAdd(pt,
                        DirectX::XMVectorScale(camUp, sinf(angle) * minRadius));
                    DirectX::XMStoreFloat3(&innerVerts[s], pt);
                }
                for (int s = 0; s < SEGMENTS; s++)
                {
                    int next = (s + 1) % SEGMENTS;
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(innerVerts[s], innerColor),
                        DirectX::VertexPositionColor(innerVerts[next], innerColor)
                    );
                }

                // --- 収束線（外リング→内リングを繋ぐ6本のガイド線）---
                DirectX::XMFLOAT4 lineColor = { ringColor.x, ringColor.y, ringColor.z, alpha * 0.15f };
                for (int s = 0; s < SEGMENTS; s++)
                {
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(ringVerts[s], lineColor),
                        DirectX::VertexPositionColor(innerVerts[s], innerColor)
                    );
                }

                // --- パリィ圏内：内リングを明るく強調 ---
                if (timeToHit < parryZone)
                {
                    DirectX::XMFLOAT4 glowColor = { 0.0f, 1.0f, 0.3f, 0.6f };
                    for (int s = 0; s < SEGMENTS; s++)
                    {
                        int next = (s + 1) % SEGMENTS;
                        // 内リングの三角形で塗りつぶし（中心→辺）
                        DirectX::XMFLOAT3 ctr;
                        DirectX::XMStoreFloat3(&ctr, ringCenter);
                        DirectX::XMFLOAT4 centerGlow = { 0.0f, 1.0f, 0.3f, 0.15f };
                        primitiveBatch->DrawTriangle(
                            DirectX::VertexPositionColor(ctr, centerGlow),
                            DirectX::VertexPositionColor(innerVerts[s], glowColor),
                            DirectX::VertexPositionColor(innerVerts[next], glowColor)
                        );
                    }
                }
            }
        }
    }

    primitiveBatch->End();

    // ラスタライザを元に戻す
    context->RSSetState(nullptr);
}

// ============================================================
//  DrawKeyPrompt - グローリーキル「F」プロンプト描画
//
//  【前提】DrawEnemies() の後に呼ぶこと（深度テストOFF＋後描画で最前面）
// ============================================================
void Game::DrawKeyPrompt(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
    if (!m_keyPrompt || !m_gloryKillTargetEnemy) return;

    DirectX::XMFLOAT3 promptPos = m_gloryKillTargetEnemy->position;
    promptPos.y += 1.0f;
    constexpr float PROMPT_SIZE = 0.4f;

    m_keyPrompt->Render(
        m_d3dContext.Get(),
        promptPos,
        PROMPT_SIZE,
        m_gloryKillTargetEnemy->staggerTimer,
        viewMatrix,
        projectionMatrix);
}

// ============================================================
//  DrawSingleEnemy - ボス/ミッドボスの単体描画
//
//  【用途】インスタンス描画に乗らない特殊な敵の描画
//  ボスは専用シェーダーやスケールが異なるため個別処理
// ============================================================
void Game::DrawSingleEnemy(const Enemy& enemy, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
    if (!enemy.isAlive && !enemy.isDying)
        return;

    if (enemy.isExploded)
        return;

    // ワールド行列を計算
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);
    DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
        enemy.position.x,
        enemy.position.y,
        enemy.position.z
    );
    DirectX::XMMATRIX world = scale * rotation * translation;

    // 適切なモデルを選択
    Model* model = nullptr;
    switch (enemy.type)
    {
    case EnemyType::NORMAL:
        model = m_enemyModel.get();
        break;
    case EnemyType::RUNNER:
        model = m_runnerModel.get();
        break;
    case EnemyType::TANK:
        model = m_tankModel.get();
        break;
    }

    if (!model) return;

    // 深度テストを有効化
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    // アニメーション描画
    std::string animName = enemy.currentAnimation;
    if (enemy.headDestroyed)
    {
        model->SetBoneScaleByPrefix("Head", 0.0f);
    }

    model->DrawAnimated(
        m_d3dContext.Get(),
        world,
        viewMatrix,
        projectionMatrix,
        DirectX::XMLoadFloat4(&enemy.color),  // XMFLOAT4 → XMVECTOR
        animName,
        enemy.animationTime
    );

    // 頭のスケールをリセット
    if (enemy.headDestroyed)
    {
        model->SetBoneScaleByPrefix("Head", 1.0f);
    }
}

//  回転対応版：レイとOBB（回転する箱）の交差判定

// ============================================================
//  CheckRayIntersection - レイとAABBの交差判定
//
//  【用途】武器スポーンのインタラクション判定
//  プレイヤーの視線レイがオブジェクトのAABBと交差するか計算
//  【アルゴリズム】スラブ法（Slab Method）
//  各軸のtmin/tmaxを計算し、全軸で重なる区間があれば交差
// ============================================================
float Game::CheckRayIntersection(
    DirectX::XMFLOAT3 rayStart,
    DirectX::XMFLOAT3 rayDir,
    DirectX::XMFLOAT3 enemyPos,
    float enemyRotationY,
    EnemyType enemyType)
{
    using namespace DirectX;

    // === 設定を取得（デバッグモード対応）===
    EnemyTypeConfig config;

    if (m_useDebugHitboxes)
    {
        switch (enemyType)
        {
        case EnemyType::NORMAL:
            config = m_normalConfigDebug;
            break;
        case EnemyType::RUNNER:
            config = m_runnerConfigDebug;
            break;
        case EnemyType::TANK:
            config = m_tankConfigDebug;
            break;
        case EnemyType::MIDBOSS:
            config = m_midbossConfigDebug;
            break;
        case EnemyType::BOSS:
            config = m_bossConfigDebug;
            break;
        }
    }
    else
    {
        config = GetEnemyConfig(enemyType);
    }

    float width = config.bodyWidth;
    float height = config.bodyHeight;

    // === レイをローカル座標系に変換 ===
    XMVECTOR vRayOrigin = XMLoadFloat3(&rayStart);
    XMVECTOR vRayDir = XMLoadFloat3(&rayDir);
    XMVECTOR vEnemyPos = XMLoadFloat3(&enemyPos);

    // 平行移動（敵を原点に）
    XMVECTOR vLocalOrigin = XMVectorSubtract(vRayOrigin, vEnemyPos);

    // 逆回転（敵の回転の逆をレイにかける）
    XMMATRIX matInvRot = XMMatrixRotationY(-enemyRotationY);
    vLocalOrigin = XMVector3TransformCoord(vLocalOrigin, matInvRot);
    vRayDir = XMVector3TransformNormal(vRayDir, matInvRot);

    // Float3に戻す
    XMFLOAT3 localOrigin, localDir;
    XMStoreFloat3(&localOrigin, vLocalOrigin);
    XMStoreFloat3(&localDir, vRayDir);

    // === AABB（軸平行ボックス）との判定 ===
    // 箱の最小点・最大点（足元がY=0になるように）
    float minX = -width / 2.0f;
    float minY = 0.0f;
    float minZ = -width / 2.0f;  // ← Z軸も//

    float maxX = width / 2.0f;
    float maxY = height;
    float maxZ = width / 2.0f;  // ← Z軸も//

    // スラブ法（Slab Method）による判定
    float tMin = 0.0f;
    float tMax = 10000.0f;

    // === X軸チェック ===
    if (fabs(localDir.x) < 1e-6f)
    {
        if (localOrigin.x < minX || localOrigin.x > maxX)
            return -1.0f;
    }
    else
    {
        float t1 = (minX - localOrigin.x) / localDir.x;
        float t2 = (maxX - localOrigin.x) / localDir.x;
        if (t1 > t2) std::swap(t1, t2);
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // === Y軸チェック ===
    if (fabs(localDir.y) < 1e-6f)
    {
        if (localOrigin.y < minY || localOrigin.y > maxY)
            return -1.0f;
    }
    else
    {
        float t1 = (minY - localOrigin.y) / localDir.y;
        float t2 = (maxY - localOrigin.y) / localDir.y;
        if (t1 > t2) std::swap(t1, t2);
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // === Z軸チェック（これが抜けてた！）===
    if (fabs(localDir.z) < 1e-6f)
    {
        if (localOrigin.z < minZ || localOrigin.z > maxZ)
            return -1.0f;
    }
    else
    {
        float t1 = (minZ - localOrigin.z) / localDir.z;
        float t2 = (maxZ - localOrigin.z) / localDir.z;
        if (t1 > t2) std::swap(t1, t2);
        tMin = max(tMin, t1);
        tMax = min(tMax, t2);
        if (tMin > tMax) return -1.0f;
    }

    // 後ろの敵には当たらない
    if (tMin < 0.0f) return -1.0f;

    return tMin;  // 衝突距離
}




// ============================================================
//  DrawBillboard - ビルボード描画
//
//  【用途】スタンリングやターゲットマーカーなど、
//  常にカメラに正対する2Dスプライトの描画
// ============================================================
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
        DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
    );

    // 小さな黄色いキューブをダメージ表示位置に描画
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f) *  // 小さくする
        DirectX::XMMatrixTranslation(m_damageDisplayPos.x,
            m_damageDisplayPos.y,
            m_damageDisplayPos.z);

    m_cube->Draw(world, viewMatrix, projectionMatrix, DirectX::Colors::Yellow);
}


// ============================================================
//  DrawParticles - パーティクルシステムの描画
//
//  【描画対象】
//  - GPUパーティクル（血しぶき/ミスト）
//  - Effekseer（リボントレイル等）
//  - 火パーティクル
//  【ブレンド】加算ブレンド → 最後にデフォルトに戻す
// ============================================================
void Game::DrawParticles()
{
    if (m_particleSystem->IsEmpty())
        return;

    auto context = m_d3dContext.Get();

    // === カメラ行列 ===
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
        DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
    );

    // === テクスチャ付きエフェクト ===
    m_particleEffect->SetView(viewMatrix);
    m_particleEffect->SetProjection(projectionMatrix);
    m_particleEffect->SetWorld(DirectX::XMMatrixIdentity());
    m_particleEffect->Apply(context);
    context->IASetInputLayout(m_particleInputLayout.Get());

    // === アルファブレンド + 加算ブレンド ===
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_states->AlphaBlend(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    // === ビルボード方向 ===
    DirectX::XMFLOAT4X4 viewF;
    DirectX::XMStoreFloat4x4(&viewF, viewMatrix);
    DirectX::XMFLOAT3 right(viewF._11, viewF._21, viewF._31);
    DirectX::XMFLOAT3 up(viewF._12, viewF._22, viewF._32);

    // === テクスチャ付きビルボード描画 ===
    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColorTexture>>(context);
    batch->Begin();

    for (const auto& particle : m_particleSystem->GetParticles())
    {
        float s = particle.size;
        DirectX::XMFLOAT3 c = particle.position;

        // 4頂点 + UV座標
        DirectX::XMFLOAT3 tl(
            c.x - right.x * s + up.x * s,
            c.y - right.y * s + up.y * s,
            c.z - right.z * s + up.z * s);
        DirectX::XMFLOAT3 tr(
            c.x + right.x * s + up.x * s,
            c.y + right.y * s + up.y * s,
            c.z + right.z * s + up.z * s);
        DirectX::XMFLOAT3 bl(
            c.x - right.x * s - up.x * s,
            c.y - right.y * s - up.y * s,
            c.z - right.z * s - up.z * s);
        DirectX::XMFLOAT3 br(
            c.x + right.x * s - up.x * s,
            c.y + right.y * s - up.y * s,
            c.z + right.z * s - up.z * s);

        DirectX::VertexPositionColorTexture vTL(tl, particle.color, DirectX::XMFLOAT2(0, 0));
        DirectX::VertexPositionColorTexture vTR(tr, particle.color, DirectX::XMFLOAT2(1, 0));
        DirectX::VertexPositionColorTexture vBL(bl, particle.color, DirectX::XMFLOAT2(0, 1));
        DirectX::VertexPositionColorTexture vBR(br, particle.color, DirectX::XMFLOAT2(1, 1));

        batch->DrawTriangle(vTL, vTR, vBL);
        batch->DrawTriangle(vTR, vBR, vBL);
    }

    batch->End();

    // === 元に戻す ===
    context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
}


// ============================================================
//  DrawWeapon - FPS視点の武器描画
//
//  【描画方式】カメラの右前方にオフセットして描画
//  武器ボブ（歩行時の揺れ）とリコイル（射撃時の跳ね上がり）を適用
//  深度バッファをクリアしてから描画（壁にめり込まない）
// ============================================================
void Game::DrawWeapon()
{
    if (!m_weaponModel)
    {
        return;
    }

    //  パリィ中は銃を非表示にする
    if (m_shieldState == ShieldState::Parrying ||
        m_shieldState == ShieldState::Guarding ||
        m_shieldBashTimer > 0.0f)
    {
        return; //  盾が出てる間は表示しない
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
    DirectX::XMVECTOR rightOffset = XMVectorScale(right,
        m_weaponOffsetRight + m_weaponSwayX * 0.1f);
    DirectX::XMVECTOR upOffset = XMVectorScale(up,
        m_weaponOffsetUp + m_weaponSwayY * 0.1f
        + m_weaponBobAmount + m_weaponLandingImpact
        + m_weaponKickUp + m_reloadAnimOffset);
    DirectX::XMVECTOR forwardOffset = XMVectorScale(forward,
        m_weaponOffsetForward - m_weaponKickBack);

    DirectX::XMVECTOR weaponPos = cameraPosition;
    weaponPos = XMVectorAdd(weaponPos, rightOffset);
    weaponPos = XMVectorAdd(weaponPos, upOffset);
    weaponPos = XMVectorAdd(weaponPos, forwardOffset);

    // === スケール ===
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(m_weaponScale, m_weaponScale, m_weaponScale);

    // === 回転（シンプル3軸） ===
    DirectX::XMMATRIX modelRotation = DirectX::XMMatrixRotationRollPitchYaw(
        m_weaponRotX - m_weaponKickRot + m_reloadAnimTilt,
        m_weaponRotY, m_weaponRotZ
    );
    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(weaponPos);

    // 合成
    DirectX::XMMATRIX weaponWorld = scale * modelRotation * cameraRotation * translation;

    m_weaponModel->Draw(
        m_d3dContext.Get(),
        weaponWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

// =================================================================
// 盾を左手に描画（DOOM: The Dark Ages スタイル）
// =================================================================

// ============================================================
//  DrawShield - 盾の描画（ガード/投擲/チャージ状態）
//
//  【状態別描画】
//  - Guarding/Charging: プレイヤーの前方に表示
//  - Throwing: 投擲中の軌道上に表示（回転あり）
//  - Idle: FPS視点で左手に表示
//  - Broken: 非表示
// ============================================================
void Game::DrawShield()
{
    if (!m_shieldModel)
        return;

    // // 盾投げ中は別の描画（ワールド空間で飛んでる盾）
    if (m_shieldState == ShieldState::Throwing)
    {
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
            DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
        );

        // ワールド空間での盾の位置
        float shieldScale = 0.25f;
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(shieldScale, shieldScale, shieldScale);

        // 回転: 元の向き補正 + 高速スピン
        DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PI);
        DirectX::XMMATRIX standUp = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
        DirectX::XMMATRIX spin = DirectX::XMMatrixRotationY(m_thrownShieldSpin);

        // 飛ぶ方向に向ける
        float yaw = atan2f(m_thrownShieldDir.x, m_thrownShieldDir.z);
        float pitch = -asinf(m_thrownShieldDir.y);
        DirectX::XMMATRIX faceDir = DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);

        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
            m_thrownShieldPos.x, m_thrownShieldPos.y, m_thrownShieldPos.z);

        DirectX::XMMATRIX shieldWorld = scale * modelFix * standUp * spin * faceDir * translation;

        m_shieldModel->Draw(
            m_d3dContext.Get(),
            shieldWorld,
            viewMatrix,
            projectionMatrix,
            DirectX::Colors::White
        );
        return;  // 通常描画はスキップ
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
        DirectX::XMConvertToRadians(m_currentFOV), aspectRatio, 0.1f, 1000.0f
    );

    // カメラの方向ベクトル
    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        sinf(playerRot.y) * cosf(playerRot.x),
        -sinf(playerRot.x),
        cosf(playerRot.y) * cosf(playerRot.x),
        0.0f
    );
    DirectX::XMVECTOR right = DirectX::XMVectorSet(
        cosf(playerRot.y), 0.0f, -sinf(playerRot.y), 0.0f
    );
    DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

    // === 盾の位置アニメーション ===
    // パリィ中: 左下 → 正面中央にスライド
    float guardProgress = 0.0f;  // 0.0=待機位置、1.0=正面ガード位置
    if (m_shieldBashTimer > 0.0f)
    {
        float t = 1.0f - (m_shieldBashTimer / m_shieldBashDuration);
        // 最初の20%: サッと正面に構える（イージングで素早く）
        // 最後の30%: ゆっくり戻す
        // 中間50%: 正面キープ
        if (t < 0.2f)
        {
            // 0→1 に素早く（easeOut）
            float p = t / 0.2f;
            guardProgress = 1.0f - (1.0f - p) * (1.0f - p);  // easeOutQuad
        }
        else if (t < 0.7f)
        {
            // 正面キープ
            guardProgress = 1.0f;
        }
        else
        {
            // 1→0 にゆっくり戻す
            float p = (t - 0.7f) / 0.3f;
            guardProgress = 1.0f - p * p;  // easeInQuad
        }
    }

    // === 待機位置（左下）===
    float restRight = -0.35f;    // 左側
    float restUp = -0.30f;       // 下
    float restForward = 0.45f;   // 前

    // === ガード位置（正面中央）===
    float guardRight = -0.10f;   // ほぼ中央（少し左）
    float guardUp = -0.30f;      // 目線の少し下
    float guardForward = 0.40f;  // 少し前に出して小さく見せる

    // 線形補間（lerp）で待機→ガードをスムーズに遷移
    float currentRight = restRight + (guardRight - restRight) * guardProgress;
    float currentUp = restUp + (guardUp - restUp) * guardProgress;
    float currentForward = restForward + (guardForward - restForward) * guardProgress;

    DirectX::XMVECTOR rightOffset = XMVectorScale(right, currentRight + m_weaponSwayX * 0.05f * (1.0f - guardProgress));
    float bobEffect = (m_weaponBobAmount + m_weaponLandingImpact) * (1.0f - guardProgress);
    DirectX::XMVECTOR upOffset = XMVectorScale(up, currentUp + m_weaponSwayY * 0.05f * (1.0f - guardProgress) + bobEffect);
    DirectX::XMVECTOR forwardOffset = XMVectorScale(forward, currentForward);

    DirectX::XMVECTOR shieldPos = cameraPosition;
    shieldPos = XMVectorAdd(shieldPos, rightOffset);
    shieldPos = XMVectorAdd(shieldPos, upOffset);
    shieldPos = XMVectorAdd(shieldPos, forwardOffset);

    // === スケール ===
    float shieldScale = 0.25f;
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(shieldScale, shieldScale, shieldScale);

    // === 回転 ===
    DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PI);
    DirectX::XMMATRIX standUp = DirectX::XMMatrixIdentity();
    // ガード中は盾をまっすぐ、待機中は少し傾ける
    float tiltAngle = 0.1f * (1.0f - guardProgress);
    DirectX::XMMATRIX tilt = DirectX::XMMatrixRotationX(tiltAngle);

    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(shieldPos);

    // 合成（bashTilt は削除、tilt に統合済み）
    DirectX::XMMATRIX shieldWorld = scale * modelFix * standUp * tilt * cameraRotation * translation;

    m_shieldModel->Draw(
        m_d3dContext.Get(),
        shieldWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

//  描画全体

// ============================================================
//  DrawWeaponSpawns - 壁の武器スポーンの描画
//
//  【描画内容】武器スポーンシステムが管理する
//  壁掛け武器のモデルを3D空間に描画
// ============================================================
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
        // 肉片描画
        DrawGibs(viewMatrix, projectionMatrix);
    }

}


// ============================================================
//  DrawHealthPickups - ヘルスピックアップの描画
//
//  【描画内容】敵がドロップした回復アイテムを3D空間に描画
//  浮遊アニメーション（上下に揺れる）+ 回転
// ============================================================
void Game::DrawHealthPickups(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
{
    if (m_healthPickups.empty()) return;

    auto* context = m_d3dContext.Get();

    m_effect->SetView(view);
    m_effect->SetProjection(proj);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    batch->Begin();

    // カメラのRight/Upベクトル取得
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, view);
    DirectX::XMFLOAT4X4 ivF;
    DirectX::XMStoreFloat4x4(&ivF, invView);
    DirectX::XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    DirectX::XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);

    for (auto& pickup : m_healthPickups)
    {
        if (!pickup.isActive) continue;

        // ボブ演出（上下に浮遊）
        float bobY = sinf(pickup.bobTimer) * 0.2f;

        // フェード（残り5秒で点滅）
        float alpha = 1.0f;
        if (pickup.lifetime < 5.0f)
            alpha = (sinf(pickup.lifetime * 8.0f) * 0.5f + 0.5f);

        // 緑色（明るめ）
        DirectX::XMFLOAT4 color(0.1f, 1.0f, 0.2f, alpha);

        DirectX::XMFLOAT3 center = pickup.position;
        center.y += bobY + 0.5f;

        float size = 0.4f;

        // --- 十字の縦棒 ---
        DirectX::XMFLOAT3 vTop, vBot, vTopR, vBotR;
        vTop.x = center.x + camUp.x * size;
        vTop.y = center.y + camUp.y * size;
        vTop.z = center.z + camUp.z * size;
        vBot.x = center.x - camUp.x * size;
        vBot.y = center.y - camUp.y * size;
        vBot.z = center.z - camUp.z * size;
        vTopR.x = center.x + camUp.x * size + camRight.x * size * 0.3f;
        vTopR.y = center.y + camUp.y * size + camRight.y * size * 0.3f;
        vTopR.z = center.z + camUp.z * size + camRight.z * size * 0.3f;
        vBotR.x = center.x - camUp.x * size + camRight.x * size * 0.3f;
        vBotR.y = center.y - camUp.y * size + camRight.y * size * 0.3f;
        vBotR.z = center.z - camUp.z * size + camRight.z * size * 0.3f;

        DirectX::XMFLOAT3 vTopL, vBotL;
        vTopL.x = center.x + camUp.x * size - camRight.x * size * 0.3f;
        vTopL.y = center.y + camUp.y * size - camRight.y * size * 0.3f;
        vTopL.z = center.z + camUp.z * size - camRight.z * size * 0.3f;
        vBotL.x = center.x - camUp.x * size - camRight.x * size * 0.3f;
        vBotL.y = center.y - camUp.y * size - camRight.y * size * 0.3f;
        vBotL.z = center.z - camUp.z * size - camRight.z * size * 0.3f;

        batch->DrawTriangle(
            DirectX::VertexPositionColor(vTopL, color),
            DirectX::VertexPositionColor(vTopR, color),
            DirectX::VertexPositionColor(vBotR, color));
        batch->DrawTriangle(
            DirectX::VertexPositionColor(vTopL, color),
            DirectX::VertexPositionColor(vBotR, color),
            DirectX::VertexPositionColor(vBotL, color));

        // --- 十字の横棒 ---
        DirectX::XMFLOAT3 hL, hR, hLU, hRU, hLD, hRD;
        hL.x = center.x - camRight.x * size;
        hL.y = center.y - camRight.y * size;
        hL.z = center.z - camRight.z * size;
        hR.x = center.x + camRight.x * size;
        hR.y = center.y + camRight.y * size;
        hR.z = center.z + camRight.z * size;

        hLU.x = hL.x + camUp.x * size * 0.3f;
        hLU.y = hL.y + camUp.y * size * 0.3f;
        hLU.z = hL.z + camUp.z * size * 0.3f;
        hRU.x = hR.x + camUp.x * size * 0.3f;
        hRU.y = hR.y + camUp.y * size * 0.3f;
        hRU.z = hR.z + camUp.z * size * 0.3f;
        hLD.x = hL.x - camUp.x * size * 0.3f;
        hLD.y = hL.y - camUp.y * size * 0.3f;
        hLD.z = hL.z - camUp.z * size * 0.3f;
        hRD.x = hR.x - camUp.x * size * 0.3f;
        hRD.y = hR.y - camUp.y * size * 0.3f;
        hRD.z = hR.z - camUp.z * size * 0.3f;

        batch->DrawTriangle(
            DirectX::VertexPositionColor(hLU, color),
            DirectX::VertexPositionColor(hRU, color),
            DirectX::VertexPositionColor(hRD, color));
        batch->DrawTriangle(
            DirectX::VertexPositionColor(hLU, color),
            DirectX::VertexPositionColor(hRD, color),
            DirectX::VertexPositionColor(hLD, color));
    }

    batch->End();
}