// ============================================================
//  GamePlay.cpp
//  ゲームプレイロジック（更新/戦闘/近接/グローリーキル）
//
//  【責務】
//  - UpdatePlaying(): プレイ中の全ゲームロジック更新
//  - 7つのサブ関数に分割（Phase 2）
//  - 近接攻撃（PerformMeleeAttack）
//  - グローリーキルアニメーション
//  - ヘルスピックアップ/スコアポップアップ
// 
//  UpdatePlaying()を3,260行→約30行のディスパッチャに改善。
//  処理は7つのサブ関数に分割：
//    1. UpdatePlayerMovement  ? 移動/ダッシュ/グローリーキル
//    2. UpdateShooting        ? 射撃/レイキャスト/ダメージ
//    3. UpdateAmmoAndReload   ? 弾薬管理/リロード
//    4. UpdateEnemyAI         ? パスファインド/分離/死亡
//    5. UpdateWaveAndEffects  ? ウェーブ/スコア/爪痕
//    6. UpdateShieldSystem    ? 盾投擲/ガード/チャージ
//    7. UpdateBossAttacks     ? ボス攻撃/弾幕/ビーム
// ============================================================

#include "Game.h"

using namespace DirectX;

// ============================================================
//  UpdatePlaying - プレイ中の全ゲームロジック更新（ディスパッチャ）
//
//  【Phase 2で30行に圧縮】
//  以前は3,260行の巨大関数だったが、
//  7つのサブ関数を順番に呼ぶだけのシンプルな構造に改善。
//  各サブ関数の呼び出し順序には意味がある（依存関係あり）。
// ============================================================
void Game::UpdatePlaying()
{
    // --- F1キーでデバッグウィンドウ切り替え ---
    static bool f1Pressed = false;
    if (GetAsyncKeyState(VK_F1) & 0x8000)
    {
        if (!f1Pressed)
        {
            m_showDebugWindow = !m_showDebugWindow;
            if (m_showDebugWindow)
            {
                m_player->SetMouseCaptured(false);
                ShowCursor(TRUE);
            }
            else
            {
                m_player->SetMouseCaptured(true);
                ShowCursor(FALSE);
            }
            f1Pressed = true;
        }
    }
    else
    {
        f1Pressed = false;
    }

    // --- F3キーで切断テスト（デバッグ用） ---
    if (GetAsyncKeyState(VK_F3) & 1)
    {
        ExecuteSliceTest();
    }

    // --- deltaTime計算 + 基本タイマー更新 ---
    float deltaTime = m_deltaTime * m_timeScale;

    // タイプ別アニメーションタイマー更新
    for (int i = 0; i < 5; i++)
    {
        m_typeWalkTimer[i] += deltaTime * m_enemySystem->m_animSpeed_Walk[i];
        m_typeAttackTimer[i] += deltaTime * m_enemySystem->m_animSpeed_Attack[i];
    }

    m_gameTime += deltaTime;

    // スタイルランク更新
    m_styleRank->Update(deltaTime);

    // 最高コンボ/スタイルランク/生存時間の記録更新
    int currentCombo = m_styleRank->GetComboCount();
    if (currentCombo > m_statMaxCombo)
        m_statMaxCombo = currentCombo;
    int currentStyleRank = static_cast<int>(m_styleRank->GetRank());
    if (currentStyleRank > m_statMaxStyleRank)
        m_statMaxStyleRank = currentStyleRank;
    m_statSurvivalTime += deltaTime;

    // グローリーキルアニメーション更新
    UpdateGloryKillAnimation(deltaTime);

    // =============================================
    //  7つのサブ関数を順番に呼ぶ
    //  【順序の意味】
    //  1. プレイヤー移動 → 2. 射撃 → 3. 弾薬
    //     （プレイヤーの位置が確定してから射撃判定）
    //  4. 敵AI → 5. ウェーブ → 6. 盾 → 7. ボス攻撃
    //     （敵の位置が確定してからボス攻撃判定）
    // =============================================
    UpdatePlayerMovement(deltaTime);    // 1. 移動/ダッシュ/グローリーキル/押し戻し
    UpdateShooting(deltaTime);          // 2. 射撃/レイキャスト/ヒット判定
    UpdateAmmoAndReload(deltaTime);     // 3. 弾切れ警告/リロード
    UpdateEnemyAI(deltaTime);           // 4. パスファインド/分離/死亡処理
    UpdateWaveAndEffects(deltaTime);    // 5. ウェーブ/スコアHUD/爪痕
    UpdateShieldSystem(deltaTime);      // 6. 盾投擲/ガード/チャージ/パリィ
    UpdateBossAttacks(deltaTime);       // 7. ボス攻撃/スラム/ビーム/弾幕
}


// ============================================================
//  UpdatePlayerMovement - プレイヤー移動/ダッシュ/グローリーキル/押し戻し/武器ボブ
//
//  【処理内容】
//  - 物理ボディの//（新スポーン敵）
//  - 弾道トレース更新
//  - プレイヤー移動入力
//  - グローリーキルカメラ制御
//  - ダッシュ演出
//  - 敵との押し戻し判定
//  - 武器ボブ（歩行時の揺れ）
// ============================================================
void Game::UpdatePlayerMovement(float deltaTime)
{
    // === 敵に物理ボディを//（まだ//されてない敵のみ）===

    if (!m_enemiesInitialized && m_enemySystem->GetEnemies().size() > 0)
    {
        //OutputDebugStringA("Initializing enemy physics bodies...\n");
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.isAlive)
            {
                AddEnemyPhysicsBody(enemy);
            }
        }
        m_enemiesInitialized = true;
        //OutputDebugStringA("Enemy physics bodies initialized!\n");
    }

    // === 弾の軌跡を更新 ===
    for (auto it = m_bulletTraces.begin(); it != m_bulletTraces.end();)
    {
        it->lifetime -= deltaTime;
        if (it->lifetime <= 0.0f)
        {
            it = m_bulletTraces.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //  --- ヒットストップ処理   ---
    if (m_hitStopTimer > 0.0f)
    {
        m_hitStopTimer -= m_deltaTime;  // timeScaleに影響されない実時間

        if (m_hitStopTimer <= 0.0f)
        {
            m_hitStopTimer = 0.0f;

            if (m_slowMoTimer <= 0.0f)
                m_timeScale = 1.0f;
        }
    }

    // 無敵時間の更新
    if (m_gloryKillInvincibleTimer > 0.0f)
    {
        m_gloryKillInvincibleTimer -= deltaTime;
        if (m_gloryKillInvincibleTimer < 0.0f)
            m_gloryKillInvincibleTimer = 0.0f;
    }

    //  グローリーキルカメラ更新
    if (m_gloryKillCameraActive)
    {
        m_gloryKillCameraLerpTime += m_deltaTime * 3.0f;
        if (m_gloryKillCameraLerpTime > 1.0f)
            m_gloryKillCameraLerpTime = 1.0f;

        //  被写界深度の強度を徐々に上げる
        if (m_gloryKillDOFActive)
        {
            m_gloryKillDOFIntensity += m_deltaTime * 4.0f;
            if (m_gloryKillDOFIntensity > 1.0f)
                m_gloryKillDOFIntensity = 1.0f;
        }
    }
    else
    {
        // カメラが無効化されたらDOFも無効化
        if (m_gloryKillDOFActive)
        {
            m_gloryKillDOFActive = false;
            m_gloryKillDOFIntensity = 0.0f;

            /*char debugDOF[128];
            sprintf_s(debugDOF, "[DOF] Deactivated! Camera ended.\n");*/
            //OutputDebugStringA(debugDOF);
        }
    }

    // グローリーキル腕アニメーション更新（横からこめかみへ）
    if (m_gloryKillArmAnimActive)
    {
        // アニメーション時間を進める（0.0～1.0）
        m_gloryKillArmAnimTime += m_deltaTime * 1.5f;  // 0.4秒で完了

        if (m_gloryKillArmAnimTime >= 1.0f)
        {
            m_gloryKillArmAnimTime = 1.0f;
            m_gloryKillArmAnimActive = false;
        }

        // イージング関数（滑らかな動き）
        float t = m_gloryKillArmAnimTime;
        float easeInOut = t < 0.5f
            ? 2.0f * t * t
            : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        // 腕の位置を計算（横から刺す）
        // 0.0: 右側（画面外） → 0.4: 中央（敵のこめかみ） → 1.0: 元の位置
         // 腕の位置を計算（横から刺す）
        if (t < 0.5f)
        {
            // 右から中央へ（刺す）0.0～0.5
            float t1 = t / 0.5f;  // 0.0～1.0に正規化
            m_gloryKillArmPos.x = 1.5f - easeInOut * 2.0f;  // 右から左へ
            m_gloryKillArmPos.y = 0.0f;  // 中央の高さ
            m_gloryKillArmPos.z = 0.5f;  // カメラの前
        }
        else
        {
            // 中央から右へ（戻る）0.5～1.0
            float t2 = (t - 0.5f) / 0.5f;  // 0.0～1.0に正規化
            m_gloryKillArmPos.x = -0.5f + t2 * 2.0f;  // 左から右へ
            m_gloryKillArmPos.y = 0.0f;
            m_gloryKillArmPos.z = 0.5f;
        }

        // 腕の回転（横向き）
        m_gloryKillArmRot.x = 0.0f;  // X軸回転なし
        m_gloryKillArmRot.y = DirectX::XM_PIDIV2;  // 90度（横向き）
        m_gloryKillArmRot.z = 0.0f;  // Z軸回転なし
    }

    // 画面フラッシュタイマーの更新
    if (m_gloryKillFlashTimer > 0.0f)
    {
        m_gloryKillFlashTimer -= m_deltaTime;
        if (m_gloryKillFlashTimer < 0.0f)
        {
            m_gloryKillFlashTimer = 0.0f;
            m_gloryKillFlashAlpha = 0.0f;
        }
        else
        {
            // 正規化された時間（0.0～1.0）
            float normalizedTime = m_gloryKillFlashTimer / 0.3f;  // 0.15f → 0.3f（長めに）

            // イージング関数：quadratic ease-out（二次関数）
            // 最初は速く減少、最後はゆっくり減少
            float easeOut = 1.0f - (1.0f - normalizedTime) * (1.0f - normalizedTime);

            m_gloryKillFlashAlpha = easeOut;
        }
    }

    // グローリーキル可能な敵を探す（常に）
    m_gloryKillTargetEnemy = nullptr;
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    float closestDist = m_gloryKillRange;

    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying)
            continue;

        if (!enemy.isStaggered)
            continue;

        // 距離を計算
        float dx = enemy.position.x - playerPos.x;
        float dz = enemy.position.z - playerPos.z;
        float dist = sqrtf(dx * dx + dz * dz);

        if (dist < closestDist)
        {
            closestDist = dist;
            m_gloryKillTargetEnemy = &enemy;
        }
    }

    // Fキーが押されたか検出
    static bool fKeyPressed = false;
    bool fKeyDown = (GetAsyncKeyState('F') & 0x8000) != 0;

    if (fKeyDown && !fKeyPressed)
    {
        // Fキーが押された瞬間
        if (m_gloryKillTargetEnemy != nullptr)
        {
            // === グローリーキル実行！ ===

            m_gloryKillActive = true;
            // デバッグログ
            /*char debugGK[256];
            sprintf_s(debugGK, "[GLORY KILL] Executed! Target ID=%d, DOF will activate next frame\n", m_gloryKillTargetID);
            //OutputDebugStringA(debugGK);*/

            // === カメラをターゲット敵に向ける ===
            m_gloryKillCameraActive = false;
            m_gloryKillCameraLerpTime = 0.0f;

            // カメラ位置：プレイヤーとターゲット敵の間、やや上から
            DirectX::XMFLOAT3 playerPosition = m_player->GetPosition();
            DirectX::XMFLOAT3 enemyPos = m_gloryKillTargetEnemy->position;

            // === 敵をプレイヤーの目の前に移動 ===
            DirectX::XMFLOAT3 playerRot = m_player->GetRotation();
            float forwardX = sinf(playerRot.y);
            float forwardZ = cosf(playerRot.y);

            // プレイヤーの前方 0.8m の位置に敵を配置
            float targetDistance = 0.8f;  // この値を小さくするとより近くなる
            m_gloryKillTargetEnemy->position.x = playerPosition.x + forwardX * targetDistance;
            m_gloryKillTargetEnemy->position.z = playerPosition.z + forwardZ * targetDistance;
            m_gloryKillTargetEnemy->position.y = playerPosition.y - 1.7f;

            // プレイヤーから敵への方向ベクトル
            float dx = enemyPos.x - playerPosition.x;
            float dz = enemyPos.z - playerPosition.z;
            float distance = sqrtf(dx * dx + dz * dz);

            // 正規化
            if (distance > 0.0f)
            {
                dx /= distance;
                dz /= distance;
            }

            // カメラ位置：プレイヤーの少し後ろ、少し上
            m_gloryKillCameraPos.x = playerPosition.x - dx;
            m_gloryKillCameraPos.y = playerPosition.y;  // 少し上
            m_gloryKillCameraPos.z = playerPosition.z - dz;

            // カメラターゲット：敵の中心（胸の高さ）
            m_gloryKillCameraTarget = enemyPos;
            m_gloryKillCameraTarget.y += 1.0f;

            // 被写界深度を有効化
            m_gloryKillDOFActive = true;
            m_gloryKillDOFIntensity = 0.0f;  // 徐々に強くする
            // デバッグログ//
            /*char debugDOF[256];
            sprintf_s(debugDOF, "[GLORY KILL] DOF Activated! Active=%d, Intensity=%.2f, FocalDepth=%.2f\n",
                m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_gloryKillFocalDepth);
            //OutputDebugStringA(debugDOF);*/


            //  終点距離を計算(プレイヤーから敵までの距離)
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            DirectX::XMVECTOR playerVec = DirectX::XMLoadFloat3(&playerPos);
            DirectX::XMVECTOR enemyVec = DirectX::XMLoadFloat3(&m_gloryKillTargetEnemy->position);
            DirectX::XMVECTOR distVec = DirectX::XMVectorSubtract(enemyVec, playerVec);
            float enemyDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distVec));
            m_gloryKillFocalDepth = enemyDistance;

            // デバッグログ（焦点距離も表示）
            /*char debugFocal[256];
            sprintf_s(debugFocal, "[DOF] Focal depth set to: %.2f meters\n", m_gloryKillFocalDepth);
            //OutputDebugStringA(debugFocal);*/

            /*char cdebugDOF[256];
            sprintf_s(cdebugDOF, "[GLORY KILL] DOF Activated! Active=%d, Intensity=%.2f, Offscreen=%p\n",
                m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_offscreenRTV.Get());
            //OutputDebugStringA(cdebugDOF);*/

            // 敵を即座に倒す
            m_gloryKillTargetEnemy->isDying = true;
            m_gloryKillTargetEnemy->animationTime = 0.0f;
            m_gloryKillTargetEnemy->currentAnimation = "Death";
            m_gloryKillTargetEnemy->corpseTimer = 3.0f;

            //  //: MIDBOSSのビームエフェクトを停止
            if ((m_gloryKillTargetEnemy->type == EnemyType::MIDBOSS ||
                m_gloryKillTargetEnemy->type == EnemyType::BOSS) &&
                m_beamHandle >= 0 && m_effekseerManager != nullptr)
            {
                m_effekseerManager->StopEffect(m_beamHandle);
                m_beamHandle = -1;
            }

            // スクリーンブラッド
            m_bloodSystem->OnGloryKill(m_gloryKillTargetEnemy->position);
            m_gpuParticles->EmitSplash(m_gloryKillTargetEnemy->position, XMFLOAT3(0.0f, 1.0f, 0.0f), 50, 10.0f);
            m_gpuParticles->EmitMist(m_gloryKillTargetEnemy->position, 150, 3.0f);
            m_gpuParticles->Emit(m_gloryKillTargetEnemy->position, 100, 8.0f);

            // HP回復（最大HPを超えないように）
            int currentHP = m_player->GetHealth();
            int maxHP = m_player->GetMaxHealth();
            int healAmount = 30;
            int newHP = min(currentHP + healAmount, maxHP);
            m_player->SetHealth(newHP);

            //  wavemanagerに加算
            int waveBonus = m_waveManager->OnEnemyKilled();
            if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);

            //  ポイント加算
            int totalPoints = 200 + waveBonus;  //  グローリーキル = 200points
            m_player->AddPoints(totalPoints);
            SpawnScorePopup(totalPoints, ScorePopupType::GLORY_KILL);

            // スコア換算
            m_score += 100;

            // 弾補充（マガジン満タン + 予備弾//）
            /*m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetMaxAmmo());
            m_weaponSystem->SetReserveAmmo(m_weaponSystem->GetReserveAmmo() + 30);*/

            // 4. デバッグログ
            /*char buffer[256];
            sprintf_s(buffer, "[GLORY KILL] Enemy ID:%d killed! HP: %d→%d, Score: +100\n",
                m_gloryKillTargetEnemy->id, currentHP, newHP);
            //OutputDebugStringA(buffer);*/


            // カメラシェイク（強め）
            m_cameraShake = 1.5f;      // 0.3f → 0.5f（強く）
            m_cameraShakeTimer = 0.5f;  // 0.2f → 0.3f（長く）

            // 無敵時間（1秒）
            m_gloryKillInvincibleTimer = 1.0f;

            // スローモーション（0.5秒間、0.3倍速）
            /*m_timeScale = 0.1f;*/

            // 血しぶきパーティクル（大量）
            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
            //m_particleSystem->CreateBloodEffect(m_gloryKillTargetEnemy->position, upDir, 800);

            // グローリーキル腕アニメーション開始（横からこめかみへ）
            m_gloryKillArmAnimActive = true;
            m_gloryKillArmAnimTime = 0.0f;
            m_gloryKillArmPos = DirectX::XMFLOAT3(2.0f, 0.3f, 0.3f);  // 初期位置（右側）
            m_gloryKillArmRot = DirectX::XMFLOAT3(0.0f, DirectX::XM_PIDIV2, -0.2f);  // 初期回転（横向き）

            // カメラタイマー（0.4秒）
            m_slowMoTimer = 0.4f;
        }
    }

    fKeyPressed = fKeyDown;


    //  --- スローモーション更新  ---
    if (m_slowMoTimer > 0.0f)
    {
        m_slowMoTimer -= m_deltaTime;
        if (m_slowMoTimer <= 0.0f)
        {
            m_timeScale = 1.0f;
            m_gloryKillCameraActive = false;
        }
    }

    //  --- カメラシェイク減衰   ---
    if (m_cameraShakeTimer > 0.0f)
    {
        m_cameraShakeTimer -= m_deltaTime;
        m_cameraShake *= 0.9f;
    }

    // === ダッシュ演出の更新 ===
    {
        // WASDのどれかが押されてるかチェック（移動中のみダッシュ）
        bool isMoving = (GetAsyncKeyState('W') & 0x8000) ||
            (GetAsyncKeyState('S') & 0x8000) ||
            (GetAsyncKeyState('A') & 0x8000) ||
            (GetAsyncKeyState('D') & 0x8000);

        bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        m_isSprinting = shiftHeld && isMoving;

        if (m_isSprinting)
        {
            // FOVを広げる（既存のシステムを利用）
            m_targetFOV = 85.0f;

            // スピードラインを少し出す（既存のシステム）
            m_speedLineAlpha = 0.5f;

            // テクスチャオーバーレイをフェードイン
            m_dashOverlayAlpha += deltaTime * 4.0f;
            if (m_dashOverlayAlpha > 0.35f) m_dashOverlayAlpha = 0.35f;
        }
        else
        {
            // ダッシュしてない時はFOVを戻す
            // ※ 他のシステム（盾チャージ等）でFOVを変えてなければ
            if (m_targetFOV == 85.0f)
                m_targetFOV = 70.0f;

            // オーバーレイをフェードアウト
            m_dashOverlayAlpha -= deltaTime * 6.0f;
            if (m_dashOverlayAlpha < 0.0f) m_dashOverlayAlpha = 0.0f;
        }
    }

    if (m_currentFOV < m_targetFOV)
        m_currentFOV = min(m_targetFOV, m_currentFOV + deltaTime * 360.0f);
    else if (m_currentFOV > m_targetFOV)
        m_currentFOV = max(m_targetFOV, m_currentFOV - deltaTime * 240.0f);

    if (m_shieldState != ShieldState::Charging && !m_isSprinting)
        m_speedLineAlpha = max(0.0f, m_speedLineAlpha - deltaTime * 5.0f);

    float deltatime = (1.0f / 60.0f) * m_timeScale;


    m_frameCount++;

    m_fpsTimer += deltatime;

    if (m_fpsTimer >= 1.0f)
    {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;
        //  Output ウィンドウに表示
        /*char fpsDebug[128];
        sprintf_s(fpsDebug, "=== FPS: %.1f ===\n", m_currentFPS);
        //OutputDebugStringA(fpsDebug);*/
        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }

    if (m_fpsTimer >= 1.0f)
    {
        m_currentFPS = (float)m_frameCount / m_fpsTimer;

        // ウィンドウタイトル更新（1秒に1回だけ）
        char debug[256];
        sprintf_s(debug, "FPS:%.0f | Wave:%d | Points:%d | HP:%d",
            m_currentFPS, m_waveManager->GetCurrentWave(),
            m_player->GetPoints(), m_player->GetHealth());
        SetWindowTextA(m_window, debug);

        m_frameCount = 0;
        m_fpsTimer = 0.0f;
    }


    if (!m_gloryKillCameraActive && m_shieldState != ShieldState::Charging)   //  === プレイヤー移動（チャージ中はスキップ）
    {
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

        //  --- メッシュコライダーとの壁判定  X ---
        if (CheckMeshCollision(testPositionX, playerRadius))
        {
            newPosition.x = oldPosition.x;
        }

        //  Z
        DirectX::XMFLOAT3 testPositionZ = oldPosition;
        testPositionZ.z = newPosition.z;

        if (CheckMeshCollision(testPositionZ, playerRadius))
        {
            newPosition.z = oldPosition.z;
        }

        //  床の高さに合わせてY座標を更新
        {
            float floorY = GetMeshFloorHeight(newPosition.x, newPosition.z, newPosition.y - 1.8f);
            newPosition.y = floorY + 1.8f;  // 1.8 = 目の高さ
        }

        // === 敵との押し戻し ===
        {
            float moveDx = newPosition.x - oldPosition.x;
            float moveDz = newPosition.z - oldPosition.z;
            bool isMoving = (moveDx * moveDx + moveDz * moveDz) > 0.0001f;

            if (isMoving)
            {
                const float pushRadius = 0.8f;  // プレイヤー+敵の接触距離
                const float pushStrength = 0.15f; // 押し戻しの強さ（大きいほど硬い壁に近い）

                for (const auto& enemy : m_enemySystem->GetEnemies())
                {
                    if (!enemy.isAlive || enemy.isDying) continue;

                    float ex = enemy.position.x - newPosition.x;
                    float ez = enemy.position.z - newPosition.z;
                    float dist = sqrtf(ex * ex + ez * ez);

                    // 敵の体の大きさを考慮
                    float enemyRadius = GetEnemyConfig(enemy.type).bodyWidth * 0.5f;
                    float minDist = playerRadius + enemyRadius;

                    if (dist < minDist && dist > 0.01f)
                    {
                        // 押し戻し方向（敵→プレイヤー）
                        float nx = -ex / dist;
                        float nz = -ez / dist;

                        // めり込み量
                        float overlap = minDist - dist;

                        // プレイヤーを押し戻す（100%ブロックではなく、一部すり抜け可能）
                        newPosition.x += nx * overlap * pushStrength;
                        newPosition.z += nz * overlap * pushStrength;
                    }
                }
            }
        }

        m_player->SetPosition(newPosition);

        // === 武器ボブ（カメラは動かさない） ===
        {
            float dx = newPosition.x - oldPosition.x;
            float dz = newPosition.z - oldPosition.z;
            float moveDist = sqrtf(dx * dx + dz * dz);

            if (moveDist > 0.001f)
            {
                m_isPlayerMoving = true;
                m_weaponBobTimer += deltaTime * m_weaponBobSpeed;

                float currentSin = sinf(m_weaponBobTimer);
                m_weaponBobAmount = currentSin * m_weaponBobStrength;

                // 着地検知：sin正→負 = 足が着いた瞬間
                if (m_prevWeaponBobSin > 0.0f && currentSin <= 0.0f)
                {
                    m_weaponLandingImpact = -0.015f;  // 武器がガクッと沈む
                }
                m_prevWeaponBobSin = currentSin;
            }
            else
            {
                m_isPlayerMoving = false;
                m_weaponBobAmount *= 0.85f;
            }

            m_weaponLandingImpact *= 0.82f;
        }
    }



    //  === 近接攻撃の入力処理   ===
    static bool lastFKeyState = false;  //  前フレームのFキー状態
    bool isMeleeKeyDown = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;

    //  Fキーが押された瞬間(前フレームは話されていて、今フレームは押されている)
    if (isMeleeKeyDown && !lastFKeyState)
    {
        //  クールダウン中でなければ近接攻撃を実行
        if (m_player->CanMeleeAttack() && m_meleeCharges > 0)
        {
            PerformMeleeAttack();
            m_player->StartMeleeAttackCooldown();
            m_meleeCharges--;  // チャージ消費

            //OutputDebugStringA("[MELEE] Melee attack executed!\n");
        }
        else
        {
            // クールダウン中のログ
            /*char buffer[64];
            sprintf_s(buffer, "[MELEE] Cooldown: %.2fs\n",
                m_player->GetMeleeAttackCooldown());*/
                //OutputDebugStringA(buffer);
        }
    }

    //  現在の状態を保存(次フレーム用)
    lastFKeyState = isMeleeKeyDown;


    //  === 壁武器購入システム
    m_nearbyWeaponSpawn = m_weaponSpawnSystem->CheckPlayerNearWeapon(
        m_player->GetPosition()
    );

    //  --- Eキーで盾投げ / 呼び戻し ---
    static bool eKeyPressed = false;
    if (GetAsyncKeyState('E') & 0x8000)
    {
        if (!eKeyPressed)
        {
            if (m_shieldState == ShieldState::Throwing)
            {
                // 投げてる最中にもう一度E → 呼び戻し
                m_thrownShieldReturning = true;
                //OutputDebugStringA("[SHIELD] Recall shield!\n");
            }
            else if (m_shieldState != ShieldState::Broken
                && m_shieldState != ShieldState::Charging)
            {
                // 盾投げ発動！
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

                m_thrownShieldDir = {
                    sinf(playerRot.y) * cosf(playerRot.x),
                    -sinf(playerRot.x),
                    cosf(playerRot.y) * cosf(playerRot.x)
                };

                float yaw = playerRot.y;
                float rightX = cosf(yaw);   // カメラの右方向X
                float rightZ = -sinf(yaw);  // カメラの右方向Z

                m_thrownShieldPos = {
                   playerPos.x + m_thrownShieldDir.x * 0.5f + rightX * 0.3f,
                   playerPos.y + m_thrownShieldDir.y * 0.5f - 0.3f,
                   playerPos.z + m_thrownShieldDir.z * 0.5f + rightZ * 0.3f
                };

                m_thrownShieldDist = 0.0f;
                m_thrownShieldReturning = false;
                m_thrownShieldSpin = 0.0f;
                m_thrownShieldHitEnemies.clear();
                m_thrownShieldReturnHitEnemies.clear();
                m_shieldState = ShieldState::Throwing;


                //  Effekseerトレイル再生（1回だけ再生してハンドルを保持）
                if (m_effectShieldTrail != nullptr)
                {
                    m_shieldTrailHandle = m_effekseerManager->Play(
                        m_effectShieldTrail,
                        m_thrownShieldPos.x,
                        m_thrownShieldPos.y,
                        m_thrownShieldPos.z
                    );
                }

                //OutputDebugStringA("[SHIELD] Shield thrown!\n");
            }
            eKeyPressed = true;
        }
    }
    else
    {
        eKeyPressed = false;
    }



}

// ============================================================
//  UpdateShooting - 射撃/レイキャスト/ヒット判定/ダメージ処理
//
//  【処理内容】
//  - キー入力管理（1回押し判定）
//  - 発砲リコイル減衰
//  - ボスHPバー演出更新
//  - 射撃処理（レイキャスト → ヒット判定）
//  - ヘッドショット/ボディショット分岐
//  - ダメージ適用 → 死亡判定 → キル処理
//  - 弾道トレーサー生成
// ============================================================
void Game::UpdateShooting(float deltaTime)
{
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

    // === 発砲リコイル減衰（バネ式で滑らかに戻る） ===
    m_weaponKickBack *= 0.85f;   // 後退 → 元に戻る
    m_weaponKickUp *= 0.85f;   // 跳ね上がり → 元に戻る
    m_weaponKickRot *= 0.85f;   // 回転 → 元に戻る

    // === ボスHPバー演出更新 ===
    {
        float targetHp = 0.0f;
        bool bossAlive = false;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS)
                && enemy.isAlive && !enemy.isDying)
            {
                targetHp = (float)enemy.health / (float)enemy.maxHealth;
                bossAlive = true;
                break;
            }
        }

        if (bossAlive)
        {
            if (targetHp < 0.0f) targetHp = 0.0f;
            float displaySpeed = 3.0f;
            m_bossHpDisplay += (targetHp - m_bossHpDisplay) * displaySpeed * m_deltaTime;
            float trailSpeed = 1.0f;
            m_bossHpTrail += (targetHp - m_bossHpTrail) * trailSpeed * m_deltaTime;
            if (m_bossHpDisplay < targetHp) m_bossHpDisplay = targetHp;
            if (m_bossHpTrail < m_bossHpDisplay) m_bossHpTrail = m_bossHpDisplay;
        }
        else
        {
            m_bossHpDisplay = 1.0f;
            m_bossHpTrail = 1.0f;
        }
    }

    //// === カメラ反動（上方向にキックしてゆっくり戻る） ===
    //if (fabsf(m_cameraRecoilVelocity) > 0.0001f || fabsf(m_cameraRecoilX) > 0.0001f)
    //{
    //    // カメラを上に動かす
    //    m_cameraRecoilX += m_cameraRecoilVelocity;

    //    // 速度を減衰（反動の勢いが収まる）
    //    m_cameraRecoilVelocity *= 0.8f;

    //    // 元の位置に戻すバネ力
    //    m_cameraRecoilX *= 0.88f;

    //    // 実際にカメラの角度を変更
    //    DirectX::XMFLOAT3 rot = m_player->GetRotation();
    //    rot.x -= m_cameraRecoilX * 0.3f;  // X回転 = 上下（マイナスで上を向く）
    //    m_player->SetRotation(rot);
    //}

    //  前フレームの回転を保持
    m_lastCameraRotX = m_player->GetRotation().x;
    m_lastCameraRotY = m_player->GetRotation().y;


    //  連射タイマー更新
    if (m_weaponSystem->GetFireRateTimer() > 0.0f) {
        m_weaponSystem->SetFireRateTimer(m_weaponSystem->GetFireRateTimer() - 1.0f / 60.0f);
    }

    //  === 射撃処理    ===
    if (!m_gloryKillCameraActive && m_player->IsMouseCaptured())
    {
        bool currentMouseState = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (currentMouseState && !m_lastMouseState &&
            !m_weaponSystem->IsReloading() &&
            m_weaponSystem->GetCurrentAmmo() > 0 &&
            m_weaponSystem->GetFireRateTimer() <= 0.0f &&
            m_shieldState != ShieldState::Guarding &&
            m_shieldState != ShieldState::Charging)
        {
            auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());

            // === デバッグ: 取得した武器データを確認 ===
            /*char debugWeapon[512];
            sprintf_s(debugWeapon, "[DEBUG WEAPON] GetWeaponData returned: damage=%d, maxAmmo=%d, fireRate=%.2f, cost=%d\n",
                weapon.damage, weapon.maxAmmo, weapon.fireRate, weapon.cost);
            //OutputDebugStringA(debugWeapon);*/

            //  弾を消費
            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() - 1);
            m_weaponSystem->SetFireRateTimer(weapon.fireRate);
            m_weaponSystem->SaveAmmoStatus();   //  弾薬状態を保存

            //  --- カメラリコイル（武器ごとに変える） ---
            WeaponType currentWeapon = m_weaponSystem->GetCurrentWeapon();
            switch (currentWeapon)
            {
            case WeaponType::PISTOL:
                m_cameraShake = 0.03f;
                m_cameraShakeTimer = 0.1f;
                m_weaponKickBack = 0.15f;
                m_weaponKickUp = 0.06f;
                m_weaponKickRot = 0.08f;
                break;

            case WeaponType::SHOTGUN:
                m_cameraShake = 0.15f;
                m_cameraShakeTimer = 0.2f;
                m_weaponKickBack = 0.4f;
                m_weaponKickUp = 0.15f;
                m_weaponKickRot = 0.2f;
                break;

            case WeaponType::RIFLE:
                m_cameraShake = 0.05f;
                m_cameraShakeTimer = 0.12f;
                m_weaponKickBack = 0.1f;
                m_weaponKickUp = 0.04f;
                m_weaponKickRot = 0.06f;
                break;

            case WeaponType::SNIPER:
                m_cameraShake = 0.2f;
                m_cameraShakeTimer = 0.25f;
                m_weaponKickBack = 0.5f;
                m_weaponKickUp = 0.2f;
                m_weaponKickRot = 0.25f;
                break;
            }


            //  銃口位置を「カメラ基準」で計算する（DrawWeaponと同じ方式）
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

            // forward（DrawWeaponと同じ）
            DirectX::XMVECTOR forward = DirectX::XMVectorSet(
                sinf(playerRot.y) * cosf(playerRot.x),
                -sinf(playerRot.x),
                cosf(playerRot.y) * cosf(playerRot.x), 0.0f);

            // right（DrawWeaponと同じ）
            DirectX::XMVECTOR right = DirectX::XMVectorSet(
                cosf(playerRot.y), 0.0f, -sinf(playerRot.y), 0.0f);

            // up = forward × right（//ワールド固定じゃなくカメラの上！）
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // 銃口 = プレイヤー位置 + 前0.8 + 右0.45 + 下0.15
            DirectX::XMVECTOR muzzleVec = DirectX::XMLoadFloat3(&playerPos);
            muzzleVec = DirectX::XMVectorAdd(muzzleVec, DirectX::XMVectorScale(forward, 0.8f));
            muzzleVec = DirectX::XMVectorAdd(muzzleVec, DirectX::XMVectorScale(right, 0.45f));
            muzzleVec = DirectX::XMVectorAdd(muzzleVec, DirectX::XMVectorScale(up, -0.15f));

            DirectX::XMFLOAT3 muzzlePosition;
            DirectX::XMStoreFloat3(&muzzlePosition, muzzleVec);

            //m_particleSystem->CreateMuzzleFlash(muzzlePosition, m_player->GetRotation(), currentWeapon);

            if (m_effectGunFire != nullptr && m_effekseerManager != nullptr)
            {
                auto handle = m_effekseerManager->Play(
                    m_effectGunFire,
                    muzzlePosition.x, muzzlePosition.y, muzzlePosition.z);

                // プレイヤーの視点方向に回転させる
                DirectX::XMFLOAT3 rot = m_player->GetRotation();
                Effekseer::Matrix43 mat;
                mat.Indentity();

                // Y軸回転（左右の向き）とX軸回転（上下の向き）
                float cosY = cosf(rot.y), sinY = sinf(rot.y);
                float cosX = cosf(rot.x), sinX = sinf(rot.x);

                mat.Value[0][0] = cosY;
                mat.Value[0][1] = 0;
                mat.Value[0][2] = -sinY;

                mat.Value[1][0] = sinX * sinY;
                mat.Value[1][1] = cosX;
                mat.Value[1][2] = sinX * cosY;

                mat.Value[2][0] = cosX * sinY;
                mat.Value[2][1] = -sinX;
                mat.Value[2][2] = cosX * cosY;

                mat.Value[3][0] = muzzlePosition.x;
                mat.Value[3][1] = muzzlePosition.y;
                mat.Value[3][2] = muzzlePosition.z;

                m_effekseerManager->SetMatrix(handle, mat);
                m_effekseerManager->SetScale(handle, 0.25f, 0.25f, 0.25f);
            }

            //  ショットガンは複数の弾を発射
            int pellets = (m_weaponSystem->GetCurrentWeapon() == WeaponType::SHOTGUN) ? 8 : 1;
            for (int p = 0; p < pellets; p++)
            {
                // 射撃方向を計算
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

                //  カメラ(視点)空の位置から玉を発射
                DirectX::XMFLOAT3 rayStart = playerPos;

                //  射撃方向
                DirectX::XMFLOAT3 rayDir(
                    sinf(playerRot.y) * cosf(playerRot.x),
                    -sinf(playerRot.x),
                    cosf(playerRot.y) * cosf(playerRot.x)
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

                // === Bullet Physics でレイキャスト ===
                auto rayResult = RaycastPhysics(rayStart, shotDir, 100.0f);

                if (rayResult.hit)
                {
                    // === ヒット成功！ ===

                    // 弾道を記録
                    BulletTrace trace;
                    trace.start = rayStart;
                    trace.end = rayResult.hitPoint;
                    trace.lifetime = 0.3f;
                    trace.maxLifetime = 0.3f;
                    trace.width = 0.15f;

                    switch (currentWeapon)
                    {
                    case WeaponType::PISTOL:
                        trace.color = { 1.0f, 0.8f, 0.2f, 1.0f };
                        break;
                    case WeaponType::SHOTGUN:
                        trace.color = { 1.0f, 0.5f, 0.1f, 1.0f };
                        break;
                    case WeaponType::RIFLE:
                        trace.color = { 0.3f, 0.8f, 1.0f, 1.0f };
                        break;
                    case WeaponType::SNIPER:
                        trace.color = { 1.0f, 0.2f, 0.2f, 1.0f };
                        trace.lifetime = 0.5f;
                        trace.maxLifetime = 0.5f;
                        trace.width = 0.25f;
                        break;
                    default:
                        trace.color = { 1.0f, 0.6f, 0.0f, 1.0f };
                        break;
                    }
                    m_bulletTraces.push_back(trace);

                    // UserPointer から敵を取得
                    Enemy* hitEnemy = rayResult.hitEnemy;

                    if (hitEnemy != nullptr)
                    {
                        // === 敵にヒット！ ===

                        // ヒットエフェクト（弾が当たった位置に再生）
                        if (m_effectAttackImpact != nullptr && m_effekseerManager != nullptr)
                        {
                            m_effekseerManager->Play(
                                m_effectAttackImpact,
                                rayResult.hitPoint.x,
                                rayResult.hitPoint.y,
                                rayResult.hitPoint.z);
                        }

                        // 設定取得（デバッグモード対応）
                        EnemyTypeConfig config;
                        if (m_useDebugHitboxes)
                        {
                            switch (hitEnemy->type)
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
                            config = GetEnemyConfig(hitEnemy->type);
                        }

                        // 頭の位置計算
                        hitEnemy->headPosition.x = hitEnemy->position.x;
                        hitEnemy->headPosition.y = hitEnemy->position.y + config.headHeight;
                        hitEnemy->headPosition.z = hitEnemy->position.z;

                        // ヘッドショット判定
                        float dx = rayResult.hitPoint.x - hitEnemy->headPosition.x;
                        float dy = rayResult.hitPoint.y - hitEnemy->headPosition.y;
                        float dz = rayResult.hitPoint.z - hitEnemy->headPosition.z;
                        float distanceToHead = sqrtf(dx * dx + dy * dy + dz * dz);

                        bool isHeadShot = (distanceToHead < config.headRadius);

                        if (isHeadShot && !hitEnemy->headDestroyed)
                        {
                            // === ヘッドショット処理 ===
                            ////OutputDebugStringA("[BULLET] HEADSHOT!\n");

                            if (hitEnemy->type != EnemyType::BOSS &&
                                hitEnemy->type != EnemyType::MIDBOSS &&
                                hitEnemy->type != EnemyType::TANK)
                            {
                                hitEnemy->headDestroyed = true;
                            }
                            hitEnemy->bloodDirection = shotDir;

                            // 血のエフェクト（後方）
                            //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, shotDir, 300);

                            // 血のエフェクト（上方）
                            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                            //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, upDir, 300);

                            // 放射状の血
                            for (int i = 0; i < 12; i++)
                            {
                                float angle = (i / 12.0f) * 6.28318f;
                                DirectX::XMFLOAT3 radialDir;
                                radialDir.x = cosf(angle);
                                radialDir.y = 0.3f + (rand() % 100) / 200.0f;
                                radialDir.z = sinf(angle);
                                //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, radialDir, 50);
                            }

                            // ランダム方向の血
                            for (int i = 0; i < 4; i++)
                            {
                                DirectX::XMFLOAT3 randomDir;
                                randomDir.x = ((rand() % 200) - 100) / 100.0f;
                                randomDir.y = ((rand() % 100) + 50) / 100.0f;
                                randomDir.z = ((rand() % 200) - 100) / 100.0f;
                                //m_particleSystem->CreateBloodEffect(hitEnemy->headPosition, randomDir, 50);
                            }

                            //m_particleSystem->CreateExplosion(hitEnemy->headPosition);

                            // ダメージ
                            if (hitEnemy->type == EnemyType::BOSS || hitEnemy->type == EnemyType::MIDBOSS || hitEnemy->type == EnemyType::TANK)
                            {
                                hitEnemy->health -= weapon.damage * 3;
                                m_statDamageDealt += weapon.damage * 3;  //  与ダメ記録
                            }
                            else
                            {
                                hitEnemy->health = 0;
                            }

                            if (hitEnemy->type != EnemyType::BOSS && hitEnemy->type != EnemyType::MIDBOSS && hitEnemy->type != EnemyType::TANK)
                            {
                                // 雑魚だけ吹っ飛ばす
                                hitEnemy->isRagdoll = true;
                                hitEnemy->velocity.x = shotDir.x * 15.0f;
                                hitEnemy->velocity.y = 8.0f;
                                hitEnemy->velocity.z = shotDir.z * 15.0f;

                                SpawnGibs(hitEnemy->position, 8, 15.0f);
                            }

                            // Effekseer エフェクト
                            /*if (m_effekseerManager != nullptr && m_effectBlood != nullptr)
                            {
                                auto handle = m_effekseerManager->Play(
                                    m_effectBlood,
                                    hitEnemy->headPosition.x,
                                    hitEnemy->headPosition.y,
                                    hitEnemy->headPosition.z
                                );
                                m_effekseerManager->SetScale(handle, 2.0f, 2.0f, 2.0f);
                            }*/

                            //  完全停止からスロー
                            m_timeScale = 0.0f;          // 完全停止
                            m_hitStopTimer = 0.04f;      // 0.08秒間フリーズ
                            m_slowMoTimer = 0.0f;


                            // カメラシェイク
                            m_cameraShake = 0.3f;
                            m_cameraShakeTimer = 0.5f;
                        }
                        else
                        {
                            // === ボディショット処理 ===
                            ////OutputDebugStringA("[BULLET] Body shot\n");

                            // === デバッグ: ダメージ前のHP ===
                            //char debugHP1[256];
                            //sprintf_s(debugHP1, "[DEBUG] Enemy ID:%d HP BEFORE damage: %df\n",
                            //    hitEnemy->id, hitEnemy->health);
                            ////OutputDebugStringA(debugHP1);

                            //// === デバッグ: 武器のダメージ ===
                            //char debugDamage[256];
                            //sprintf_s(debugDamage, "[DEBUG] Weapon damage: %d (WeaponType:%d)\n",
                            //    weapon.damage, (int)currentWeapon);
                            ////OutputDebugStringA(debugDamage);

                            //m_particleSystem->CreateBloodEffect(rayResult.hitPoint, shotDir, 15);
                            m_gpuParticles->EmitSplash(rayResult.hitPoint, shotDir, 15, 4.0f);
                            m_gpuParticles->EmitMist(rayResult.hitPoint, 30, 1.5f);
                            hitEnemy->health -= weapon.damage;
                            m_statDamageDealt += weapon.damage;  //  与ダメ記録

                            // === デバッグ: ダメージ後のHP ===
                            /*char debugHP2[256];
                            sprintf_s(debugHP2, "[DEBUG] Enemy ID:%d HP AFTER damage: %d\n",
                                hitEnemy->id, hitEnemy->health);
                            //OutputDebugStringA(debugHP2);*/


                            // カメラシェイク（強化）
                            m_cameraShake = 0.15f;
                            m_cameraShakeTimer = 0.15f;

                            // 死亡時の吹っ飛ばし
                            if (hitEnemy->health <= 0.0f)
                            {
                                // === デバッグ: ラグドール処理に入った ===
                                ////OutputDebugStringA("[DEBUG] Entering ragdoll processing (HP <= 0.0f)\n");

                                hitEnemy->isRagdoll = true;

                                //  カメラシェイク
                                m_cameraShake = 0.25f;
                                m_cameraShakeTimer = 0.2;

                                m_bloodSystem->OnEnemyKilled(hitEnemy->position, m_player->GetPosition());

                                float knockbackPower = 10.0f;

                                switch (currentWeapon)
                                {
                                case WeaponType::PISTOL:
                                    knockbackPower = 8.0f;
                                    break;
                                case WeaponType::SHOTGUN:
                                    knockbackPower = 25.0f;
                                    break;
                                case WeaponType::RIFLE:
                                    knockbackPower = 12.0f;
                                    break;
                                case WeaponType::SNIPER:
                                    knockbackPower = 20.0f;
                                    break;
                                }

                                hitEnemy->velocity.x = shotDir.x * knockbackPower;
                                hitEnemy->velocity.y = 5.0f;
                                hitEnemy->velocity.z = shotDir.z * knockbackPower;
                            }
                            else
                            {
                                // === デバッグ: ラグドール処理に入らなかった ===
                               ////OutputDebugStringA("[DEBUG] NOT entering ragdoll (HP > 0.0f)\n");
                            }
                        }

                        // ノックバック
                        /*float knockbackStrength = isHeadShot ? 0.5f : 0.2f;
                        hitEnemy->position.x += shotDir.x * knockbackStrength;
                        hitEnemy->position.z += shotDir.z * knockbackStrength;*/

                        // 死亡処理
                        if (hitEnemy->health <= 0)
                        {
                            // === デバッグ: 死亡判定に入った ===
                            ////OutputDebugStringA("[DEBUG] Entering death check (HP <= 0)\n");

                            if (!hitEnemy->isDying)
                            {
                                // === デバッグ: 死亡処理実行 ===
                                /*char debugDeath[256];
                                sprintf_s(debugDeath, "[DEBUG] Setting isDying=true, isAlive=false for enemy ID:%d\n",
                                    hitEnemy->id);*/
                                    /*//OutputDebugStringA(debugDeath);*/

                                hitEnemy->isDying = true;
                                hitEnemy->isAlive = false;
                                hitEnemy->currentAnimation = "Death";
                                hitEnemy->animationTime = 0.0f;
                                // 変更後（ボスは早く消す）:
                                if (hitEnemy->type == EnemyType::BOSS || hitEnemy->type == EnemyType::MIDBOSS)
                                    hitEnemy->corpseTimer = 2.0f;  // ボスは2秒で消す
                                else
                                    hitEnemy->corpseTimer = 5.0f;

                                // ===  死んだ瞬間に物理ボディを削除 ===
                                RemoveEnemyPhysicsBody(hitEnemy->id);

                                m_bloodSystem->OnEnemyKilled(hitEnemy->position, m_player->GetPosition());
                                m_gpuParticles->EmitSplash(hitEnemy->position, XMFLOAT3(hitEnemy->position.x - m_player->GetPosition().x, hitEnemy->position.y - m_player->GetPosition().y, hitEnemy->position.z - m_player->GetPosition().z), 40, 7.0f);
                                m_gpuParticles->EmitMist(rayResult.hitPoint, 100, 2.0f);
                                m_gpuParticles->Emit(hitEnemy->position, 60, 5.0f);

                                // MIDBOSSが死んだらビーム停止
                                if (hitEnemy->type == EnemyType::MIDBOSS &&
                                    m_beamHandle >= 0 && m_effekseerManager != nullptr)
                                {
                                    m_effekseerManager->StopEffect(m_beamHandle);
                                    m_beamHandle = -1;
                                }

                                // ポイント加算
                                int waveBonus = m_waveManager->OnEnemyKilled();
                                if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);
                                int totalPoints = (isHeadShot ? 150 : 60) + waveBonus;
                                m_player->AddPoints(totalPoints);
                                SpawnScorePopup(totalPoints, isHeadShot ? ScorePopupType::HEADSHOT : ScorePopupType::KILL);

                                //  スタイルランク：キル通知
                                m_styleRank->NotifyKill(isHeadShot);

                                //  スタッツ記録
                                m_statKills++;
                                if (isHeadShot) m_statHeadshots++;

                                // ダメージ表示
                                m_showDamageDisplay = true;
                                m_damageDisplayTimer = 2.0f;
                                m_damageDisplayPos = hitEnemy->position;
                                m_damageDisplayPos.y += 2.0f;
                                m_damageValue = isHeadShot ? 150 : 60;
                            }

                            else
                            {
                                // === デバッグ: すでにisDying=true ===
                                /*//OutputDebugStringA("[DEBUG] Enemy already dying (isDying=true)\n");*/
                            }



                            // ウィンドウタイトル更新
                            /*char debug[256];
                            sprintf_s(debug, isHeadShot ? "HEADSHOT!! Points:%d" : "Hit! Points:%d",
                                m_player->GetPoints());*/
                                //SetWindowTextA(m_window, debug);
                        }
                        else
                        {
                            // === デバッグ: 死亡判定に入らなかった ===
                            /*//OutputDebugStringA("[DEBUG] NOT entering death check (HP > 0)\n");*/
                        }
                    }
                    else
                    {
                        // 敵に当たらなかった（壁など）
                       /* //OutputDebugStringA("[BULLET] Hit wall or object\n");*/
                       // 敵に当たらなかった（壁など）→ 壁に血痕を貼る

                    }
                }
                else
                {
                    // 何にも当たらなかった
                    // 法線がほぼ水平 → 壁ヒット

                }
            }
        }
        m_lastMouseState = currentMouseState;

    }

}

// ============================================================
//  UpdateAmmoAndReload - 弾切れ警告/リロード処理
//
//  【処理内容】
//  - 弾切れ時の警告αアニメーション
//  - リロード中の進捗更新
//  - リロードフェーズアニメーション
// ============================================================
void Game::UpdateAmmoAndReload(float deltaTime)
{
    // --- 1回押した時だけ反応するキー入力（リロード用） ---
    static std::map<int, bool> keyWasPressedReload;
    auto IsFirstKeyPress = [&](int vk_code) {
        bool isPressed = GetAsyncKeyState(vk_code) & 0x8000;
        if (isPressed && !keyWasPressedReload[vk_code]) {
            keyWasPressedReload[vk_code] = true;
            return true;
        }
        if (!isPressed) {
            keyWasPressedReload[vk_code] = false;
        }
        return false;
        };

    // === 弾切れ警告の更新 ===
    {

        bool outOfAmmo = (m_weaponSystem->GetCurrentAmmo() == 0) &&
            !m_weaponSystem->IsReloading();

        if (outOfAmmo)
        {
            // 警告フェードイン
            m_reloadWarningTimer += deltaTime;
            m_reloadWarningAlpha += deltaTime * 5.0f;
            if (m_reloadWarningAlpha > 1.0f) m_reloadWarningAlpha = 1.0f;
        }
        else
        {
            // 警告フェードアウト
            m_reloadWarningAlpha -= deltaTime * 8.0f;
            if (m_reloadWarningAlpha < 0.0f)
            {
                m_reloadWarningAlpha = 0.0f;
                m_reloadWarningTimer = 0.0f;
            }
        }
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

    // === リロード中の処理 ===
    if (m_weaponSystem->IsReloading())
    {
        // リロード残り時間を減らす
        float newTimer = m_weaponSystem->GetReloadTimer() - deltaTime;
        m_weaponSystem->SetReloadTimer(newTimer);

        // 現在の武器のリロード時間を取得して、進行度（0→1）を計算
        auto& weapon = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());
        float totalTime = weapon.reloadTime;                // 例：ショットガンなら1.0秒
        float progress = 1.0f - (newTimer / totalTime);     // 0.0（開始）→ 1.0（完了）
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        m_reloadAnimProgress = progress;

        // --- フェーズアニメーション ---
        // フェーズ1（0?30%）: 武器を下に下げる
        if (progress < 0.3f)
        {
            float t = progress / 0.3f;    // このフェーズ内の進行度（0→1）
            t = t * t;                     // イーズイン：最初ゆっくり→だんだん速く
            m_reloadAnimOffset = t * -0.4f; // 最大0.4m下に下げる
            m_reloadAnimTilt = t * 0.5f;    // 最大0.5rad手前に傾ける
        }
        // フェーズ2（30?70%）: 画面下で停止（マガジン交換中のイメージ）
        else if (progress < 0.7f)
        {
            m_reloadAnimOffset = -0.4f;     // 下がったまま
            m_reloadAnimTilt = 0.5f;        // 傾いたまま
        }
        // フェーズ3（70?100%）: 武器を元の位置に戻す
        else
        {
            float t = (progress - 0.7f) / 0.3f;         // このフェーズ内の進行度（0→1）
            t = 1.0f - (1.0f - t) * (1.0f - t);         // イーズアウト：最初速い→だんだんゆっくり
            m_reloadAnimOffset = -0.4f * (1.0f - t);     // 下から元の位置へ
            m_reloadAnimTilt = 0.5f * (1.0f - t);        // 傾きも元に戻る
        }

        // リロード完了判定
        if (newTimer <= 0.0f)
        {
            // 弾を補充する
            int needed = m_weaponSystem->GetMaxAmmo() - m_weaponSystem->GetCurrentAmmo();
            int reload = min(needed, m_weaponSystem->GetReserveAmmo());
            m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetCurrentAmmo() + reload);
            m_weaponSystem->SetReserveAmmo(m_weaponSystem->GetReserveAmmo() - reload);
            m_weaponSystem->SetReloading(false);
            m_weaponSystem->SaveAmmoStatus();

            // アニメーションをリセット
            m_reloadAnimProgress = 0.0f;
            m_reloadAnimOffset = 0.0f;
            m_reloadAnimTilt = 0.0f;
        }
    }
    else
    {
        // リロードしてない時：残ってる動きを滑らかにゼロへ戻す
        // powを使ってフレームレートに依存しない減衰にする
        float decay = powf(0.85f, m_deltaTime * 60.0f);  // 60FPS基準で同じ速度
        m_reloadAnimOffset *= decay;
        m_reloadAnimTilt *= decay;
    }


    if (m_showDamageDisplay)
    {
        m_damageDisplayTimer -= deltaTime; // 60FPS想定
        if (m_damageDisplayTimer <= 0.0f)
        {
            m_showDamageDisplay = false;
        }
    }

    //  アニメーションを進める
   /* if (m_effekseerManager != nullptr)
    {
        m_effekseerManager->Update(1.0f / 60.0f);
    }*/

    m_particleSystem->Update(deltaTime);

    m_bloodSystem->Update(deltaTime);
}

// ============================================================
//  UpdateEnemyAI - パスファインディング/分離押し出し/死亡処理
//
//  【処理内容】
//  - グローリーキル中は停止
//  - 敵の旧座標保存
//  - A*パスファインディング（経路再計算タイマー付き）
//  - 敵同士の重なり押し出し
//  - 死亡処理（isDying → 物理ボディ削除 → isAlive=false）
// ============================================================
void Game::UpdateEnemyAI(float deltaTime)
{
    //  グローリーキル中は敵の更新を停止
    if (!m_gloryKillCameraActive)
    {
        // === 敵の旧座標を保存（パス追従の基準に使う） ===
        struct EnemyOldPos { int id; float x, z; };
        std::vector<EnemyOldPos> enemyOldPositions;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.isAlive && !enemy.isDying && !enemy.isRagdoll)
                enemyOldPositions.push_back({ enemy.id, enemy.position.x, enemy.position.z });
        }
        //  【意味】プレイヤーの現在位置
        //  【用途】敵がプレイヤーに向かって動くため
        m_enemySystem->Update(deltaTime, m_player->GetPosition());

        if (m_navGrid.IsReady())
        {
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                // 死亡中・ラグドール・スタガー中・ボス系はスキップ
                if (!enemy.isAlive || enemy.isDying || enemy.isRagdoll
                    || enemy.isStaggered
                    || enemy.type == EnemyType::MIDBOSS
                    || enemy.type == EnemyType::BOSS)
                    continue;

                // --- 経路再計算タイマー ---
                enemy.aiPathTimer -= deltaTime;
                if (enemy.aiPathTimer <= 0.0f)
                {
                    enemy.aiPathTimer = 0.5f + (float)(enemy.id % 5) * 0.1f;

                    enemy.aiPath = m_navGrid.FindPath(
                        enemy.position.x, enemy.position.z,
                        playerPos.x, playerPos.z);
                    enemy.aiPathIndex = 0;

                    if (enemy.aiPath.size() > 1)
                        enemy.aiPathIndex = 1;
                }

                // パスが無い or 短い → EnemySystemのまま
                if (enemy.aiPath.empty() || enemy.aiPathIndex >= (int)enemy.aiPath.size())
                {
                    enemy.aiControlled = false;
                    continue;
                }
                // プレイヤーが近い → 直接移動
                float dxP = playerPos.x - enemy.position.x;
                float dzP = playerPos.z - enemy.position.z;
                if (sqrtf(dxP * dxP + dzP * dzP) < 3.0f)
                {
                    enemy.aiPath.clear();
                    enemy.aiControlled = false;
                    continue;
                }

                // 旧座標を取得
                float oldX = enemy.position.x;
                float oldZ = enemy.position.z;
                for (const auto& op : enemyOldPositions)
                {
                    if (op.id == enemy.id) { oldX = op.x; oldZ = op.z; break; }
                }

                // EnemySystemの移動を取り消し、パスに沿って移動
                DirectX::XMFLOAT3& waypoint = enemy.aiPath[enemy.aiPathIndex];

                float dx = waypoint.x - oldX;
                float dz = waypoint.z - oldZ;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist > 0.1f)
                {
                    enemy.aiControlled = true;  // A*が制御中

                    float nx = dx / dist;
                    float nz = dz / dist;

                    // プレイヤーとの距離で速度を決定
                    float distToPlayer = sqrtf(dxP * dxP + dzP * dzP);
                    float runThreshold = 8.0f;

                    float walkSpeed = 2.5f;
                    float runSpeed = 5.0f;
                    switch (enemy.type)
                    {
                    case EnemyType::NORMAL: walkSpeed = 2.5f; runSpeed = 4.5f; break;
                    case EnemyType::RUNNER: walkSpeed = 3.0f; runSpeed = 6.0f; break;
                    case EnemyType::TANK:   walkSpeed = 1.5f; runSpeed = 2.5f; break;
                    }

                    bool shouldRun = (distToPlayer > runThreshold);
                    float speed = shouldRun ? runSpeed : walkSpeed;
                    speed *= m_enemySystem->m_waveSpeedMult;

                    enemy.position.x = oldX + nx * speed * deltaTime;
                    enemy.position.z = oldZ + nz * speed * deltaTime;
                    enemy.rotationY = atan2f(nx, nz) + 3.14159f;

                    // velocityをEnemySystemと整合させる
                    float velMag = shouldRun ? 2.0f : 0.5f;
                    enemy.velocity.x = nx * velMag;
                    enemy.velocity.z = nz * velMag;

                    // アニメーション設定
                    std::string moveAnim = shouldRun ? "Run" : "Walk";
                    if (enemy.currentAnimation != moveAnim &&
                        enemy.currentAnimation != "Death" &&
                        enemy.currentAnimation != "Stun")
                    {
                        enemy.currentAnimation = moveAnim;
                        enemy.animationTime = 0.0f;
                    }
                }

                // ウェイポイントに近づいたら次へ
                float dxW = waypoint.x - enemy.position.x;
                float dzW = waypoint.z - enemy.position.z;
                if (sqrtf(dxW * dxW + dzW * dzW) < 1.0f)
                {
                    enemy.aiPathIndex++;
                }
            }
        }
    }

    // =============================================
     // === 敵同士の重なり押し出し（A*移動後） ===
     // =============================================
    {
        auto& enemies = m_enemySystem->GetEnemies();
        const float collisionRadius = 0.5f;
        const float minDist = collisionRadius * 2.0f;  // 1.0m以内で押し出し
        const float pushStr = 0.08f;

        for (size_t i = 0; i < enemies.size(); i++)
        {
            if (!enemies[i].isAlive || enemies[i].isDying) continue;

            for (size_t j = i + 1; j < enemies.size(); j++)
            {
                if (!enemies[j].isAlive || enemies[j].isDying) continue;

                float dx = enemies[j].position.x - enemies[i].position.x;
                float dz = enemies[j].position.z - enemies[i].position.z;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist < minDist && dist > 0.001f)
                {
                    float nx = dx / dist;
                    float nz = dz / dist;
                    float overlap = (minDist - dist) * pushStr;

                    enemies[i].position.x -= nx * overlap;
                    enemies[i].position.z -= nz * overlap;
                    enemies[j].position.x += nx * overlap;
                    enemies[j].position.z += nz * overlap;
                }
            }
        }
    }

    //  === 敵の「移動・回転・アニメーション更新・死亡」をまとめて制御するループ   ===
    //DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (enemy.isAlive && !enemy.isDying)
        {
            UpdateEnemyPhysicsBody(enemy);
        }

        // === 死亡処理 ===
        if (enemy.isDying)
        {
            enemy.corpseTimer -= deltaTime;
            if (enemy.corpseTimer <= 0.0f)
            {
                enemy.isAlive = false;
                enemy.isDying = false;
                RemoveEnemyPhysicsBody(enemy.id);

                continue;
            }
        }

        //  ========================================
        //  アニメーション更新
        //  ========================================
        if (enemy.type != EnemyType::BOSS && enemy.type != EnemyType::MIDBOSS &&
            m_enemyModel && m_enemyModel->HasAnimation(enemy.currentAnimation))
        {
            float duration = m_enemyModel->GetAnimationDuration(enemy.currentAnimation);
            // ↑ ここで duration が定義される

            if (enemy.isDying)
            {
                //  死亡中: 終端で止める（タイプ別速度）
                int typeIdx = 0;
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  typeIdx = 0; break;
                case EnemyType::RUNNER:  typeIdx = 1; break;
                case EnemyType::TANK:    typeIdx = 2; break;
                case EnemyType::MIDBOSS: typeIdx = 3; break;
                case EnemyType::BOSS:    typeIdx = 4; break;
                }
                float deathSpeed = m_enemySystem->m_animSpeed_Death[typeIdx];
                enemy.animationTime += deltaTime * deathSpeed;
                if (enemy.animationTime >= duration)  // ← ここで使える
                {
                    enemy.animationTime = duration - 0.001f;
                }
            }
            else
            {
                //  通常: ループ（タイプ×アニメ別速度）
                int typeIdx = 0;
                switch (enemy.type)
                {
                case EnemyType::NORMAL:  typeIdx = 0; break;
                case EnemyType::RUNNER:  typeIdx = 1; break;
                case EnemyType::TANK:    typeIdx = 2; break;
                case EnemyType::MIDBOSS: typeIdx = 3; break;
                case EnemyType::BOSS:    typeIdx = 4; break;
                }

                float animSpeed = 0.1f;
                if (enemy.currentAnimation == "Idle")        animSpeed = m_enemySystem->m_animSpeed_Idle[typeIdx];
                else if (enemy.currentAnimation == "Walk")   animSpeed = m_enemySystem->m_animSpeed_Walk[typeIdx];
                else if (enemy.currentAnimation == "Run")    animSpeed = m_enemySystem->m_animSpeed_Run[typeIdx];
                else if (enemy.currentAnimation == "Attack") animSpeed = 0.0f;
                else if (enemy.currentAnimation == "AttackJump") animSpeed = 0.0f;
                else if (enemy.currentAnimation == "AttackSlash") animSpeed = 0.0f;
                else                                         animSpeed = 0.1f;

                enemy.animationTime += deltaTime * animSpeed;
                if (enemy.animationTime >= duration)
                {
                    enemy.animationTime = 0.0f;
                }
            }
        }

        //  === 首から血を噴き出す   ===
        if (enemy.headDestroyed && enemy.isAlive)
        {
            // 敵の向いている方向の前方ベクトル
            float forwardX = -sinf(enemy.rotationY);
            float forwardZ = -cosf(enemy.rotationY);

            // 首は体の中心から前方に少しずらした位置
            DirectX::XMFLOAT3 neckPosition;
            neckPosition.x = enemy.position.x + forwardX * 1.8f;
            neckPosition.z = enemy.position.z + forwardZ * 1.8f;

            // アニメーション進行度で高さを調整
            if (enemy.isDying && m_enemyModel && m_enemyModel->HasAnimation("Death"))
            {
                float deathDuration = m_enemyModel->GetAnimationDuration("Death");
                float progress = enemy.animationTime / deathDuration;
                progress = min(progress, 1.0f);

                // 立ってる時: 1.5m → 倒れた時: 0.3m
                float neckHeight = 1.5f - (progress * 1.2f);
                neckPosition.y = enemy.position.y + neckHeight;
            }
            else
            {
                // 通常時
                neckPosition.y = enemy.position.y + 1.5f;
            }

            // 血を噴出（保存した弾の方向を使う）
            //m_particleSystem->CreateBloodEffect(
            //    neckPosition,
            //    enemy.bloodDirection,  //   弾が飛んできた方向
            //    3
            //);
        }
    }
    UpdatePhysics(deltaTime);

    //  死んだ敵を削除 毎フレーム敵を削除しないと、配列か肥大化
    m_enemySystem->ClearDeadEnemies();

    //  ウェーブ管理
    m_waveManager->Update(1.0f / 60.0f, m_player->GetPosition(), m_enemySystem.get());

}

// ============================================================
//  UpdateWaveAndEffects - ウェーブ管理/スコアHUD/爪痕エフェクト
//
//  【処理内容】
//  - ウェーブマネージャー更新
//  - ウェーブバナー検出（新ウェーブ開始時）
//  - スコアHUD更新（血しぶき背景タイマー）
//  - 爪痕エフェクト検出（背後からの攻撃のみ）
//  - 新スポーン敵の物理ボディ//
// ============================================================
void Game::UpdateWaveAndEffects(float deltaTime)
{
    // === ウェーブバナー検出 ===
    {
        int currentWave = m_waveManager->GetCurrentWave();
        if (currentWave != m_lastWaveNumber)
        {
            m_lastWaveNumber = currentWave;
            m_waveBannerTimer = m_waveBannerDuration;
            m_waveBannerNumber = currentWave;
            m_waveBannerIsBoss = (currentWave % 2 == 0) || (currentWave % 3 == 0);
        }
        if (m_waveBannerTimer > 0.0f)
            m_waveBannerTimer -= 1.0f / 60.0f;
    }

    // === スコアHUD更新 ===
    {
        float dt = 1.0f / 60.0f;
        int realScore = m_player->GetPoints();

        // スコアが増えたら演出発火
        if (realScore > m_lastDisplayedScore)
        {
            int diff = realScore - m_lastDisplayedScore;
            m_scoreFlashTimer = 0.4f;
            m_scoreShakeTimer = 0.2f;
        }
        m_lastDisplayedScore = realScore;

        // 表示値をスムーズに追従（スロットマシン風に回る）
        float diff = (float)realScore - m_scoreDisplayValue;
        if (fabsf(diff) < 1.0f)
            m_scoreDisplayValue = (float)realScore;
        else
            m_scoreDisplayValue += diff * 8.0f * dt;

        if (m_scoreFlashTimer > 0.0f) m_scoreFlashTimer -= dt;
        if (m_scoreShakeTimer > 0.0f) m_scoreShakeTimer -= dt;
    }

    // === 爪痕エフェクト検出（背後からの攻撃のみ） ===
    {
        static int lastHP = 100;
        int curHP = m_player->GetHealth();
        if (curHP < lastHP && curHP > 0)
        {
            // プレイヤーの前方ベクトル
            float playerYaw = m_player->GetRotation().y;
            float forwardX = sinf(playerYaw);
            float forwardZ = cosf(playerYaw);
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

            // 一番近い生きてる敵を探す
            float closestDist = 999.0f;
            bool isBehind = false;
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                float dx = enemy.position.x - playerPos.x;
                float dz = enemy.position.z - playerPos.z;
                float dist = sqrtf(dx * dx + dz * dz);
                if (dist < closestDist && dist < 10.0f)
                {
                    closestDist = dist;
                    // 内積: 正=前方、負=背後
                    float dot = forwardX * dx + forwardZ * dz;
                    isBehind = (dot < 0.0f);
                }
            }

            if (isBehind)
                m_clawTimer = m_clawDuration;
        }
        lastHP = curHP;

        if (m_clawTimer > 0.0f)
            m_clawTimer -= 1.0f / 60.0f;
    }

    //  全敵のY座標を床の高さに合わせる
    //  全敵のY座標をメッシュ床の高さに合わせる
    {
        for (auto& enemy : m_enemySystem->GetEnemiesMutable())
        {
            if (enemy.isAlive)
            {
                //  ボスのジャンプ中はY座標を上書きしない
                if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS) &&
                    (enemy.bossPhase == BossAttackPhase::JUMP_AIR ||
                        enemy.bossPhase == BossAttackPhase::JUMP_SLAM))
                    continue;

                enemy.position.y = GetMeshFloorHeight(
                    enemy.position.x, enemy.position.z, enemy.position.y);
            }
        }
    }

    // ボススポーンエフェクト
    if (m_effectBossSpawn != nullptr && m_effekseerManager != nullptr)
    {
        if (m_waveManager->DidMidBossSpawn())
        {
            // MIDBOSSの位置を探してエフェクト再生
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive)
                {
                    auto h = m_effekseerManager->Play(m_effectBossSpawn,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    m_effekseerManager->SetScale(h, 3.0f, 3.0f, 3.0f);
                    break;
                }
            }
        }
        if (m_waveManager->DidBossSpawn())
        {
            // BOSSの位置を探してエフェクト再生
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::BOSS && enemy.isAlive)
                {
                    auto h = m_effekseerManager->Play(m_effectBossSpawn,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    m_effekseerManager->SetScale(h, 5.0f, 5.0f, 5.0f);
                    break;
                }
            }
        }
    }

    // 雑魚敵スポーンエフェクト
    if (m_effectEnemySpawn != nullptr && m_effekseerManager != nullptr)
    {
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (enemy.justSpawned)
            {
                // ボス系は BossSpawn で出すからスキップ
                if (enemy.type != EnemyType::MIDBOSS && enemy.type != EnemyType::BOSS)
                {
                    auto h = m_effekseerManager->Play(m_effectEnemySpawn,
                        enemy.position.x, enemy.position.y, enemy.position.z);
                    m_effekseerManager->SetScale(h, 1.5f, 1.5f, 1.5f);
                }

                enemy.justSpawned = false;
            }
        }
    }


}

// ============================================================
//  UpdateShieldSystem - 盾投擲/ガード/チャージ/パリィ
//
//  【処理内容】
//  - ベストターゲット追跡（最も近い敵を自動ロック）
//  - 盾投擲（前進→敵にヒット→戻り）
//  - ガード状態管理（パリィウィンドウ）
//  - チャージ攻撃（溜め→解放）
//  - シールドHUDアニメーション更新
//  - シールドバッシュタイマー
// ============================================================
void Game::UpdateShieldSystem(float deltaTime)
{
    //  新しくスポーンされた敵に物理ボディを//
    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (enemy.isAlive)
        {
            //  子の敵がまだ物理ボディを持っていないか確認
            if (m_enemyPhysicsBodies.find(enemy.id) == m_enemyPhysicsBodies.end())
            {
                AddEnemyPhysicsBody(enemy);
            }
        }
    }


    // ==============================================
    // シールドシステム入力処理
    // ==============================================
    static bool rmbPressed = false;
    bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    switch (m_shieldState)
    {
        // ─── 通常状態 ───
    case ShieldState::Idle:
    {
        // 右クリック押した瞬間 → パリィウィンドウ開始
        if (rmbDown && !rmbPressed)
        {
            m_shieldState = ShieldState::Parrying;
            m_parryWindowTimer = m_parryWindowDuration;  // 0.15秒
            m_lastParryAttemptTime = m_gameTime;
            m_shieldBashTimer = m_shieldBashDuration;    // 盾アニメ開始
            //OutputDebugStringA("[SHIELD] State: PARRYING (window open)\n");
        }

        // 盾を下ろすアニメ（ゆっくり戻す）
        m_shieldGuardBlend = max(0.0f, m_shieldGuardBlend - deltaTime * 5.0f);

        //  ガードしてないときはエネルギーリセット
        m_chargeEnergy = 0.0f;
        m_chargeReady = false;

        // シールドHP回復（非ガード時、遅延後）
        if (m_shieldRegenDelayTimer > 0.0f)
        {
            m_shieldRegenDelayTimer -= deltaTime;
        }
        else if (m_shieldHP < m_shieldMaxHP)
        {
            m_shieldHP = min(m_shieldMaxHP, m_shieldHP + m_shieldRegenRate * deltaTime);
        }
        break;
    }

    // ─── パリィ受付中（右クリ押した直後の短い時間）───
    case ShieldState::Parrying:
    {
        m_parryWindowTimer -= deltaTime;

        // 盾を構えるアニメ（素早く上げる）
        m_shieldGuardBlend = min(1.0f, m_shieldGuardBlend + deltaTime * 10.0f);

        if (m_parryWindowTimer <= 0.0f)
        {
            // パリィウィンドウ終了
            if (rmbDown)
            {
                // まだ押してる → ガードに移行
                m_shieldState = ShieldState::Guarding;
                m_isGuarding = true;
                //OutputDebugStringA("[SHIELD] State: GUARDING (hold)\n");
            }
            else
            {
                // もう離した → タップだった、Idleに戻る
                m_shieldState = ShieldState::Idle;
                //OutputDebugStringA("[SHIELD] State: IDLE (parry tap ended)\n");
            }
        }
        break;
    }

    // ─── ガード中（長押し）───
    case ShieldState::Guarding:
    {
        // 盾を表示し続ける
        m_shieldGuardBlend = min(1.0f, m_shieldGuardBlend + deltaTime * 10.0f);

        // 盾アニメを表示位置でキープ
        m_shieldBashTimer = m_shieldBashDuration * 0.5f;

        //  チャージエネルギー蓄積
        if (m_chargeEnergy < 1.0f)
        {
            m_chargeEnergy += m_chargeEnergyRate * deltaTime;
            if (m_chargeEnergy >= 1.0f)
            {
                m_chargeEnergy = 1.0f;
                m_chargeReady = true;
                //OutputDebugStringA("[SHIELD] Charge energy FULL! Ready to charge!\n");
            }
        }

        // === 毎フレーム: ベストターゲットを追跡 ===
        {
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            float camYaw = m_player->GetRotation().y;
            float camPitch = m_player->GetRotation().x;

            DirectX::XMFLOAT3 lookDir;
            lookDir.x = sinf(camYaw) * cosf(camPitch);
            lookDir.y = -sinf(camPitch);
            lookDir.z = cosf(camYaw) * cosf(camPitch);

            float bestScore = -1.0f;
            m_guardLockTargetID = -1;

            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;

                float dx = enemy.position.x - playerPos.x;
                float dy = enemy.position.y - playerPos.y;
                float dz = enemy.position.z - playerPos.z;
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                if (dist < 1.0f || dist > 50.0f) continue;

                float nx = dx / dist;
                float ny = dy / dist;
                float nz = dz / dist;

                float dot = nx * lookDir.x + ny * lookDir.y + nz * lookDir.z;

                if (dot > 0.7f)
                {
                    float score = dot / dist;
                    if (score > bestScore)
                    {
                        bestScore = score;
                        m_guardLockTargetID = enemy.id;
                        m_guardLockTargetPos = enemy.position;
                    }
                }
            }
        }

        bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (lmbDown && m_chargeReady && m_guardLockTargetID >= 0)
        {
            // ロックオン成功 → チャージ開始！
            m_shieldState = ShieldState::Charging;
            m_chargeTimer = 0.0f;
            m_chargeEnergy = 0.0f;
            m_chargeReady = false;
            m_chargeHasTarget = true;
            m_chargeTargetEnemyID = m_guardLockTargetID;
            m_chargeTarget = m_guardLockTargetPos;

            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            float dx = m_guardLockTargetPos.x - playerPos.x;
            float dz = m_guardLockTargetPos.z - playerPos.z;
            float len = sqrtf(dx * dx + dz * dz);
            if (len > 0.01f)
                m_chargeDirection = { dx / len, 0.0f, dz / len };
            else
            {
                float camYaw = m_player->GetRotation().y;
                m_chargeDirection = { sinf(camYaw), 0.0f, cosf(camYaw) };
            }

            float distToTarget = len;
            m_chargeDuration = min(0.8f, max(0.2f, distToTarget / m_chargeSpeed));
        }

        if (!rmbDown)
        {
            // 右クリック離す → Idleに戻る
            m_shieldState = ShieldState::Idle;
            m_isGuarding = false;
            m_guardLockTargetID = -1;
            //OutputDebugStringA("[SHIELD] State: IDLE (guard released)\n");
        }
        break;
    }

    // ??? シールドチャージ突進中 ???
    case ShieldState::Charging:
    {
        m_chargeTimer += deltaTime;

        //  FOV拡大＋スピードライン
        m_targetFOV = 100.0f;
        m_speedLineAlpha = 0.8f;

        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        playerPos.x += m_chargeDirection.x * m_chargeSpeed * deltaTime;
        playerPos.z += m_chargeDirection.z * m_chargeSpeed * deltaTime;
        m_player->SetPosition(playerPos);

        float dxTarget = m_chargeTarget.x - playerPos.x;
        float dzTarget = m_chargeTarget.z - playerPos.z;
        float distToTarget = sqrtf(dxTarget * dxTarget + dzTarget * dzTarget);

        bool reachedTarget = (distToTarget < 2.0f);  // within 2m of target
        bool timeUp = (m_chargeTimer >= m_chargeDuration);

        if (m_mapSystem && m_mapSystem->CheckCollision(playerPos, 0.5f))
        {
            reachedTarget = true;
            //OutputDebugStringA("[CHARGE] Hit wall! Exploding here.\n");
        }

        if (reachedTarget || timeUp)
        {

            DirectX::XMFLOAT3 blastCenter = m_player->GetPosition();
            float blastRadius = 5.0f;

            int killCount = 0;
            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;

                float dx = enemy.position.x - blastCenter.x;
                float dz = enemy.position.z - blastCenter.z;
                float dist = sqrtf(dx * dx + dz * dz);

                if (dist > blastRadius) continue;

                if (enemy.type == EnemyType::NORMAL || enemy.type == EnemyType::RUNNER)
                {
                    enemy.health = 0;
                    enemy.isDying = true;
                    enemy.isAlive = false;
                    enemy.isRagdoll = true;
                    enemy.isExploded = true;
                    enemy.corpseTimer = 3.0f;

                    RemoveEnemyPhysicsBody(enemy.id);

                    // スクリーンブラッド
                    {
                        DirectX::XMFLOAT3 pPos = m_player->GetPosition();
                        float bx = enemy.position.x - pPos.x;
                        float bz = enemy.position.z - pPos.z;
                        float bDist = sqrtf(bx * bx + bz * bz);
                        if (bDist < 8.0f)
                        {
                            float intensity = 1.0f - (bDist / 8.0f);
                            m_bloodSystem->OnExplosionKill(enemy.position, m_player->GetPosition());
                            m_gpuParticles->EmitSplash(enemy.position, XMFLOAT3(enemy.position.x - m_player->GetPosition().x, enemy.position.y - m_player->GetPosition().y, enemy.position.z - m_player->GetPosition().z), 50, 9.0f);
                            m_gpuParticles->EmitMist(enemy.position, 150, 3.5f);
                            m_gpuParticles->Emit(enemy.position, 80, 7.0f);
                        }
                    }

                    int waveBonus = m_waveManager->OnEnemyKilled();
                    if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);
                    m_player->AddPoints(100 + waveBonus);
                    SpawnScorePopup(100 + waveBonus, ScorePopupType::MELEE_KILL);

                    //  肉片を生成（8個、爆発力15）
                    SpawnGibs(enemy.position, 12, 18.0f);


                    DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                    m_particleSystem->CreateBloodEffect(enemy.position, upDir, 400);
                    m_particleSystem->CreateExplosion(enemy.position);


                    for (int a = 0; a < 6; a++)
                    {
                        float rad = a * 1.047f;
                        DirectX::XMFLOAT3 radDir = { cosf(rad), 0.3f, sinf(rad) };
                        m_particleSystem->CreateBloodEffect(enemy.position, radDir, 40);
                    }

                    killCount++;
                    m_statKills++;       //  総キル
                    m_statMeleeKills++;  //  近接キル
                }
                else
                {
                    // TANK / MIDBOSS / BOSS へのダメージ
                    enemy.health -= 80;
                    m_statDamageDealt += 80;

                    // 死ななくても血を出す
                    DirectX::XMFLOAT3 hitDir = {
                        enemy.position.x - blastCenter.x,
                        0.5f,  // やや上向き
                        enemy.position.z - blastCenter.z
                    };
                    m_gpuParticles->EmitSplash(enemy.position, hitDir, 40, 7.0f);
                    m_gpuParticles->EmitMist(enemy.position, 100, 2.5f);
                    m_gpuParticles->Emit(enemy.position, 30, 4.0f);

                    //  タンクだけ確定よろめき
                    if (enemy.type == EnemyType::TANK && !enemy.isStaggered)
                    {
                        enemy.isStaggered = true;
                        enemy.staggerFlashTimer = 0.0f;
                        enemy.staggerTimer = 0.0f;
                        enemy.stunValue = 0.0f;
                        enemy.currentAnimation = "Stun";
                        enemy.animationTime = 0.0f;
                    }
                    else
                    {
                        // MIDBOSS/BOSS はスタン蓄積のみ
                        enemy.stunValue += 60.0f;
                        if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                        {
                            enemy.isStaggered = true;
                            enemy.staggerFlashTimer = 0.0f;
                            enemy.staggerTimer = 0.0f;
                            enemy.stunValue = 0.0f;
                            enemy.currentAnimation = "Stun";
                            enemy.animationTime = 0.0f;
                        }
                    }

                    if (enemy.health <= 0)
                    {
                        enemy.health = 0;
                        enemy.isDying = true;
                        enemy.isAlive = false;
                        enemy.isRagdoll = true;
                        enemy.corpseTimer = 3.0f;
                        RemoveEnemyPhysicsBody(enemy.id);
                        m_bloodSystem->OnMeleeKill(enemy.position);
                        m_gpuParticles->EmitSplash(enemy.position,
                            XMFLOAT3(enemy.position.x - m_player->GetPosition().x,
                                enemy.position.y - m_player->GetPosition().y,
                                enemy.position.z - m_player->GetPosition().z), 45, 8.0f);
                        m_gpuParticles->EmitMist(enemy.position, 120, 2.5f);
                        m_gpuParticles->Emit(enemy.position, 70, 6.0f);
                        m_statKills++;
                        m_statMeleeKills++;

                        int waveBonus = m_waveManager->OnEnemyKilled();
                        m_player->AddPoints(150 + waveBonus);
                    }
                }
            }

            DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
            //m_particleSystem->CreateBloodEffect(blastCenter, upDir, 300);
            //m_particleSystem->CreateExplosion(blastCenter);

            /*char buf[128];
            sprintf_s(buf, "[CHARGE] AOE EXPLOSION! %d grunts obliterated!\n", killCount);*/
            //OutputDebugStringA(buf);

            // Camera shake (kills数に応じて強さ変化)
            m_cameraShake = 0.3f + killCount * 0.15f;      // 1体:0.45  3体:0.75  5体:1.05
            m_cameraShakeTimer = 0.3f;

            // Slow-mo (短めでキレのある演出)
            if (killCount > 0)
            {
                m_timeScale = 0.15f;
                m_slowMoTimer = 0.02f + killCount * 0.02f;

                m_weaponSystem->SetCurrentAmmo(m_weaponSystem->GetMaxAmmo());
                int curReserve = m_weaponSystem->GetReserveAmmo();
                int maxReserve = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon()).reserveAmmo;
                int newReserve = curReserve + killCount * 10;
                if (newReserve > maxReserve) newReserve = maxReserve;
                m_weaponSystem->SetReserveAmmo(newReserve);
            }

            //  FOVを戻す
            m_targetFOV = 70.0f;
            m_speedLineAlpha = 0.0f;

            m_shieldState = ShieldState::Guarding;
            m_isGuarding = true;
            m_chargeHasTarget = false;
            m_chargeTargetEnemyID = -1;
        }

        m_shieldGuardBlend = 1.0f;
        break;
    }

    case ShieldState::Throwing:
    {
        //  ガード不可（盾が手元にない）
        m_shieldGuardBlend = max(0.0f, m_shieldGuardBlend - deltaTime * 10.0f);

        // 回転（見た目のスピン）
        m_thrownShieldSpin += deltaTime * 20.0f;


        if (!m_thrownShieldReturning)
        {
            // === 前進（直線） ===
            float move = m_thrownShieldSpeed * deltaTime;
            m_thrownShieldPos.x += m_thrownShieldDir.x * move;
            m_thrownShieldPos.y += m_thrownShieldDir.y * move;
            m_thrownShieldPos.z += m_thrownShieldDir.z * move;
            m_thrownShieldDist += move;

            // 最大距離で折り返し
            if (m_thrownShieldDist >= m_thrownShieldMaxDist)
            {
                m_thrownShieldReturning = true;
                //OutputDebugStringA("[SHIELD] Max distance, returning!\n");
            }
        }
        else
        {
            // === 戻り（プレイヤーに向かう） ===
            DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
            float dx = playerPos.x - m_thrownShieldPos.x;
            float dy = playerPos.y - m_thrownShieldPos.y;
            float dz = playerPos.z - m_thrownShieldPos.z;
            float dist = sqrtf(dx * dx + dy * dy + dz * dz);

            if (dist < 2.0f)
            {
                // プレイヤーに到達 → 盾復活！
                m_shieldState = ShieldState::Idle;

                //  Effekseerトレイル停止
                if (m_shieldTrailHandle >= 0)
                {
                    m_effekseerManager->StopEffect(m_shieldTrailHandle);
                    m_shieldTrailHandle = -1;
                }

                //OutputDebugStringA("[SHIELD] Shield caught! Back to Idle.\n");
                break;
            }

            // 方向を正規化
            float invDist = 1.0f / dist;
            dx *= invDist;
            dy *= invDist;
            dz *= invDist;

            // 戻り速度は少し速く（50）
            float returnSpeed = 50.0f * deltaTime;
            m_thrownShieldPos.x += dx * returnSpeed;
            m_thrownShieldPos.y += dy * returnSpeed;
            m_thrownShieldPos.z += dz * returnSpeed;
        }

        //  Effekseer: SetBaseMatrixで盾に追従
        if (m_shieldTrailHandle >= 0 && m_effekseerManager != nullptr)
        {
            m_effekseerManager->SetLocation(m_shieldTrailHandle,
                Effekseer::Vector3D(
                    m_thrownShieldPos.x,
                    m_thrownShieldPos.y,
                    m_thrownShieldPos.z));
        }



        // === 敵との当たり判定 ===
        for (auto& enemy : m_enemySystem->GetEnemies())
        {
            if (!enemy.isAlive || enemy.isDying) continue;

            float ex = enemy.position.x - m_thrownShieldPos.x;
            float ey = (enemy.position.y + 0.8f) - m_thrownShieldPos.y;
            float ez = enemy.position.z - m_thrownShieldPos.z;
            float distSq = ex * ex + ey * ey + ez * ez;
            float hitR = m_thrownShieldHitRadius;

            if (distSq < hitR * hitR)
            {
                // 重複チェック（行きと帰りで別管理）
                auto& hitSet = m_thrownShieldReturning
                    ? m_thrownShieldReturnHitEnemies
                    : m_thrownShieldHitEnemies;

                if (hitSet.find(enemy.id) == hitSet.end())
                {
                    hitSet.insert(enemy.id);

                    // ダメージ
                    enemy.health -= (int)m_thrownShieldDamage;
                    enemy.staggerFlashTimer = 0.15f;

                    //  死亡チェック
                    if (enemy.health <= 0)
                    {
                        enemy.health = 0;
                        enemy.isDying = true;
                        enemy.corpseTimer = 3.0f;

                        // === メッシュ切断 ===
                        SliceEnemyWithShield(enemy, m_thrownShieldDir);

                        // 切断後に物理ボディ削除＆isAlive=false
                        enemy.isAlive = false;
                        RemoveEnemyPhysicsBody(enemy.id);
                        int waveBonus = m_waveManager->OnEnemyKilled();
                        if (waveBonus > 0) SpawnScorePopup(waveBonus, ScorePopupType::WAVE_BONUS);
                        m_player->AddPoints(100 + waveBonus);
                        SpawnScorePopup(100 + waveBonus, ScorePopupType::SHIELD_KILL);
                    }

                    // スタン
                    enemy.stunValue = enemy.maxStunValue;

                    // 血エフェクト
                    DirectX::XMFLOAT3 upDir = { 0.0f, 1.0f, 0.0f };
                    //m_particleSystem->CreateBloodEffect(enemy.position, upDir, 80);
                    //  盾投げヒット血エフェクト（GPU）
                    {
                        // 盾の飛行方向を血の飛散方向に使う
                        DirectX::XMFLOAT3 splashDir = m_thrownShieldDir;
                        splashDir.y += 0.3f;  // 少し上向きに飛ばす

                        // 敵の位置（少し上＝胴体）からスプラッシュ
                        DirectX::XMFLOAT3 hitPos = enemy.position;
                        hitPos.y += 0.8f;

                        m_gpuParticles->EmitSplash(hitPos, splashDir, 25, 6.0f);
                        m_gpuParticles->EmitMist(hitPos, 40, 1.5f);
                    }

                    // カメラシェイク（軽め）
                    m_cameraShake = 0.15f;
                    m_cameraShakeTimer = 0.1f;

                    // ヒットストップ（短め）
                    m_hitStopTimer = 0.03f;

                    /*char msg[128];
                    sprintf_s(msg, "[SHIELD THROW] Hit enemy ID:%d! HP:%d\n",
                        enemy.id, enemy.health);*/
                        //OutputDebugStringA(msg);
                }
            }
        }

        break;
    }

    // ─── 盾破壊（一時使用不能）───
    case ShieldState::Broken:
    {
        m_shieldGuardBlend = max(0.0f, m_shieldGuardBlend - deltaTime * 3.0f);
        m_isGuarding = false;
        m_shieldBrokenTimer -= deltaTime;

        if (m_shieldBrokenTimer <= 0.0f)
        {
            // 復帰！
            m_shieldState = ShieldState::Idle;
            m_shieldHP = m_shieldMaxHP * 0.3f;  // 30%で復帰
            //OutputDebugStringA("[SHIELD] State: IDLE (shield restored!)\n");
        }
        break;
    }
    }

    rmbPressed = rmbDown;

    // === シールドHUD アニメーション更新 ===
    {
        // 表示HPを実際のHPに滑らかに追従させる
        // 減る時はゆっくり（ダメージ演出）、回復時は速い
        float lerpSpeed = (m_shieldDisplayHP > m_shieldHP) ? 3.0f : 8.0f;
        m_shieldDisplayHP += (m_shieldHP - m_shieldDisplayHP) * min(1.0f, lerpSpeed * deltaTime);

        // グロウ強度の更新（ガード/パリィで光る）
        float targetGlow = 0.0f;
        if (m_shieldState == ShieldState::Guarding)
            targetGlow = 0.7f;
        else if (m_shieldState == ShieldState::Parrying)
            targetGlow = 1.0f;
        m_shieldGlowIntensity += (targetGlow - m_shieldGlowIntensity) * min(1.0f, 12.0f * deltaTime);
    }

    // GPU血パーティクルに地面の高さを伝える
    {
        DirectX::XMFLOAT3 pPos = m_player->GetPosition();
        float groundY = GetMeshFloorHeight(pPos.x, pPos.z, 0.0f);
        m_gpuParticles->SetFloorY(groundY + 0.02f);  // 地面のちょっと上
    }

    m_gpuParticles->Update(deltaTime);

    UpdateSlicedPieces(deltaTime);

    //  回復アイテム更新
    UpdateHealthPickups(deltaTime);
    UpdateScorePopups(deltaTime);


    // パリィ成功エフェクトタイマー
    if (m_parryFlashTimer > 0.0f)
    {
        m_parryFlashTimer -= deltaTime;
    }

    // 盾バッシュタイマー（Idle時のみ減少、Guarding時はキープ）
    if (m_shieldState != ShieldState::Guarding && m_shieldState != ShieldState::Charging && m_shieldBashTimer > 0.0f)
    {
        m_shieldBashTimer -= deltaTime;
        if (m_shieldBashTimer < 0.0f)
            m_shieldBashTimer = 0.0f;
    }

}

// ============================================================
//  UpdateBossAttacks - ボス攻撃/接触ダメージ/弾幕
//
//  【処理内容】
//  - 接触ダメージ判定（敵→プレイヤー）
//  - ボスAIフェーズ管理
//    - ジャンプ叩きつけ（範囲ダメージ）
//    - 斬撃3本発射（BOSS専用）
//    - 咆哮ビーム（MIDBOSS専用）
//  - 弾幕（プロジェクタイル）の移動/当たり判定/削除
//  - Effekseer更新
//  - 死亡済み敵のクリーンアップ
// ============================================================
void Game::UpdateBossAttacks(float deltaTime)
{
    // === 接触ダメージ判定 ===
    for (auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive || enemy.isDying)
            continue;

        if ((enemy.type == EnemyType::MIDBOSS) &&
            enemy.bossPhase != BossAttackPhase::ROAR_FIRE &&
            m_beamHandle >= 0 && m_effekseerManager != nullptr)
        {
            m_effekseerManager->StopEffect(m_beamHandle);
            m_beamHandle = -1;
        }

        //  === ボス特殊攻撃の処理 ===
        if ((enemy.type == EnemyType::BOSS || enemy.type == EnemyType::MIDBOSS) &&
            enemy.bossPhase != BossAttackPhase::IDLE &&
            enemy.attackJustLanded)
        {
            // --- ジャンプ叩きつけ：範囲ダメージ ---
            if (enemy.bossPhase == BossAttackPhase::JUMP_SLAM)
            {
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
                float dx = playerPos.x - enemy.position.x;
                float dz = playerPos.z - enemy.position.z;
                float dist = sqrtf(dx * dx + dz * dz);

                float slamRadius = (enemy.type == EnemyType::BOSS) ? m_slamRadiusBoss : m_slamRadiusMidBoss;
                float slamDamage = ((enemy.type == EnemyType::BOSS) ? m_slamDamageBoss : m_slamDamageMidBoss) * enemy.damageMultiplier;  //  ウェーブ倍率

                if (dist < slamRadius)
                {
                    // パリィ/ガード判定
                    if (m_shieldState == ShieldState::Parrying)
                    {
                        // ジャンプ叩きはパリィ可能
                        m_shieldState = ShieldState::Idle;
                        m_parrySuccess = true;
                        // パリィ時ヒットストップ
                        m_hitStopTimer = 0.06f;
                        m_timeScale = 0.0f;
                        m_slowMoTimer = 0.0f;   // 残留slowMo防止

                        // パリィ成功→近接チャージ回復
                        if (m_meleeCharges < m_meleeMaxCharges)
                            m_meleeCharges++;

                        m_lastParryResultTime = m_gameTime;
                        m_lastParryWasSuccess = true;
                        m_parrySuccessCount++;
                        m_player->AddPoints(150);
                        SpawnScorePopup(150, ScorePopupType::PARRY);
                        m_styleRank->NotifyParry();
                        m_parryFlashTimer = 0.3f;

                        // パリィエフェクト再生
                        if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                        {
                            DirectX::XMFLOAT3 pos = m_player->GetPosition();
                            DirectX::XMFLOAT3 rot = m_player->GetRotation();
                            float fx = pos.x + sinf(rot.y) * 1.0f;
                            float fz = pos.z + cosf(rot.y) * 1.0f;
                            auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                            m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                        }

                        float stunDamage = m_slamStunDamage;
                        enemy.stunValue += stunDamage;
                        if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                        {
                            enemy.isStaggered = true;
                            enemy.staggerFlashTimer = 0.0f;
                            enemy.staggerTimer = 0.0f;
                            enemy.stunValue = 0.0f;
                            enemy.currentAnimation = "Stun";
                            enemy.animationTime = 0.0f;
                        }

                        m_cameraShake = 0.2f;
                        m_cameraShakeTimer = 0.3f;
                        m_hitStopTimer = 0.1f;
                        m_timeScale = 0.2f;
                        m_slowMoTimer = 0.4f;

                        //OutputDebugStringA("[BOSS] JUMP SLAM PARRIED!\n");
                    }
                    else if (m_shieldState == ShieldState::Guarding)
                    {
                        // ガード：HPダメージなし、盾HPのみ消費
                        m_shieldHP -= slamDamage * 0.5f;
                        m_shieldRegenDelayTimer = m_shieldRegenDelay;
                        m_cameraShake = 0.1f;
                        m_cameraShakeTimer = 0.15f;

                        if (m_shieldHP <= 0.0f)
                        {
                            m_shieldHP = 0.0f;
                            m_shieldState = ShieldState::Broken;
                            m_shieldBrokenTimer = m_shieldBrokenDuration;
                        }
                    }
                    else
                    {
                        // ノーガード：フルダメージ
                        bool died = m_player->TakeDamage((int)slamDamage);
                        m_statDamageTaken += (int)slamDamage;  //  被ダメ記録
                        m_cameraShake = 0.3f;
                        m_cameraShakeTimer = 0.3f;
                        if (died) m_gameState = GameState::GAMEOVER;

                        //OutputDebugStringA("[BOSS] JUMP SLAM HIT! (no guard)\n");
                    }
                }

                // 叩きつけで画面揺れ（距離に関係なく）
                m_cameraShake = (std::max)(m_cameraShake, 0.08f);
                m_cameraShakeTimer = (std::max)(m_cameraShakeTimer, 0.2f);

                // 地面エフェクトはここで後で//（Effekseer）
                if (m_particleSystem)
                {
                    //m_particleSystem->CreateExplosion(enemy.position);
                }

                //  衝撃波エフェクト//
                if (m_effectGroundSlam != nullptr && m_effekseerManager != nullptr)
                {
                    m_effekseerManager->Play(
                        m_effectGroundSlam,
                        enemy.position.x, enemy.position.y + 0.1f, enemy.position.z);
                }

                enemy.attackJustLanded = false;
                continue;  // このenemyの処理は終わり
            }

            // --- 斬撃3本発射（BOSS） ---
            if (enemy.bossPhase == BossAttackPhase::SLASH_FIRE)
            {
                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

                // 敵→プレイヤーの方向
                float dx = playerPos.x - enemy.position.x;
                float dz = playerPos.z - enemy.position.z;
                float dist = sqrtf(dx * dx + dz * dz);
                if (dist < 0.1f) dist = 0.1f;
                float nx = dx / dist;
                float nz = dz / dist;

                // 3本の斬撃を扇状に発射
                // 角度オフセット: -15度, 0度, +15度
                float angles[5] = { -0.70f, -0.35f, 0.0f, 0.35f, 0.70f };  // ラジアン（約15度）

                for (int i = 0; i < 5; i++)
                {
                    BossProjectile proj;
                    // 方向を回転
                    float cosA = cosf(angles[i]);
                    float sinA = sinf(angles[i]);
                    proj.direction.x = nx * cosA - nz * sinA;
                    proj.direction.y = 0.0f;
                    proj.direction.z = nx * sinA + nz * cosA;

                    // 発射位置（ボスの前方1m）
                    proj.position.x = enemy.position.x + proj.direction.x * 1.0f;
                    proj.position.y = enemy.position.y + 1.0f;
                    proj.position.z = enemy.position.z + proj.direction.z * 1.0f;

                    proj.speed = m_slashSpeed;
                    proj.lifetime = 3.0f;
                    proj.damage = m_slashDamage * enemy.damageMultiplier;
                    proj.ownerID = enemy.id;
                    proj.isActive = true;

                    // 真ん中の1本だけパリィ可能（緑）
                    proj.isParriable = (rand() % 2 == 0);

                    // エフェクト再生＆ハンドル保存
                    Effekseer::EffectRef slashEffect = proj.isParriable
                        ? m_effectSlashGreen : m_effectSlashRed;
                    if (slashEffect != nullptr && m_effekseerManager != nullptr)
                    {
                        Effekseer::Handle h = m_effekseerManager->Play(
                            slashEffect,
                            proj.position.x, proj.position.y, proj.position.z);
                        float angle = atan2f(proj.direction.x, proj.direction.z);
                        m_effekseerManager->SetRotation(h, 0.0f, angle, 0.0f);
                        m_effekseerManager->SetScale(h, 0.6f, 0.3f, 0.3f);
                        proj.effectHandle = h;
                    }

                    m_bossProjectiles.push_back(proj);

                    /* char buf[128];
                     sprintf_s(buf, "[BOSS] Slash projectile %d fired! Parriable: %s\n",
                         i, proj.isParriable ? "YES(green)" : "NO(red)");*/
                         //OutputDebugStringA(buf);
                }

                enemy.attackJustLanded = false;
                continue;
            }

            // --- 咆哮ビーム（MIDBOSS） ---
            if (enemy.bossPhase == BossAttackPhase::ROAR_FIRE)
            {
                // ビームエフェクト再生（初回のみ
                Effekseer::EffectRef beamEffect = enemy.bossBeamParriable
                    ? m_effectBeamGreen : m_effectBeamRed;
                if (beamEffect != nullptr && m_beamHandle < 0 && m_effekseerManager != nullptr)
                {
                    m_beamHandle = m_effekseerManager->Play(
                        beamEffect,
                        enemy.position.x, enemy.position.y + 1.5f, enemy.position.z);
                    m_effekseerManager->SetScale(m_beamHandle, 0.5f, 0.5f, 0.5f);
                }

                // ビームの位置と向きを毎フレーム更新
                if (m_beamHandle >= 0 && m_effekseerManager != nullptr)
                {
                    m_effekseerManager->SetLocation(m_beamHandle,
                        Effekseer::Vector3D(enemy.position.x, enemy.position.y + 1.5f, enemy.position.z));
                    float angle = atan2f(
                        enemy.bossTargetPos.x - enemy.position.x,
                        enemy.bossTargetPos.z - enemy.position.z) + 3.14159f;
                    m_effekseerManager->SetRotation(m_beamHandle, 0.0f, angle, 0.0f);
                }


                DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

                //  ビーム到達遅延（発射から3.5秒後にプレイヤーに届く）
                float beamArrivalTime = 2.0f;  // ROAR_FIRE開始からの到達時間
                bool beamReached = (enemy.bossPhaseTimer >= beamArrivalTime);

                // ビームの方向計算（エフェクト表示用、到達前も必要）
                float bdx = enemy.bossTargetPos.x - enemy.position.x;
                float bdz = enemy.bossTargetPos.z - enemy.position.z;
                float bDist = sqrtf(bdx * bdx + bdz * bdz);
                if (bDist < 0.1f) bDist = 0.1f;
                float bnx = bdx / bDist;
                float bnz = bdz / bDist;

                float px = playerPos.x - enemy.position.x;
                float pz = playerPos.z - enemy.position.z;

                float projDist = px * bnx + pz * bnz;
                float perpDist = fabs(px * (-bnz) + pz * bnx);

                float beamWidth = m_beamWidth;
                float beamLength = m_beamLength;
                float beamDPS = m_beamDPS;

                // // ビーム到達後のみダメージ判定
                if (beamReached && projDist > 0.0f && projDist < beamLength && perpDist < beamWidth)
                {
                    float frameDamage = beamDPS * (1.0f / 60.0f) * enemy.damageMultiplier;  //  ウェーブ倍率

                    if (m_shieldState == ShieldState::Parrying && enemy.bossBeamParriable)
                    {
                        // 緑ビーム → パリィ成功！
                        m_shieldState = ShieldState::Idle;
                        m_parrySuccess = true;
                        // パリィ時ヒットストップ
                        m_hitStopTimer = 0.06f;
                        m_timeScale = 0.0f;
                        m_slowMoTimer = 0.0f;   // 残留slowMo防止

                        // パリィ成功→近接チャージ回復
                        if (m_meleeCharges < m_meleeMaxCharges)
                            m_meleeCharges++;

                        m_parryFlashTimer = 0.3f;
                        m_player->AddPoints(150);
                        SpawnScorePopup(150, ScorePopupType::PARRY);
                        m_styleRank->NotifyParry();

                        // パリィエフェクト再生
                        if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                        {
                            DirectX::XMFLOAT3 pos = m_player->GetPosition();
                            DirectX::XMFLOAT3 rot = m_player->GetRotation();
                            float fx = pos.x + sinf(rot.y) * 1.0f;
                            float fz = pos.z + cosf(rot.y) * 1.0f;
                            auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                            m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                        }

                        enemy.stunValue += m_beamStunOnParry * 2.0f;
                        if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                        {
                            enemy.isStaggered = true;
                            enemy.staggerFlashTimer = 0.0f;
                            enemy.staggerTimer = 0.0f;
                            enemy.stunValue = 0.0f;
                            enemy.currentAnimation = "Stun";
                            enemy.animationTime = 0.0f;
                        }

                        // パリィ成功でビーム中断！
                        enemy.bossPhase = BossAttackPhase::ROAR_RECOVERY;
                        enemy.bossPhaseTimer = 0.0f;

                        m_cameraShake = 0.25f;
                        m_cameraShakeTimer = 0.3f;
                        m_hitStopTimer = 0.15f;
                        m_timeScale = 0.15f;
                        m_slowMoTimer = 0.5f;

                        if (m_beamHandle >= 0 && m_effekseerManager != nullptr)
                        {
                            m_effekseerManager->StopEffect(m_beamHandle);
                            m_beamHandle = -1;
                        }

                        //OutputDebugStringA("[MIDBOSS] BEAM PARRIED!\n");
                    }
                    else if (m_shieldState == ShieldState::Parrying && !enemy.bossBeamParriable)
                    {
                        // 赤ビーム：パリィ不可→ガード扱い、HPダメージなし
                        m_shieldHP -= frameDamage * 0.3f;
                        m_shieldRegenDelayTimer = m_shieldRegenDelay;
                    }
                    else if (m_shieldState == ShieldState::Guarding)
                    {
                        // ガード：HPダメージなし、盾HPのみ消費
                        m_shieldHP -= frameDamage * 0.3f;
                        m_shieldRegenDelayTimer = m_shieldRegenDelay;

                        if (m_shieldHP <= 0.0f)
                        {
                            m_shieldHP = 0.0f;
                            m_shieldState = ShieldState::Broken;
                            m_shieldBrokenTimer = m_shieldBrokenDuration;
                        }
                    }
                    else
                    {
                        m_statDamageTaken += (int)(std::max)(1.0f, frameDamage);  //被ダメ
                        bool died = m_player->TakeDamage((int)(std::max)(1.0f, frameDamage));
                        if (died) m_gameState = GameState::GAMEOVER;
                    }
                }

                continue;
            }

            // 上記以外のボス攻撃フェーズは通常のattackJustLandedにフォールスルー
            enemy.attackJustLanded = false;
            continue;
        }

        // 攻撃が「今まさに当たった瞬間」のみ判定
        if (enemy.attackJustLanded)
        {
            if (m_gloryKillInvincibleTimer > 0)
                break;

            // パリィウィンドウ中 → パリィ成功！
            if (m_shieldState == ShieldState::Parrying)
            {
                // パリィ成功 → シールドHP消費なし
                m_shieldState = ShieldState::Idle;
                m_parrySuccess = true;
                // パリィ時ヒットストップ
                m_hitStopTimer = 0.06f;
                m_timeScale = 0.0f;
                m_slowMoTimer = 0.0f;   // 残留slowMo防止

                // パリィ成功→近接チャージ回復
                if (m_meleeCharges < m_meleeMaxCharges)
                    m_meleeCharges++;

                m_lastParryResultTime = m_gameTime;
                m_lastParryWasSuccess = true;
                m_parrySuccessCount++;
                m_player->AddPoints(75);
                SpawnScorePopup(75, ScorePopupType::PARRY);
                m_styleRank->NotifyParry();
                m_parryFlashTimer = 0.3f;

                // パリィエフェクト再生
                if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                {
                    DirectX::XMFLOAT3 pos = m_player->GetPosition();
                    DirectX::XMFLOAT3 rot = m_player->GetRotation();
                    float fx = pos.x + sinf(rot.y) * 1.0f;
                    float fz = pos.z + cosf(rot.y) * 1.0f;
                    auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                    m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                }

                if (enemy.type == EnemyType::NORMAL || enemy.type == EnemyType::RUNNER)
                {
                    enemy.isStaggered = true;
                    enemy.staggerFlashTimer = 0.0f;
                    enemy.staggerTimer = 0.0f;
                    enemy.currentAnimation = "Stun";
                    enemy.animationTime = 0.0f;

                    /* char buf[128];
                     sprintf_s(buf, "[PARRY] SUCCESS! Enemy ID:%d STAGGERED instantly!\n", enemy.id);*/
                     //OutputDebugStringA(buf);
                }
                else
                {
                    float stunDamage = 40.0f;
                    enemy.stunValue += stunDamage;

                    /*char buf[256];
                    sprintf_s(buf, "[PARRY] SUCCESS! Enemy ID:%d Stun: %.0f/%.0f\n",
                        enemy.id, enemy.stunValue, enemy.maxStunValue);*/
                        //OutputDebugStringA(buf);

                    if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                    {
                        enemy.isStaggered = true;
                        enemy.staggerFlashTimer = 0.0f;
                        enemy.staggerTimer = 0.0f;
                        enemy.stunValue = 0.0f;
                        enemy.currentAnimation = "Stun";
                        enemy.animationTime = 0.0f;

                        /*sprintf_s(buf, "[PARRY] STUN BREAK! Enemy ID:%d is now STAGGERED!\n", enemy.id);*/
                        //OutputDebugStringA(buf);
                    }
                }

                // フィードバック
                m_cameraShake = 0.15f;
                m_cameraShakeTimer = 0.2f;
                m_hitStopTimer = 0.08f;
                m_timeScale = 0.3f;
                m_slowMoTimer = 0.3f;

                if (m_particleSystem)
                {
                    XMFLOAT3 sparkDir(0.0f, 1.0f, 0.0f);
                    /*m_particleSystem->CreateBloodEffect(
                        enemy.position, sparkDir, 30);*/
                }

                //OutputDebugStringA("[PARRY] === TIMING PARRY! ===\n");
                break;
            }
            else if (m_shieldState == ShieldState::Guarding)
            {
                float rawDamage;
                switch (enemy.type)
                {
                case EnemyType::RUNNER: rawDamage = 8.0f;  break;
                case EnemyType::TANK:   rawDamage = 25.0f; break;
                default:                rawDamage = 10.0f; break;
                }
                rawDamage *= enemy.damageMultiplier;

                // // HPダメージなし！盾HPのみ消費
                m_shieldHP -= rawDamage * 0.5f;
                m_shieldRegenDelayTimer = m_shieldRegenDelay;

                m_cameraShake = 0.05f;
                m_cameraShakeTimer = 0.1f;

                if (m_shieldHP <= 0.0f)
                {
                    m_shieldHP = 0.0f;
                    m_shieldState = ShieldState::Broken;
                    m_shieldBrokenTimer = m_shieldBrokenDuration;
                    m_isGuarding = false;
                }
                break;
            }
            else
            {

                float rawDamage;
                switch (enemy.type)
                {
                case EnemyType::RUNNER: rawDamage = 8.0f;  break;
                case EnemyType::TANK:   rawDamage = 25.0f; break;
                default:                rawDamage = 10.0f; break;
                }
                rawDamage *= enemy.damageMultiplier;  //  ウェーブ倍率

                bool died = m_player->TakeDamage((int)rawDamage);
                m_statDamageTaken += (int)rawDamage;  //  被ダメ記録

                m_cameraShake = 0.1f;
                m_cameraShakeTimer = 0.15f;

                //OutputDebugStringA("[ENEMY] Hit player! No guard - full damage!\n");

                if (died) m_gameState = GameState::GAMEOVER;
                break;
            }

            enemy.attackJustLanded = false;
        }
        // 継続ダメージ（密着し続けてる場合、一定間隔でダメージ）
        else if (enemy.touchingPlayer && m_shieldState == ShieldState::Idle)
        {
            if (m_gloryKillInvincibleTimer <= 0)
            {

            }
        }
    }


    // ================================================== =
    // ボスプロジェクタイル更新 & 当たり判定
    // ===================================================
    {
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

        for (auto& proj : m_bossProjectiles)
        {
            if (!proj.isActive) continue;

            // --- 移動 ---
            proj.position.x += proj.direction.x * proj.speed * deltaTime;
            proj.position.z += proj.direction.z * proj.speed * deltaTime;

            //  エフェクト追従
            if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
            {
                m_effekseerManager->SetLocation(proj.effectHandle,
                    Effekseer::Vector3D(proj.position.x, proj.position.y, proj.position.z));
            }

            // --- 寿命 ---
            proj.lifetime -= deltaTime;
            if (proj.lifetime <= 0.0f)
            {
                // エフェクト停止
                if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                {
                    m_effekseerManager->StopEffect(proj.effectHandle);
                    proj.effectHandle = -1;
                }

                proj.isActive = false;
                continue;
            }

            // --- プレイヤーとの当たり判定 ---
            float dx = playerPos.x - proj.position.x;
            float dz = playerPos.z - proj.position.z;
            float dist = sqrtf(dx * dx + dz * dz);

            float hitRadius = m_slashHitRadius;  // 斬撃の当たり判定の幅

            if (dist < hitRadius)
            {
                // ========================================
                // パリィ可能（緑）＋ パリィ状態 → パリィ成功！
                // ========================================
                if (proj.isParriable && m_shieldState == ShieldState::Parrying)
                {
                    m_shieldState = ShieldState::Idle;
                    m_parrySuccess = true;
                    // パリィ時ヒットストップ
                    m_hitStopTimer = 0.06f;
                    m_timeScale = 0.0f;
                    m_slowMoTimer = 0.0f;   // 残留slowMo防止

                    // パリィ成功→近接チャージ回復
                    if (m_meleeCharges < m_meleeMaxCharges)
                        m_meleeCharges++;

                    m_lastParryResultTime = m_gameTime;
                    m_lastParryWasSuccess = true;
                    m_parrySuccessCount++;

                    // パリィエフェクト再生
                    if (m_effectParry != nullptr && m_effekseerManager != nullptr)
                    {
                        DirectX::XMFLOAT3 pos = m_player->GetPosition();
                        DirectX::XMFLOAT3 rot = m_player->GetRotation();
                        float fx = pos.x + sinf(rot.y) * 1.0f;
                        float fz = pos.z + cosf(rot.y) * 1.0f;
                        auto h = m_effekseerManager->Play(m_effectParry, fx, pos.y, fz);
                        m_effekseerManager->SetScale(h, 2.0f, 2.0f, 2.0f);
                    }

                    m_parryFlashTimer = 0.3f;

                    // 発射元のボスにスタンダメージ
                    for (auto& enemy : m_enemySystem->GetEnemies())
                    {
                        if (enemy.id == proj.ownerID && enemy.isAlive)
                        {
                            enemy.stunValue += m_slashStunOnParry;
                            if (enemy.stunValue >= enemy.maxStunValue && !enemy.isStaggered)
                            {
                                enemy.isStaggered = true;
                                enemy.staggerFlashTimer = 0.0f;
                                enemy.staggerTimer = 0.0f;
                                enemy.stunValue = 0.0f;
                                enemy.currentAnimation = "Stun";
                                enemy.animationTime = 0.0f;
                                //OutputDebugStringA("[PROJECTILE] PARRY -> BOSS STAGGERED!\n");
                            }
                            break;
                        }
                    }

                    m_cameraShake = 0.2f;
                    m_cameraShakeTimer = 0.25f;
                    m_hitStopTimer = 0.1f;
                    m_timeScale = 0.2f;
                    m_slowMoTimer = 0.4f;
                    m_player->AddPoints(150);
                    SpawnScorePopup(150, ScorePopupType::PARRY);
                    m_styleRank->NotifyParry();

                    // エフェクト停止
                    if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                    {
                        m_effekseerManager->StopEffect(proj.effectHandle);
                        proj.effectHandle = -1;
                    }

                    proj.isActive = false;
                    //OutputDebugStringA("[PROJECTILE] GREEN SLASH PARRIED!\n");
                }
                // ========================================
                // ガード中 → ダメージ軽減（赤もガード可能）
                // ========================================
                else if (m_shieldState == ShieldState::Guarding)
                {
                    float reduced = proj.damage * (1.0f - m_guardDamageReduction);
                    m_statDamageTaken += (int)reduced;  //  被ダメ記録
                    bool died = m_player->TakeDamage((int)reduced);
                    m_shieldHP -= proj.damage * 0.4f;
                    m_shieldRegenDelayTimer = m_shieldRegenDelay;

                    m_cameraShake = 0.08f;
                    m_cameraShakeTimer = 0.1f;

                    if (m_shieldHP <= 0.0f)
                    {
                        m_shieldHP = 0.0f;
                        m_shieldState = ShieldState::Broken;
                        m_shieldBrokenTimer = m_shieldBrokenDuration;
                    }
                    if (died) m_gameState = GameState::GAMEOVER;

                    // エフェクト停止
                    if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                    {
                        m_effekseerManager->StopEffect(proj.effectHandle);
                        proj.effectHandle = -1;
                    }

                    proj.isActive = false;

                    /* char buf[64];
                     sprintf_s(buf, "[PROJECTILE] %s SLASH GUARDED!\n",
                         proj.isParriable ? "GREEN" : "RED");*/
                         //OutputDebugStringA(buf);
                }
                // ========================================
                // ノーガード → フルダメージ
                // ========================================
                else
                {
                    m_statDamageTaken += (int)proj.damage;  //  被ダメ記録
                    bool died = m_player->TakeDamage((int)proj.damage);
                    m_cameraShake = 0.15f;
                    m_cameraShakeTimer = 0.2f;
                    if (died) m_gameState = GameState::GAMEOVER;

                    // エフェクト停止
                    if (proj.effectHandle >= 0 && m_effekseerManager != nullptr)
                    {
                        m_effekseerManager->StopEffect(proj.effectHandle);
                        proj.effectHandle = -1;
                    }

                    proj.isActive = false;

                    /*char buf[64];
                    sprintf_s(buf, "[PROJECTILE] %s SLASH HIT! (no guard)\n",
                        proj.isParriable ? "GREEN" : "RED");*/
                        //OutputDebugStringA(buf);
                }
            }
        }

        // --- 消えたプロジェクタイルを削除 ---
        m_bossProjectiles.erase(
            std::remove_if(m_bossProjectiles.begin(), m_bossProjectiles.end(),
                [](const BossProjectile& p) { return !p.isActive; }),
            m_bossProjectiles.end()
        );
    }


    //  Effekseer更新
    if (m_effekseerManager != nullptr)
    {
        m_effekseerManager->Update(1.0f);
    }

    auto& enemies = m_enemySystem->GetEnemies();
    enemies.erase(
        std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return !e.isAlive && !e.isDying; }),
        enemies.end()
    );

}

// ============================================================
//  UpdateGloryKillAnimation - グローリーキルの腕/ナイフアニメーション
//
//  【処理】アニメーション時間を進め、完了時にリセット
//  腕の突き出し→引き戻しの往復モーションを制御
// ============================================================
void Game::UpdateGloryKillAnimation(float deltaTime)
{
    if (!m_gloryKillArmAnimActive)
        return;

    // アニメーション時間を進める
    m_gloryKillArmAnimTime += deltaTime / GLORY_KILL_ANIM_DURATION;

    if (m_gloryKillArmAnimTime >= 1.0f)
    {
        // アニメーション終了
        m_gloryKillArmAnimActive = false;
        m_gloryKillArmAnimTime = 0.0f;
        m_gloryKillArmAnimRot = { 0.0f, 0.0f, 0.0f };
        m_gloryKillKnifeAnimRot = { 0.0f, 0.0f, 0.0f };
        return;
    }

    float t = m_gloryKillArmAnimTime;

    // === フェーズ分け ===
    // Phase 1: 構え (0.0 ~ 0.2) - 腕を右に引いて横向きに
    // Phase 2: 溜め (0.2 ~ 0.35) - 止まる
    // Phase 3: 突き刺し (0.35 ~ 0.5) - 素早く左へ突く
    // Phase 4: 停止 (0.5 ~ 0.75) - 刺さった状態
    // Phase 5: 引き抜き (0.75 ~ 1.0) - 戻す

    float armRotY = 0.0f;   // 腕の横方向回転
    float armRotZ = 0.0f;   // 腕の傾き
    float knifeRotX = 0.0f; // ナイフの角度

    if (t < 0.2f)
    {
        // Phase 1: 構え - 腕を右に引く
        float phase = t / 0.2f;  // 0→1
        phase = phase * phase;   // イージング（加速）

        armRotY = phase * 0.8f;  // 右に引く
        armRotZ = phase * 0.3f;  // 少し傾ける
        knifeRotX = phase * (-0.5f); // ナイフを横向きに
    }
    else if (t < 0.35f)
    {
        // Phase 2: 溜め - 止まる（緊張感）
        armRotY = 0.8f;
        armRotZ = 0.3f;
        knifeRotX = -0.5f;
    }
    else if (t < 0.5f)
    {
        // Phase 3: 突き刺し！ - 素早く左へ
        float phase = (t - 0.35f) / 0.15f;  // 0→1
        phase = 1.0f - (1.0f - phase) * (1.0f - phase);  // イージング（減速）

        armRotY = 0.8f - phase * 1.6f;  // 右から左へ振る（0.8 → -0.8）
        armRotZ = 0.3f - phase * 0.5f;  // 傾きを戻しながら
        knifeRotX = -0.5f + phase * 0.3f;
    }
    else if (t < 0.75f)
    {
        // Phase 4: 停止 - 刺さった状態で止まる
        armRotY = -0.8f;
        armRotZ = -0.2f;
        knifeRotX = -0.2f;
    }
    else
    {
        // Phase 5: 引き抜き - 元に戻す
        float phase = (t - 0.75f) / 0.25f;  // 0→1
        phase = phase * phase;  // イージング

        armRotY = -0.8f * (1.0f - phase);
        armRotZ = -0.2f * (1.0f - phase);
        knifeRotX = -0.2f * (1.0f - phase);
    }

    // AnimRot に適用
    m_gloryKillArmAnimRot.y = armRotY;
    m_gloryKillArmAnimRot.z = armRotZ;
    m_gloryKillKnifeAnimRot.x = knifeRotX;
}


// ============================================================
//  PerformMeleeAttack - 近接攻撃の実行
//
//  【処理フロー】
//  1. チャージ消費チェック
//  2. プレイヤー正面の敵を検索（距離+角度判定）
//  3. ダメージ適用 + ヒットストップ
//  4. 弾薬回復（DOOM Eternal風：殴ると弾が出る）
//  5. スタガー状態の敵にはグローリーキル発動
//  6. 血エフェクト + スコアポップアップ
// ============================================================
void Game::PerformMeleeAttack()
{
    using namespace DirectX;

    //  プレイヤーの位置と向きを取得
    XMFLOAT3 playerPos = m_player->GetPosition();
    float cameraRotY = m_player->GetCameraRotationY();  //  Y軸回転(左右)

    //  前方ベクトルを計算
    // カメラの向きから前方ベクトルを作成
    XMFLOAT3 forward;
    forward.x = sinf(cameraRotY);   //  X方向成分
    forward.y = 0.0f;   //  Y方向は０（水平方向のみ）
    forward.z = cosf(cameraRotY);   //  Z方向成分

    //  レイキャストの開始位置と方向
    const float meleeRange = 2.0f;  //  近接攻撃の射程(2メートル)

    //  レイキャストを実行(Bullet Physic)
    RaycastResult result = RaycastPhysics(
        playerPos,  // 開始位置
        forward,    // 方向
        meleeRange  // 最大距離
    );

    //  敵に当たったか確認
    if (result.hit && result.hitEnemy)
    {
        Enemy* hitEnemy = result.hitEnemy;

        //  グローリー判定(敵がよろめいてる状態)
        if (hitEnemy->isStaggered)
        {
            //  グローリーキル
            //OutputDebugStringA("[GLORY KILL] GLORY KILL EXECUTED!\n");

            //  アニメーション開始
            m_gloryKillArmAnimActive = true;
            m_gloryKillArmAnimTime = 0.0f;

            //  即死させる
            hitEnemy->health = 0;

            //  弾薬を50%回復（上限あり）
            WeaponType currentWeapon = m_weaponSystem->GetCurrentWeapon();
            WeaponData weaponData = m_weaponSystem->GetWeaponData(currentWeapon);

            int currentAmmo = m_weaponSystem->GetReserveAmmo();
            int maxReserve = weaponData.reserveAmmo;
            int ammoReward = (int)(maxReserve * 0.5f);  // 50%回復
            int newAmmo = currentAmmo + ammoReward;
            if (newAmmo > maxReserve) newAmmo = maxReserve;  // 上限

            m_weaponSystem->SetReserveAmmo(newAmmo);

            /*char buffer[128];
            sprintf_s(buffer, "[GLORY KILL] Ammo recovered: +%d (Total: %d/%d)\n",
                ammoReward, newAmmo, maxReserve);*/
                //OutputDebugStringA(buffer);


                //  カメラシェイク
            m_cameraShake = 0.3f;   //  通常の２倍
            m_cameraShakeTimer = 0.3f;

            //  ヒットストップ
            m_hitStopTimer = 0.1f;  //  通常の２倍

            //  スローモーション
            m_timeScale = 0.2f; //  20%スロー
            m_slowMoTimer = 0.8f;   //  0.8秒間

            //  パーティクル
            if (m_particleSystem)
            {
                XMFLOAT3 bloodDir(0.0f, 1.0f, 0.0f);

                /*m_particleSystem->CreateBloodEffect(
                    hitEnemy->position,
                    bloodDir,
                    150
                );*/
            }
        }
        else
        {
            const float normalMeleeDamage = 25.0f;  // 小ダメージ

            int oldHealth = hitEnemy->health;
            hitEnemy->health -= (int)normalMeleeDamage;
            m_statDamageDealt += (int)normalMeleeDamage;  //  与ダメ記録

            // 弾補充（上限あり）
            int currentAmmo = m_weaponSystem->GetReserveAmmo();
            WeaponData wd = m_weaponSystem->GetWeaponData(m_weaponSystem->GetCurrentWeapon());
            int newAmmo = currentAmmo + m_meleeAmmoRefill;
            if (newAmmo > wd.reserveAmmo) newAmmo = wd.reserveAmmo;
            m_weaponSystem->SetReserveAmmo(newAmmo);
            /*char buffer[128];
            sprintf_s(buffer, "[MELEE] Normal hit! Damage:%.0f HP:%d->%d\n",
                normalMeleeDamage, oldHealth, hitEnemy->health);*/
                //OutputDebugStringA(buffer);

                // 軽いフィードバック
            m_cameraShake = 0.08f;
            m_cameraShakeTimer = 0.15f;

            m_hitStopTimer = 0.03f;

            // 少量のパーティクル
            if (m_particleSystem)
            {
                XMFLOAT3 bloodDir(0.0f, 1.0f, 0.0f);

                //m_particleSystem->CreateBloodEffect(
                //    hitEnemy->position,
                //    bloodDir,
                //    30  // 少量
                //);
            }
        }
    }
    else
    {
        //  空振り
        //OutputDebugStringA("[MELEE] Missed!\n");

        m_cameraShake = 0.05f;
        m_cameraShakeTimer = 0.1f;
    }

}

// =================================================================
// CreateBlurResources - ガウシアンブラー用のリソースを作成
// =================================================================

// ============================================================
//  SpawnHealthPickup - ヘルスピックアップの生成
// ============================================================
void Game::SpawnHealthPickup()
{
    int activeCount = 0;
    for (auto& p : m_healthPickups)
        if (p.isActive) activeCount++;
    if (activeCount >= m_maxPickups) return;

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    HealthPickup pickup;

    float angle = ((float)rand() / RAND_MAX) * 6.2832f;
    float dist = 8.0f + ((float)rand() / RAND_MAX) * 17.0f;

    pickup.position.x = playerPos.x + cosf(angle) * dist;
    pickup.position.y = GetMeshFloorHeight(pickup.position.x, pickup.position.z, 0.0f) + 0.5f;
    pickup.position.z = playerPos.z + sinf(angle) * dist;

    //  マップ内にクランプ（歩行エリア内のみ）
    pickup.position.x = (std::max)(-31.0f, (std::min)(42.0f, pickup.position.x));
    pickup.position.z = (std::max)(-8.0f, (std::min)(22.0f, pickup.position.z));

    pickup.healAmount = 20 + (rand() % 3) * 5;
    pickup.lifetime = 30.0f;
    pickup.maxLifetime = 30.0f;
    pickup.bobTimer = ((float)rand() / RAND_MAX) * 6.28f;
    pickup.spinAngle = 0.0f;
    pickup.isActive = true;

    m_healthPickups.push_back(pickup);
}

// ============================================================
//  回復アイテムの更新（寿命・拾い判定・演出）
// ============================================================

// ============================================================
//  UpdateHealthPickups - ヘルスピックアップの更新
// ============================================================
void Game::UpdateHealthPickups(float deltaTime)
{
    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();

    for (auto& pickup : m_healthPickups)
    {
        if (!pickup.isActive) continue;

        // 寿命カウントダウン
        pickup.lifetime -= deltaTime;
        if (pickup.lifetime <= 0.0f)
        {
            pickup.isActive = false;
            continue;
        }

        // ボブ＆回転演出
        pickup.bobTimer += deltaTime * 3.0f;
        pickup.spinAngle += deltaTime * 2.0f;

        // === 拾い判定 ===
        float dx = playerPos.x - pickup.position.x;
        float dz = playerPos.z - pickup.position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < m_pickupRadius * m_pickupRadius)
        {
            int currentHP = m_player->GetHealth();
            int maxHP = m_player->GetMaxHealth();

            // 最大HPなら拾わない（もったいないから残す）
            if (currentHP < maxHP)
            {
                int newHP = (std::min)(currentHP + pickup.healAmount, maxHP);
                m_player->SetHealth(newHP);
                pickup.isActive = false;

                // 拾った演出：軽いカメラシェイク（心地よい）
                m_cameraShake = 0.02f;
                m_cameraShakeTimer = 0.1f;
            }
        }
    }

    // 死んだアイテムを削除
    m_healthPickups.erase(
        std::remove_if(m_healthPickups.begin(), m_healthPickups.end(),
            [](const HealthPickup& p) { return !p.isActive; }),
        m_healthPickups.end());

    // 定期スポーン
    m_pickupSpawnTimer += deltaTime;
    if (m_pickupSpawnTimer >= m_pickupSpawnInterval)
    {
        m_pickupSpawnTimer = 0.0f;
        SpawnHealthPickup();
    }
}

// ============================================================
//  回復アイテム描画（緑の十字ビルボード）
// ============================================================

// ============================================================
//  SpawnScorePopup - スコアポップアップの生成
// ============================================================
void Game::SpawnScorePopup(int points, ScorePopupType type)
{
    ScorePopup popup;
    float cx = m_outputWidth * 0.5f + 120.0f;
    float cy = m_outputHeight * 0.5f - 60.0f;

    // タイプごとに出現位置を少しバラす（画面中央付近）
    float randX = ((float)rand() / RAND_MAX - 0.5f) * 60.0f;
    float randY = ((float)rand() / RAND_MAX - 0.5f) * 30.0f;

    switch (type)
    {
    case ScorePopupType::HEADSHOT:
        popup.screenX = cx + randX;
        popup.screenY = cy - 40.0f + randY;   // 少し上
        popup.maxTime = 1.6f;
        break;
    case ScorePopupType::GLORY_KILL:
        popup.screenX = cx + randX;
        popup.screenY = cy - 30.0f + randY;
        popup.maxTime = 2.0f;                  // 長めに表示
        break;
    case ScorePopupType::WAVE_BONUS:
        popup.screenX = cx;
        popup.screenY = cy - 80.0f;            // 上の方
        popup.maxTime = 2.0f;
        break;
    default:
        popup.screenX = cx + randX;
        popup.screenY = cy - 20.0f + randY;
        popup.maxTime = 1.2f;
        break;
    }

    popup.points = points;
    popup.type = type;
    popup.timer = 0.0f;
    popup.scale = 1.0f;
    popup.alpha = 1.0f;
    popup.offsetY = 0.0f;
    popup.burstAngle = ((float)rand() / RAND_MAX) * 6.28f;
    popup.isActive = true;

    m_scorePopups.push_back(popup);
}


// ============================================================
//  UpdateScorePopups - スコアポップアップの更新
// ============================================================
void Game::UpdateScorePopups(float deltaTime)
{
    for (auto& popup : m_scorePopups)
    {
        if (!popup.isActive) continue;

        popup.timer += deltaTime;
        float t = popup.timer / popup.maxTime;  // 0.0?1.0

        if (t >= 1.0f)
        {
            popup.isActive = false;
            continue;
        }

        // === アニメーション ===

        // Phase 1 (0?0.1): パンチイン（大きく出て縮む）
        if (t < 0.1f)
        {
            float p = t / 0.1f;
            popup.scale = 2.0f - p * 1.0f;    // 2.0 → 1.0
            popup.alpha = 1.0f;
        }
        // Phase 2 (0.1?0.7): 安定表示＋ゆっくり上昇
        else if (t < 0.7f)
        {
            popup.scale = 1.0f;
            popup.alpha = 1.0f;
        }
        // Phase 3 (0.7?1.0): フェードアウト＋加速上昇
        else
        {
            float fadeT = (t - 0.7f) / 0.3f;   // 0→1
            popup.scale = 1.0f - fadeT * 0.3f;  // 少し縮む
            popup.alpha = 1.0f - fadeT;          // 透明に
        }

        // 上昇（イージング付き）
        popup.offsetY = t * t * 120.0f;

        // バースト回転
        popup.burstAngle += deltaTime * 1.5f;
    }

    // 消えたポップアップを削除
    m_scorePopups.erase(
        std::remove_if(m_scorePopups.begin(), m_scorePopups.end(),
            [](const ScorePopup& p) { return !p.isActive; }),
        m_scorePopups.end());
}


// ============================================================
//  DrawScorePopups - スコアポップアップの描画
// ============================================================
void Game::DrawScorePopups()
{
    if (m_scorePopups.empty() || !m_spriteBatch || !m_fontLarge) return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied());

    for (auto& popup : m_scorePopups)
    {
        if (!popup.isActive) continue;

        float x = popup.screenX;
        float y = popup.screenY - popup.offsetY;
        float s = popup.scale;
        float a = popup.alpha;

        // === グロウ/バースト背景 ===
        ID3D11ShaderResourceView* glowTex = nullptr;
        DirectX::XMVECTORF32 glowColor = { 1, 1, 1, a * 0.6f };

        switch (popup.type)
        {
        case ScorePopupType::HEADSHOT:
            // バースト + グロウ両方
            if (m_scoreBurst.Get())
            {
                float burstSize = 160.0f * s;
                RECT burstRect = {
                    (LONG)(x - burstSize * 0.5f), (LONG)(y - burstSize * 0.5f),
                    (LONG)(x + burstSize * 0.5f), (LONG)(y + burstSize * 0.5f)
                };
                DirectX::XMVECTORF32 burstColor = { 1, 0.9f, 0.3f, a * 0.5f };
                m_spriteBatch->Draw(m_scoreBurst.Get(), burstRect, nullptr,
                    burstColor, popup.burstAngle,
                    DirectX::XMFLOAT2(64, 64));
            }
            glowTex = m_scoreGlow.Get();
            glowColor = { 1, 0.85f, 0.2f, a * 0.7f };
            break;

        case ScorePopupType::GLORY_KILL:
            glowTex = m_scoreGlowRed.Get();
            glowColor = { 1, 0.3f, 0.2f, a * 0.8f };
            break;

        case ScorePopupType::WAVE_BONUS:
            glowTex = m_scoreGlowBlue.Get();
            glowColor = { 0.7f, 0.5f, 1, a * 0.6f };
            break;

        case ScorePopupType::PARRY:
            glowTex = m_scoreGlowBlue.Get();
            glowColor = { 0.2f, 1.0f, 0.4f, a * 0.6f };
            break;

        default:
            glowTex = m_scoreGlow.Get();
            glowColor = { 1, 0.9f, 0.4f, a * 0.5f };
            break;
        }

        // グロウ描画
        if (glowTex)
        {
            float glowSize = 120.0f * s;
            RECT glowRect = {
                (LONG)(x - glowSize * 0.5f), (LONG)(y - glowSize * 0.5f),
                (LONG)(x + glowSize * 0.5f), (LONG)(y + glowSize * 0.5f)
            };
            m_spriteBatch->Draw(glowTex, glowRect, glowColor);
        }

        // === アイコン（ヘッドショット/グローリーキル） ===
        ID3D11ShaderResourceView* iconTex = nullptr;
        if (popup.type == ScorePopupType::HEADSHOT)
            iconTex = m_scoreHeadshot.Get();
        else if (popup.type == ScorePopupType::GLORY_KILL)
            iconTex = m_scoreSkull.Get();

        if (iconTex)
        {
            float iconSize = 32.0f * s;
            RECT iconRect = {
                (LONG)(x - 70.0f * s - iconSize), (LONG)(y - iconSize * 0.5f),
                (LONG)(x - 70.0f * s),             (LONG)(y + iconSize * 0.5f)
            };
            DirectX::XMVECTORF32 iconColor = { 1, 1, 1, a };
            m_spriteBatch->Draw(iconTex, iconRect, iconColor);
        }

        // === スコアテキスト ===
        wchar_t buf[32];
        swprintf_s(buf, L"+%d", popup.points);

        // テキストサイズ測定（中央揃え用）
        DirectX::XMVECTOR textSize = m_fontLarge->MeasureString(buf);
        float tw = DirectX::XMVectorGetX(textSize);
        float th = DirectX::XMVectorGetY(textSize);

        DirectX::XMFLOAT2 textPos(x - tw * 0.5f * s, y - th * 0.5f * s);

        // テキスト色（タイプ別）
        DirectX::XMVECTORF32 textColor;
        DirectX::XMVECTORF32 shadowColor = { 0, 0, 0, a * 0.8f };

        switch (popup.type)
        {
        case ScorePopupType::HEADSHOT:
            textColor = { 1.0f, 0.85f, 0.1f, a };   // 金色
            break;
        case ScorePopupType::GLORY_KILL:
            textColor = { 1.0f, 0.2f, 0.15f, a };    // 赤
            break;
        case ScorePopupType::WAVE_BONUS:
            textColor = { 0.7f, 0.5f, 1.0f, a };     // 紫
            break;
        case ScorePopupType::SHIELD_KILL:
            textColor = { 0.3f, 0.8f, 1.0f, a };     // 水色
            break;
        case ScorePopupType::PARRY:
            textColor = { 0.2f, 1.0f, 0.4f, a };
        default:
            textColor = { 1.0f, 1.0f, 1.0f, a };     // 白
            break;
        }

        // 影（2px オフセット）
        DirectX::XMFLOAT2 shadowPos(textPos.x + 2.0f, textPos.y + 2.0f);
        m_fontLarge->DrawString(m_spriteBatch.get(), buf, shadowPos,
            shadowColor, 0.0f, DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2(s, s));

        // 本体テキスト
        m_fontLarge->DrawString(m_spriteBatch.get(), buf, textPos,
            textColor, 0.0f, DirectX::XMFLOAT2(0, 0),
            DirectX::XMFLOAT2(s, s));

        // === ラベルテキスト（HEADSHOT! / GLORY KILL! 等） ===
        const wchar_t* label = nullptr;
        switch (popup.type)
        {
        case ScorePopupType::HEADSHOT:    label = L"HEADSHOT";    break;
        case ScorePopupType::GLORY_KILL:  label = L"GLORY KILL";  break;
        case ScorePopupType::WAVE_BONUS:  label = L"WAVE CLEAR";  break;
        case ScorePopupType::SHIELD_KILL: label = L"SHIELD KILL"; break;
        case ScorePopupType::PARRY:       label = L"PARRY";       break;
        default: break;
        }

        if (label && m_font)
        {
            DirectX::XMVECTOR labelSize = m_font->MeasureString(label);
            float lw = DirectX::XMVectorGetX(labelSize);
            DirectX::XMFLOAT2 labelPos(x - lw * 0.5f, y + th * 0.5f * s + 4.0f);

            m_font->DrawString(m_spriteBatch.get(), label, labelPos,
                textColor, 0.0f, DirectX::XMFLOAT2(0, 0),
                DirectX::XMFLOAT2(0.9f * s, 0.9f * s));
        }
    }

    m_spriteBatch->End();
}