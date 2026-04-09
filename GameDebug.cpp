// ============================================================
//  GameDebug.cpp
//  �f�o�b�OUI�E�q�b�g�{�b�N�X�����E�e���g���[�T�[
//
//  �y�Ӗ��z
//  - ImGui�̏�����/�j��/���t���[���`��
//  - �G�q�b�g�{�b�N�X�̃��C���[�t���[���`��
//  - Bullet Physics�J�v�Z���̉���
//  - �e���g���[�T�[�i�r���{�[�h�N�A�b�h�j�̕`��
//
//  �y�݌v�Ӑ}�z
//  �f�o�b�O�@�\�̓����[�X�r���h�ł͕s�v�B
//  �������Ă������ƂŁA��o����#ifdef DEBUG��
//  �ۂ��Ə��O����I����������B
//
//  �y��o���̒��Ӂz
//  �Z�K��o���� m_showDebugWindow=false ���f�t�H���g�B
//  F1�L�[�ŊJ�����Ƃ� README �ɖ��L����B
// ============================================================

#include "Game.h"

using namespace DirectX;

// ============================================================
//  �w���p�[: �f�o�b�O/�f�t�H���g�̃q�b�g�{�b�N�X�ݒ���擾
//
//  �y�Ȃ��w���p�[���H�z
//  DrawHitboxes, DrawCapsule�\��, AddEnemyPhysicsBody �Ȃ�
//  �����ӏ��œ���switch�����J��Ԃ���Ă����iDRY�����ᔽ�j�B
//  1�ӏ��ɂ܂Ƃ߂邱�ƂŃq�b�g�{�b�N�X�ݒ�̕ύX�����S�ɂȂ�B
// ============================================================
EnemyTypeConfig Game_GetDebugConfig(
    EnemyType type,
    bool useDebug,
    const EnemyTypeConfig& normalDbg,
    const EnemyTypeConfig& runnerDbg,
    const EnemyTypeConfig& tankDbg,
    const EnemyTypeConfig& midbossDbg,
    const EnemyTypeConfig& bossDbg)
{
    if (!useDebug)
        return GetEnemyConfig(type);

    switch (type)
    {
    case EnemyType::NORMAL:  return normalDbg;
    case EnemyType::RUNNER:  return runnerDbg;
    case EnemyType::TANK:    return tankDbg;
    case EnemyType::MIDBOSS: return midbossDbg;
    case EnemyType::BOSS:    return bossDbg;
    default:                 return GetEnemyConfig(type);
    }
}

// ============================================================
//  InitImGui / ShutdownImGui
//
//  �y�Ăяo���zInitialize() / �f�X�g���N�^ ��1�񂸂�
//  �y�ˑ��zm_window, m_d3dDevice, m_d3dContext ���L���ł��邱��
// ============================================================
void Game::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // �_�[�N�e�[�}�i�S�V�b�N�ȕ��͋C�ɍ����j
    ImGui::StyleColorsDark();

    // Win32 + DX11 �o�b�N�G���h������
    ImGui_ImplWin32_Init(m_window);
    ImGui_ImplDX11_Init(m_d3dDevice.Get(), m_d3dContext.Get());
}

void Game::ShutdownImGui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// ============================================================
//  DrawDebugUI - ImGui�f�o�b�O�E�B���h�E�̕`��
//
//  �y�@�\�ꗗ�z
//  - FPS / Wave / �G���̕\��
//  - �}�b�v���C�e�B���O�����i�v���Z�b�g3��j
//  - �v���C���[���i�ʒu�E��]�EHP�j
//  - �O���[���[�L�����f���ʒu����
//  - �q�b�g�{�b�N�X���A���^�C������ + �N���b�v�{�[�h�o��
//  - �G�U���^�C�~���O�����i�^�C�����C���o�[�t���j
//  - �{�XAI�t�F�[�Y�Ď�
//  - �p���B�E�B���h�E����
//  - �ߐڃ`���[�W�V�X�e������
//
//  �y���Ӂz���̊֐���1,100�s����A�{���͂���ɕ������ׂ��B
//  ������ImGui��Begin/End�\����A�_���I��1�֐��ɂ܂Ƃ߂�̂����R�B
//  �����I�ɂ̓^�u���ƂɃT�u�֐�������������B
// ============================================================
void Game::DrawDebugUI()
{
    // ImGui�V�t���[���J�n
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // === �f�o�b�O�E�B���h�E ===
    if (m_showDebugWindow)
    {
        ImGui::Begin("Gothic Swarm Debug", &m_showDebugWindow, ImGuiWindowFlags_AlwaysAutoResize);

        // --- ��{��� ---
        ImGui::Text("FPS: %.1f", m_currentFPS);
        ImGui::Separator();

        ImGui::Text("Wave: %d", m_waveManager->GetCurrentWave());

        // �^�C�v�ʓG�J�E���g
        int totalEnemies = 0, normalCount = 0, runnerCount = 0, tankCount = 0;
        for (const auto& enemy : m_enemySystem->GetEnemies())
        {
            if (!enemy.isAlive && !enemy.isDying) continue;
            totalEnemies++;
            switch (enemy.type)
            {
            case EnemyType::NORMAL: normalCount++; break;
            case EnemyType::RUNNER: runnerCount++; break;
            case EnemyType::TANK:   tankCount++;   break;
            }
        }

        ImGui::Checkbox("Draw NavGrid", &m_debugDrawNavGrid);

        // === �}�b�v���C�e�B���O���� ===
        if (m_mapSystem && ImGui::CollapsingHeader("Map Lighting"))
        {
            ImGui::Text("--- Main Light ---");
            ImGui::SliderFloat3("Direction 0", &m_mapSystem->m_lightDir0.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color 0", &m_mapSystem->m_lightColor0.x);

            ImGui::Text("--- Fill Light ---");
            ImGui::SliderFloat3("Direction 1", &m_mapSystem->m_lightDir1.x, -1.0f, 1.0f);
            ImGui::ColorEdit3("Color 1", &m_mapSystem->m_lightColor1.x);

            ImGui::Text("--- Ambient ---");
            ImGui::ColorEdit3("Ambient", &m_mapSystem->m_ambientColor.x);

            // �v���Z�b�g�{�^��
            if (ImGui::Button("Preset: Gothic Dark"))
            {
                m_mapSystem->m_lightDir0 = { 0.0f, -1.0f, 0.3f };
                m_mapSystem->m_lightColor0 = { 0.6f, 0.45f, 0.3f, 1.0f };
                m_mapSystem->m_lightDir1 = { -0.5f, -0.3f, -0.7f };
                m_mapSystem->m_lightColor1 = { 0.15f, 0.15f, 0.25f, 1.0f };
                m_mapSystem->m_ambientColor = { 0.08f, 0.06f, 0.05f, 1.0f };
            }
            ImGui::SameLine();
            if (ImGui::Button("Preset: Bright"))
            {
                m_mapSystem->m_lightDir0 = { 0.3f, -0.8f, 0.5f };
                m_mapSystem->m_lightColor0 = { 1.0f, 0.95f, 0.9f, 1.0f };
                m_mapSystem->m_lightDir1 = { -0.5f, -0.3f, -0.7f };
                m_mapSystem->m_lightColor1 = { 0.4f, 0.4f, 0.5f, 1.0f };
                m_mapSystem->m_ambientColor = { 0.25f, 0.25f, 0.3f, 1.0f };
            }
            ImGui::SameLine();
            if (ImGui::Button("Preset: Blood Moon"))
            {
                m_mapSystem->m_lightDir0 = { 0.2f, -0.6f, 0.4f };
                m_mapSystem->m_lightColor0 = { 0.8f, 0.2f, 0.1f, 1.0f };
                m_mapSystem->m_lightDir1 = { -0.5f, -0.3f, -0.7f };
                m_mapSystem->m_lightColor1 = { 0.1f, 0.05f, 0.15f, 1.0f };
                m_mapSystem->m_ambientColor = { 0.1f, 0.02f, 0.02f, 1.0f };
            }
        }

        ImGui::Text("Total Enemies: %d", totalEnemies);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  Normal: %d", normalCount);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Runner: %d", runnerCount);
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "  Tank: %d", tankCount);
        ImGui::Separator();

        // --- �v���C���[��� ---
        ImGui::Text("Health: %d", m_player->GetHealth());
        ImGui::Text("Points: %d", m_player->GetPoints());

        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();
        ImGui::Text("Player Pos: (%.2f, %.2f, %.2f)", playerPos.x, playerPos.y, playerPos.z);
        ImGui::Text("Ray Start Y: %.2f", playerPos.y + 0.5f);
        ImGui::Text("Camera Rot: (%.2f, %.2f)",
            DirectX::XMConvertToDegrees(playerRot.x),
            DirectX::XMConvertToDegrees(playerRot.y));
        ImGui::Separator();

        // --- �O���[���[�L�����f������ ---
        if (ImGui::CollapsingHeader("Glory Kill Model"))
        {
            ImGui::Checkbox("Always Show Arm (Debug)", &m_debugShowGloryKillArm);
            ImGui::Separator();

            ImGui::Text("=== ARM ===");
            ImGui::SliderFloat("Arm Scale", &m_gloryKillArmScale, 0.001f, 0.1f);
            ImGui::SliderFloat("Arm Forward", &m_gloryKillArmOffset.x, 0.0f, 2.0f);
            ImGui::SliderFloat("Arm Up/Down", &m_gloryKillArmOffset.y, -1.0f, 1.0f);
            ImGui::SliderFloat("Arm Left/Right", &m_gloryKillArmOffset.z, -1.0f, 1.0f);

            ImGui::Text("--- Base Rotation (Fix Model) ---");
            ImGui::SliderFloat("Arm BaseRotX", &m_gloryKillArmBaseRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm BaseRotY", &m_gloryKillArmBaseRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm BaseRotZ", &m_gloryKillArmBaseRot.z, -3.14f, 3.14f);

            ImGui::Text("--- Anim Rotation (For Motion) ---");
            ImGui::SliderFloat("Arm AnimRotX", &m_gloryKillArmAnimRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm AnimRotY", &m_gloryKillArmAnimRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Arm AnimRotZ", &m_gloryKillArmAnimRot.z, -3.14f, 3.14f);

            ImGui::Separator();
            ImGui::Text("=== KNIFE ===");
            ImGui::SliderFloat("Knife Scale", &m_gloryKillKnifeScale, 0.001f, 0.1f);
            ImGui::SliderFloat("Knife Forward", &m_gloryKillKnifeOffset.x, -0.5f, 0.5f);
            ImGui::SliderFloat("Knife Up/Down", &m_gloryKillKnifeOffset.y, -0.5f, 0.5f);
            ImGui::SliderFloat("Knife Left/Right", &m_gloryKillKnifeOffset.z, -0.5f, 0.5f);

            ImGui::Text("--- Base Rotation ---");
            ImGui::SliderFloat("Knife BaseRotX", &m_gloryKillKnifeBaseRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife BaseRotY", &m_gloryKillKnifeBaseRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife BaseRotZ", &m_gloryKillKnifeBaseRot.z, -3.14f, 3.14f);

            ImGui::Text("--- Anim Rotation ---");
            ImGui::SliderFloat("Knife AnimRotX", &m_gloryKillKnifeAnimRot.x, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife AnimRotY", &m_gloryKillKnifeAnimRot.y, -3.14f, 3.14f);
            ImGui::SliderFloat("Knife AnimRotZ", &m_gloryKillKnifeAnimRot.z, -3.14f, 3.14f);
        }

        // --- �\���g�O�� ---
        ImGui::Text("Visual Debug:");
        ImGui::Checkbox("Show Body Hitboxes", &m_showHitboxes);
        ImGui::Checkbox("Show Head Hitboxes", &m_showHeadHitboxes);
        ImGui::Checkbox("Show Physics Capsules", &m_showPhysicsHitboxes);
        ImGui::Checkbox("Show Bullet Trajectory", &m_showBulletTrajectory);
        ImGui::SliderFloat("Ray Start Y", &m_rayStartY, 0.5f, 3.0f, "%.2f");

        if (ImGui::TreeNode("Weapon Transform"))
        {
            ImGui::SliderFloat("Scale", &m_weaponScale, 0.001f, 0.5f, "%.4f");
            ImGui::SliderFloat("Rot X", &m_weaponRotX, -3.14f, 3.14f, "%.2f");
            ImGui::SliderFloat("Rot Y", &m_weaponRotY, -3.14f, 3.14f, "%.2f");
            ImGui::SliderFloat("Rot Z", &m_weaponRotZ, -3.14f, 3.14f, "%.2f");
            ImGui::Separator();
            ImGui::SliderFloat("Right", &m_weaponOffsetRight, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Up", &m_weaponOffsetUp, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Forward", &m_weaponOffsetForward, 0.0f, 2.0f, "%.2f");
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Weapon Bob"))
        {
            ImGui::SliderFloat("Speed", &m_weaponBobSpeed, 5.0f, 20.0f, "%.1f");
            ImGui::SliderFloat("Strength", &m_weaponBobStrength, 0.005f, 0.06f, "%.3f");
            ImGui::Text("Bob: %.4f  Impact: %.4f", m_weaponBobAmount, m_weaponLandingImpact);
            ImGui::TreePop();
        }

        // �}��
        if (m_showHitboxes || m_showHeadHitboxes || m_showPhysicsHitboxes)
        {
            ImGui::Separator();
            ImGui::Text("Color Legend:");
            if (m_showHitboxes)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  Red = Normal Body");
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Yellow = Runner Body");
                ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "  Blue = Tank Body");
            }
            if (m_showPhysicsHitboxes)
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "  Green = Physics Capsule");
            if (m_showHeadHitboxes)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "  Magenta = Head");
        }
        ImGui::Separator();

        // === �q�b�g�{�b�N�X�����^�u ===
        if (ImGui::CollapsingHeader("Hitbox Tuning", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Use Debug Hitboxes (Real-time)", &m_useDebugHitboxes);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                m_useDebugHitboxes ? "Using DEBUG values" : "Using Entities.h values");
            ImGui::Separator();

            // �e�^�C�v�̃q�b�g�{�b�N�X�����i�}�N���I�ɌJ��Ԃ��\���j
            struct HitboxTuneInfo {
                const char* label;
                EnemyType type;
                EnemyTypeConfig* config;
                float maxBody;
                float maxHead;
            };

            HitboxTuneInfo tuneTargets[] = {
                { "NORMAL Hitbox",  EnemyType::NORMAL,  &m_normalConfigDebug,  2.0f, 1.0f },
                { "RUNNER Hitbox",  EnemyType::RUNNER,  &m_runnerConfigDebug,  2.0f, 1.0f },
                { "TANK Hitbox",    EnemyType::TANK,    &m_tankConfigDebug,    2.0f, 1.0f },
                { "MIDBOSS Hitbox", EnemyType::MIDBOSS, &m_midbossConfigDebug, 3.0f, 2.0f },
                { "BOSS Hitbox",    EnemyType::BOSS,    &m_bossConfigDebug,    4.0f, 3.0f },
            };

            for (auto& info : tuneTargets)
            {
                if (ImGui::TreeNode(info.label))
                {
                    ImGui::Text("Body:");
                    ImGui::SliderFloat(("Body Width##" + std::string(info.label)).c_str(),
                        &info.config->bodyWidth, 0.1f, info.maxBody);
                    ImGui::SliderFloat(("Body Height##" + std::string(info.label)).c_str(),
                        &info.config->bodyHeight, 0.5f, info.maxBody * 2.5f);
                    ImGui::Text("Head:");
                    ImGui::SliderFloat(("Head Height##" + std::string(info.label)).c_str(),
                        &info.config->headHeight, 0.5f, info.maxBody * 2.5f);
                    ImGui::SliderFloat(("Head Radius##" + std::string(info.label)).c_str(),
                        &info.config->headRadius, 0.1f, info.maxHead);

                    ImGui::Text("  Body: %.2f x %.2f", info.config->bodyWidth, info.config->bodyHeight);
                    ImGui::Text("  Head: Y=%.2f, R=%.2f", info.config->headHeight, info.config->headRadius);

                    if (ImGui::Button(("Copy to Clipboard##" + std::string(info.label)).c_str()))
                    {
                        char buffer[512];
                        sprintf_s(buffer,
                            "config.bodyWidth = %.2ff;\n"
                            "config.bodyHeight = %.2ff;\n"
                            "config.headHeight = %.2ff;\n"
                            "config.headRadius = %.2ff;",
                            info.config->bodyWidth, info.config->bodyHeight,
                            info.config->headHeight, info.config->headRadius);
                        ImGui::SetClipboardText(buffer);
                    }
                    ImGui::TreePop();
                }
            }

            if (ImGui::Button("Apply Hitbox to Physics"))
            {
                for (auto& enemy : m_enemySystem->GetEnemies())
                {
                    if (!enemy.isAlive) continue;
                    RemoveEnemyPhysicsBody(enemy.id);
                    AddEnemyPhysicsBody(enemy);
                }
            }

            if (ImGui::Button("Reset All to Default"))
            {
                m_normalConfigDebug = GetEnemyConfig(EnemyType::NORMAL);
                m_runnerConfigDebug = GetEnemyConfig(EnemyType::RUNNER);
                m_tankConfigDebug = GetEnemyConfig(EnemyType::TANK);
                m_midbossConfigDebug = GetEnemyConfig(EnemyType::MIDBOSS);
                m_bossConfigDebug = GetEnemyConfig(EnemyType::BOSS);
            }

            ImGui::Separator();
            ImGui::TextWrapped("Tip: Adjust values while looking at hitboxes, then click 'Copy to Clipboard' and paste into Entities.h");
        }

        // === �G�U���^�C�~���O ===
        if (ImGui::TreeNode("Enemy Attack Timing"))
        {
            ImGui::Text("--- NORMAL ---");
            ImGui::SliderFloat("Hit Time##normal", &m_enemySystem->m_normalAttackHitTime, 0.1f, 10.0f, "%.2f");
            ImGui::SliderFloat("Duration##normal", &m_enemySystem->m_normalAttackDuration, 0.5f, 10.0f, "%.2f");

            ImGui::Text("--- RUNNER ---");
            ImGui::SliderFloat("Hit Time##runner", &m_enemySystem->m_runnerAttackHitTime, 0.05f, 10.0f, "%.2f");
            ImGui::SliderFloat("Duration##runner", &m_enemySystem->m_runnerAttackDuration, 0.3f, 10.0f, "%.2f");

            ImGui::Text("--- TANK ---");
            ImGui::SliderFloat("Hit Time##tank", &m_enemySystem->m_tankAttackHitTime, 0.3f, 10.0f, "%.2f");
            ImGui::SliderFloat("Duration##tank", &m_enemySystem->m_tankAttackDuration, 1.0f, 10.0f, "%.2f");

            ImGui::Separator();
            if (ImGui::TreeNode("Animation Speed"))
            {
                const char* typeNames[] = { "NORMAL", "RUNNER", "TANK", "MIDBOSS", "BOSS" };
                const char* animNames[] = { "Idle", "Walk", "Run", "Attack", "Death" };
                float* animArrays[] = {
                    m_enemySystem->m_animSpeed_Idle,
                    m_enemySystem->m_animSpeed_Walk,
                    m_enemySystem->m_animSpeed_Run,
                    m_enemySystem->m_animSpeed_Attack,
                    m_enemySystem->m_animSpeed_Death
                };

                constexpr int ENEMY_TYPE_COUNT = 5;
                for (int a = 0; a < 5; a++)
                {
                    if (ImGui::TreeNode(animNames[a]))
                    {
                        for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
                            ImGui::SliderFloat(typeNames[i], &animArrays[a][i], 0.01f, 5.0f, "%.2f");
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::Text("=== FBX Animation Durations (raw) ===");
            if (m_enemyModel)
            {
                ImGui::Text("NORMAL - Walk:%.2f  Attack:%.2f  Death:%.2f  Idle:%.2f  Run:%.2f",
                    m_enemyModel->GetAnimationDuration("Walk"),
                    m_enemyModel->GetAnimationDuration("Attack"),
                    m_enemyModel->GetAnimationDuration("Death"),
                    m_enemyModel->GetAnimationDuration("Idle"),
                    m_enemyModel->GetAnimationDuration("Run"));
            }
            if (m_runnerModel)
            {
                ImGui::Text("RUNNER - Walk:%.2f  Attack:%.2f  Death:%.2f  Idle:%.2f  Run:%.2f",
                    m_runnerModel->GetAnimationDuration("Walk"),
                    m_runnerModel->GetAnimationDuration("Attack"),
                    m_runnerModel->GetAnimationDuration("Death"),
                    m_runnerModel->GetAnimationDuration("Idle"),
                    m_runnerModel->GetAnimationDuration("Run"));
            }
            if (m_tankModel)
            {
                ImGui::Text("TANK   - Walk:%.2f  Attack:%.2f  Death:%.2f  Idle:%.2f  Run:%.2f",
                    m_tankModel->GetAnimationDuration("Walk"),
                    m_tankModel->GetAnimationDuration("Attack"),
                    m_tankModel->GetAnimationDuration("Death"),
                    m_tankModel->GetAnimationDuration("Idle"),
                    m_tankModel->GetAnimationDuration("Run"));
            }
            if (m_midBossModel)
            {
                ImGui::Text("MIDBOSS- Walk:%.2f  Attack:%.2f  Death:%.2f",
                    m_midBossModel->GetAnimationDuration("Walk"),
                    m_midBossModel->GetAnimationDuration("Attack"),
                    m_midBossModel->GetAnimationDuration("Death"));
            }
            if (m_bossModel)
            {
                ImGui::Text("BOSS   - Walk:%.2f  AtkJump:%.2f  AtkSlash:%.2f  Death:%.2f",
                    m_bossModel->GetAnimationDuration("Walk"),
                    m_bossModel->GetAnimationDuration("AttackJump"),
                    m_bossModel->GetAnimationDuration("AttackSlash"),
                    m_bossModel->GetAnimationDuration("Death"));
            }

            ImGui::Separator();
            ImGui::Text("=== Debug Spawn ===");

            if (ImGui::Button("Clear All Enemies"))
            {
                for (auto& e : m_enemySystem->GetEnemies())
                    RemoveEnemyPhysicsBody(e.id);
                m_enemySystem->GetEnemies().clear();
                m_waveManager->SetPaused(true);
            }
            ImGui::SameLine();
            if (ImGui::Button("Resume Waves"))
                m_waveManager->SetPaused(false);

            ImGui::SameLine();
            DirectX::XMFLOAT3 pPos = m_player->GetPosition();
            if (ImGui::Button("+ NORMAL"))
                m_enemySystem->SpawnEnemyOfType(EnemyType::NORMAL, pPos);
            ImGui::SameLine();
            if (ImGui::Button("+ RUNNER"))
                m_enemySystem->SpawnEnemyOfType(EnemyType::RUNNER, pPos);
            ImGui::SameLine();
            if (ImGui::Button("+ TANK"))
                m_enemySystem->SpawnEnemyOfType(EnemyType::TANK, pPos);

            ImGui::Separator();

            // �U�����̓G�̃^�C�����C���o�[�\��
            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive || enemy.isDying) continue;
                if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS) continue;
                if (enemy.currentAnimation != "Attack") continue;

                float hitTime, duration;
                const char* typeName;
                ImVec4 typeColor;
                switch (enemy.type)
                {
                case EnemyType::RUNNER:
                    hitTime = m_enemySystem->m_runnerAttackHitTime;
                    duration = m_enemySystem->m_runnerAttackDuration;
                    typeName = "RUNNER"; typeColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    break;
                case EnemyType::TANK:
                    hitTime = m_enemySystem->m_tankAttackHitTime;
                    duration = m_enemySystem->m_tankAttackDuration;
                    typeName = "TANK"; typeColor = ImVec4(0.0f, 0.5f, 1.0f, 1.0f);
                    break;
                default:
                    hitTime = m_enemySystem->m_normalAttackHitTime;
                    duration = m_enemySystem->m_normalAttackDuration;
                    typeName = "NORMAL"; typeColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    break;
                }

                char label[64];
                sprintf_s(label, "[%s] ID:%d", typeName, enemy.id);
                ImGui::TextColored(typeColor, "%s", label);
                ImGui::Text("  --- Attack Timeline ---");

                // �^�C�����C���o�[�`��
                constexpr float HIT_DURATION = 0.05f;
                float recovery = duration - hitTime - HIT_DURATION;
                if (recovery < 0.01f) recovery = 0.01f;

                struct PhaseInfo { const char* name; float duration; ImVec4 color; bool isDamagePhase; };
                std::vector<PhaseInfo> phases = {
                    {"WINDUP",   hitTime,      ImVec4(0.4f, 0.4f, 0.4f, 1), false},
                    {"HIT!",     HIT_DURATION, ImVec4(1.0f, 0.0f, 0.0f, 1), true },
                    {"RECOVERY", recovery,     ImVec4(0.2f, 0.8f, 0.2f, 1), false},
                };

                float currentOffset = enemy.animationTime;
                float totalDuration = 0;
                for (auto& p : phases) totalDuration += p.duration;

                constexpr float BAR_WIDTH = 280.0f;
                constexpr float BAR_HEIGHT = 20.0f;
                ImVec2 barStart = ImGui::GetCursorScreenPos();
                barStart.x += 20.0f;
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                float xOffset = 0;
                for (auto& p : phases)
                {
                    float w = (p.duration / totalDuration) * BAR_WIDTH;
                    ImU32 col = ImGui::ColorConvertFloat4ToU32(p.color);
                    if (p.isDamagePhase)
                    {
                        float blink = sinf((float)ImGui::GetTime() * 10.0f) * 0.3f + 0.7f;
                        ImVec4 blinkColor = p.color; blinkColor.w = blink;
                        col = ImGui::ColorConvertFloat4ToU32(blinkColor);
                    }
                    drawList->AddRectFilled(
                        ImVec2(barStart.x + xOffset, barStart.y),
                        ImVec2(barStart.x + xOffset + w, barStart.y + BAR_HEIGHT), col);
                    if (w > 30)
                        drawList->AddText(ImVec2(barStart.x + xOffset + 2, barStart.y + 3),
                            IM_COL32(255, 255, 255, 220), p.name);
                    xOffset += w;
                }

                drawList->AddRect(barStart,
                    ImVec2(barStart.x + BAR_WIDTH, barStart.y + BAR_HEIGHT),
                    IM_COL32(200, 200, 200, 180));

                // �J�[�\���i���ݎ����}�[�J�[�j
                float markerX = barStart.x + (currentOffset / totalDuration) * BAR_WIDTH;
                markerX = (std::min)(markerX, barStart.x + BAR_WIDTH);
                markerX = (std::max)(markerX, barStart.x);
                drawList->AddLine(
                    ImVec2(markerX, barStart.y - 2),
                    ImVec2(markerX, barStart.y + BAR_HEIGHT + 2),
                    IM_COL32(255, 255, 255, 255), 2.0f);
                drawList->AddTriangleFilled(
                    ImVec2(markerX - 5, barStart.y - 6),
                    ImVec2(markerX + 5, barStart.y - 6),
                    ImVec2(markerX, barStart.y - 1),
                    IM_COL32(255, 255, 0, 255));

                ImGui::Dummy(ImVec2(BAR_WIDTH + 30, BAR_HEIGHT + 10));
                ImGui::Text("    Time: %.2f / %.2f sec", currentOffset, totalDuration);

                float elapsed = 0;
                for (auto& p : phases)
                {
                    if (currentOffset >= elapsed && currentOffset < elapsed + p.duration)
                    {
                        if (p.isDamagePhase)
                            ImGui::TextColored(ImVec4(1, 0, 0, 1), "    >>> DAMAGE ACTIVE! <<< Parry NOW!");
                        else
                            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "    [%s] No damage yet", p.name);
                        break;
                    }
                    elapsed += p.duration;
                }

                ImGui::TextColored(
                    enemy.attackJustLanded ? ImVec4(1, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1),
                    "    attackJustLanded: %s",
                    enemy.attackJustLanded ? "TRUE (HIT FRAME!)" : "false");
                ImGui::Spacing();
            }

            ImGui::TreePop();
        }

        // === �{�XAI�f�o�b�O ===
        if (ImGui::CollapsingHeader("Boss AI Debug"))
        {
            if (ImGui::CollapsingHeader("Boss Attack Timing"))
            {
                ImGui::Text("--- Melee ---");
                ImGui::SliderFloat("Melee Hit Time", &m_enemySystem->m_bossMeleeHitTime, 0.1f, 5.0f);
                ImGui::SliderFloat("Melee Duration", &m_enemySystem->m_bossMeleeDuration, 0.5f, 5.0f);

                ImGui::Text("--- Jump Slam ---");
                ImGui::SliderFloat("Jump Windup", &m_enemySystem->m_bossJumpWindupTime, 0.1f, 2.0f);
                ImGui::SliderFloat("Jump Air Time", &m_enemySystem->m_bossJumpAirTime, 0.2f, 2.0f);
                ImGui::SliderFloat("Jump Height", &m_enemySystem->m_bossJumpHeight, 2.0f, 20.0f);
                ImGui::SliderFloat("Slam Recovery", &m_enemySystem->m_bossSlamRecoveryTime, 0.1f, 3.0f);

                ImGui::Text("--- Slash ---");
                ImGui::SliderFloat("Slash Windup", &m_enemySystem->m_bossSlashWindupTime, 0.1f, 2.0f);
                ImGui::SliderFloat("Slash Fire", &m_enemySystem->m_bossSlashFireTime, 0.05f, 1.0f);
                ImGui::SliderFloat("Slash Recovery", &m_enemySystem->m_bossSlashRecoveryTime, 0.1f, 3.0f);

                ImGui::Text("--- Beam ---");
                ImGui::SliderFloat("Roar Windup", &m_enemySystem->m_bossRoarWindupTime, 0.1f, 5.0f);
                ImGui::SliderFloat("Roar Fire", &m_enemySystem->m_bossRoarFireTime, 1.0f, 10.0f);
                ImGui::SliderFloat("Roar Recovery", &m_enemySystem->m_bossRoarRecoveryTime, 0.5f, 5.0f);

                ImGui::Text("--- General ---");
                ImGui::SliderFloat("Attack Cooldown", &m_enemySystem->m_bossAttackCooldown, 0.5f, 10.0f);
            }

            // �t�F�[�Y���ϊ��p
            auto phaseName = [](BossAttackPhase p) -> const char* {
                switch (p) {
                case BossAttackPhase::IDLE:           return "IDLE";
                case BossAttackPhase::JUMP_WINDUP:    return "JUMP_WINDUP";
                case BossAttackPhase::JUMP_AIR:       return "JUMP_AIR";
                case BossAttackPhase::JUMP_SLAM:      return "JUMP_SLAM";
                case BossAttackPhase::SLAM_RECOVERY:  return "SLAM_RECOVERY";
                case BossAttackPhase::SLASH_WINDUP:    return "SLASH_WINDUP";
                case BossAttackPhase::SLASH_FIRE:      return "SLASH_FIRE";
                case BossAttackPhase::SLASH_RECOVERY:  return "SLASH_RECOVERY";
                case BossAttackPhase::ROAR_WINDUP:     return "ROAR_WINDUP";
                case BossAttackPhase::ROAR_FIRE:       return "ROAR_FIRE";
                case BossAttackPhase::ROAR_RECOVERY:   return "ROAR_RECOVERY";
                default: return "UNKNOWN";
                }
                };

            int bossCount = 0;
            for (auto& enemy : m_enemySystem->GetEnemies())
            {
                if (!enemy.isAlive) continue;
                if (enemy.type != EnemyType::MIDBOSS && enemy.type != EnemyType::BOSS) continue;

                bossCount++;
                const char* typeName = (enemy.type == EnemyType::BOSS) ? "BOSS" : "MIDBOSS";
                ImVec4 headerColor = (enemy.type == EnemyType::BOSS)
                    ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
                    : ImVec4(1.0f, 0.6f, 0.0f, 1.0f);

                ImGui::TextColored(headerColor, "=== %s (ID:%d) ===", typeName, enemy.id);

                // ��{���
                ImGui::Text("  HP: %.0f / %.0f", enemy.health, (float)GetEnemyConfig(enemy.type).health);
                ImGui::Text("  Pos: (%.1f, %.1f, %.1f)", enemy.position.x, enemy.position.y, enemy.position.z);

                // �v���C���[�Ƃ̋���
                DirectX::XMFLOAT3 pPos = m_player->GetPosition();
                float dx = pPos.x - enemy.position.x;
                float dz = pPos.z - enemy.position.z;
                float dist = sqrtf(dx * dx + dz * dz);
                ImGui::Text("  Distance to Player: %.1f", dist);

                // AI�t�F�[�Y�i�F�����j
                BossAttackPhase phase = enemy.bossPhase;
                ImVec4 phaseColor(0.5f, 0.5f, 0.5f, 1.0f); // �O���[
                if (phase == BossAttackPhase::IDLE) phaseColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                else if (phase == BossAttackPhase::JUMP_AIR || phase == BossAttackPhase::JUMP_SLAM)
                    phaseColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);  // �I�����W
                else if (phase == BossAttackPhase::SLASH_FIRE)
                    phaseColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // ��
                else if (phase == BossAttackPhase::ROAR_FIRE)
                    phaseColor = ImVec4(0.8f, 0.2f, 1.0f, 1.0f);  // ��
                else if (phase == BossAttackPhase::SLAM_RECOVERY ||
                    phase == BossAttackPhase::SLASH_RECOVERY ||
                    phase == BossAttackPhase::ROAR_RECOVERY)
                    phaseColor = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);  // �΁i�`�����X�j

                ImGui::TextColored(phaseColor, "  Phase: %s", phaseName(phase));
                ImGui::Text("  Phase Timer: %.2f s", enemy.bossPhaseTimer);
                ImGui::Text("  Cooldown: %.2f s", enemy.bossAttackCooldown);
                ImGui::Text("  Attack Count: %d", enemy.bossAttackCount);

                // �X�^�����
                float stunPercent = (enemy.maxStunValue > 0) ? enemy.stunValue / enemy.maxStunValue : 0;
                ImGui::Text("  Stun: %.0f / %.0f", enemy.stunValue, enemy.maxStunValue);
                ImGui::ProgressBar(stunPercent, ImVec2(200, 14), "");
                if (enemy.isStaggered)
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  *** STAGGERED ***");
                {
                    // ���f������A�j���̎��ۂ̒������擾
                    float actualDuration = 0.0f;
                    if (enemy.type == EnemyType::BOSS && m_bossModel)
                        actualDuration = m_bossModel->GetAnimationDuration(enemy.currentAnimation);
                    else if (enemy.type == EnemyType::MIDBOSS && m_midBossModel)
                        actualDuration = m_midBossModel->GetAnimationDuration(enemy.currentAnimation);

                    ImGui::Text("  Animation: %s (t=%.2f / %.2f)",
                        enemy.currentAnimation.c_str(), enemy.animationTime, actualDuration);
                }
                // �r�[�����iMIDBOSS�̂݁j
                if (enemy.type == EnemyType::MIDBOSS)
                {
                    ImGui::TextColored(
                        enemy.bossBeamParriable ? ImVec4(0.0f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                        "  Beam: %s", enemy.bossBeamParriable ? "GREEN (Parriable)" : "RED (Guard only)");
                }

                // =============================================
                // �U���^�C�����C������
                // =============================================
                // �� �ߐڍU���̃^�C�����C���iIDLE����Attack/AttackSlash�j
                if (enemy.bossPhase == BossAttackPhase::IDLE &&
                    (enemy.currentAnimation == "Attack" || enemy.currentAnimation == "AttackSlash"))
                {
                    ImGui::Spacing();
                    ImGui::Text("  --- Melee Attack Timeline ---");

                    struct PhaseInfo {
                        const char* name;
                        float duration;
                        ImVec4 color;
                        bool isDamagePhase;
                    };

                    float hitTime = m_enemySystem->m_bossMeleeHitTime;
                    float totalDuration = m_enemySystem->m_bossMeleeDuration;
                    float hitDuration = 0.05f;
                    float recovery = totalDuration - hitTime - hitDuration;
                    if (recovery < 0.01f) recovery = 0.01f;

                    std::vector<PhaseInfo> phases = {
                        {"WINDUP",   hitTime,     ImVec4(0.4f, 0.4f, 0.4f, 1), false},
                        {"HIT!",     hitDuration, ImVec4(1.0f, 0.0f, 0.0f, 1), true },
                        {"RECOVERY", recovery,    ImVec4(0.2f, 0.8f, 0.2f, 1), false},
                    };

                    float currentOffset = enemy.animationTime;
                    float total = 0;
                    for (auto& p : phases) total += p.duration;

                    float barWidth = 280.0f;
                    float barHeight = 20.0f;
                    ImVec2 barStart = ImGui::GetCursorScreenPos();
                    barStart.x += 20.0f;
                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    float xOffset = 0;
                    for (auto& p : phases)
                    {
                        float w = (p.duration / total) * barWidth;
                        ImU32 col = ImGui::ColorConvertFloat4ToU32(p.color);
                        if (p.isDamagePhase)
                        {
                            float blink = sinf((float)ImGui::GetTime() * 10.0f) * 0.3f + 0.7f;
                            ImVec4 bc = p.color; bc.w = blink;
                            col = ImGui::ColorConvertFloat4ToU32(bc);
                        }
                        drawList->AddRectFilled(
                            ImVec2(barStart.x + xOffset, barStart.y),
                            ImVec2(barStart.x + xOffset + w, barStart.y + barHeight), col);
                        if (w > 30)
                            drawList->AddText(ImVec2(barStart.x + xOffset + 2, barStart.y + 3),
                                IM_COL32(255, 255, 255, 220), p.name);
                        xOffset += w;
                    }

                    drawList->AddRect(barStart,
                        ImVec2(barStart.x + barWidth, barStart.y + barHeight),
                        IM_COL32(200, 200, 200, 180));

                    float markerX = barStart.x + (currentOffset / total) * barWidth;
                    markerX = (std::min)(markerX, barStart.x + barWidth);
                    markerX = (std::max)(markerX, barStart.x);
                    drawList->AddLine(
                        ImVec2(markerX, barStart.y - 2),
                        ImVec2(markerX, barStart.y + barHeight + 2),
                        IM_COL32(255, 255, 255, 255), 2.0f);
                    drawList->AddTriangleFilled(
                        ImVec2(markerX - 5, barStart.y - 6),
                        ImVec2(markerX + 5, barStart.y - 6),
                        ImVec2(markerX, barStart.y - 1),
                        IM_COL32(255, 255, 0, 255));

                    ImGui::Dummy(ImVec2(barWidth + 30, barHeight + 10));
                    ImGui::Text("    Time: %.2f / %.2f sec", currentOffset, total);

                    float elapsed = 0;
                    for (auto& p : phases)
                    {
                        if (currentOffset >= elapsed && currentOffset < elapsed + p.duration)
                        {
                            if (p.isDamagePhase)
                                ImGui::TextColored(ImVec4(1, 0, 0, 1), "    >>> DAMAGE ACTIVE! <<< Parry NOW!");
                            else
                                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "    [%s] No damage yet", p.name);
                            break;
                        }
                        elapsed += p.duration;
                    }

                    ImGui::TextColored(
                        enemy.attackJustLanded ? ImVec4(1, 0, 0, 1) : ImVec4(0.4f, 0.4f, 0.4f, 1),
                        "    attackJustLanded: %s",
                        enemy.attackJustLanded ? "TRUE (HIT FRAME!)" : "false");
                }


                ImGui::Separator();
            }

            if (bossCount == 0)
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No BOSS/MIDBOSS alive");

            // �v���W�F�N�^�C�����
            if (!m_bossProjectiles.empty())
            {
                ImGui::Text("Active Projectiles: %d", (int)m_bossProjectiles.size());
                for (int i = 0; i < (int)m_bossProjectiles.size() && i < 5; i++)
                {
                    auto& p = m_bossProjectiles[i];
                    ImGui::TextColored(
                        p.isParriable ? ImVec4(0.0f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                        "  [%d] %s pos(%.1f,%.1f) life=%.1f",
                        i, p.isParriable ? "GREEN" : "RED",
                        p.position.x, p.position.z, p.lifetime);
                }
            }

            // �V�[���h���
            ImGui::Separator();
            const char* shieldNames[] = { "Idle", "Parrying", "Guarding", "Throwing", "Charging", "Broken" };
            int si = (int)m_shieldState;
            if (si >= 0 && si < 6)
                ImGui::Text("Shield State: %s", shieldNames[si]);
            ImGui::Text("Shield HP: %.0f / %.0f", m_shieldHP, m_shieldMaxHP);
            ImGui::Text("Beam Handle: %d", m_beamHandle);


            // =============================================
            // �p���B�^�C�~���O
            // =============================================
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Parry Timing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // �p���B�E�B���h�E�i���A���^�C�������j
                ImGui::SliderFloat("Parry Window (sec)", &m_parryWindowDuration, 0.05f, 0.5f, "%.3f");

                // ���O�̃p���B����
                float timeSinceAttempt = m_gameTime - m_lastParryAttemptTime;
                float timeSinceResult = m_gameTime - m_lastParryResultTime;

                if (timeSinceAttempt < 3.0f)
                {
                    ImGui::TextColored(
                        m_lastParryWasSuccess ? ImVec4(0, 1, 0.3f, 1) : ImVec4(1, 0.3f, 0, 1),
                        "Last Parry: %s (%.2fs ago)",
                        m_lastParryWasSuccess ? "SUCCESS!" : "MISS",
                        timeSinceAttempt);
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "Last Parry: ---");
                }

                ImGui::Text("Success: %d / Fail: %d", m_parrySuccessCount, m_parryFailCount);

                // �p���B�E�B���h�E�����o�[
                float parryRatio = (m_shieldState == ShieldState::Parrying)
                    ? m_parryWindowTimer / m_parryWindowDuration : 0.0f;
                ImGui::Text("Parry Window:");
                ImGui::SameLine();
                ImVec4 barColor = (parryRatio > 0) ? ImVec4(0, 1, 0.3f, 1) : ImVec4(0.3f, 0.3f, 0.3f, 1);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                ImGui::ProgressBar(parryRatio, ImVec2(150, 16), parryRatio > 0 ? "ACTIVE" : "");
                ImGui::PopStyleColor();
            }

            // =============================================
            // �{�X�U���p�����[�^����
            // =============================================
            if (ImGui::CollapsingHeader("Boss Attack Tuning"))
            {
                ImGui::Text("--- Jump Slam ---");
                ImGui::SliderFloat("Slam Radius (BOSS)", &m_slamRadiusBoss, 2.0f, 15.0f);
                ImGui::SliderFloat("Slam Radius (MID)", &m_slamRadiusMidBoss, 2.0f, 12.0f);
                ImGui::SliderFloat("Slam Damage (BOSS)", &m_slamDamageBoss, 10.0f, 100.0f);
                ImGui::SliderFloat("Slam Damage (MID)", &m_slamDamageMidBoss, 5.0f, 60.0f);
                ImGui::SliderFloat("Slam Stun on Parry", &m_slamStunDamage, 10.0f, 100.0f);

                ImGui::Separator();
                ImGui::Text("--- Slash Projectile ---");
                ImGui::SliderFloat("Slash Speed", &m_slashSpeed, 5.0f, 30.0f);
                ImGui::SliderFloat("Slash Damage", &m_slashDamage, 10.0f, 80.0f);
                ImGui::SliderFloat("Slash Hit Radius", &m_slashHitRadius, 0.5f, 3.0f);
                ImGui::SliderFloat("Slash Stun on Parry", &m_slashStunOnParry, 10.0f, 100.0f);

                ImGui::Separator();
                ImGui::Text("--- Beam ---");
                ImGui::SliderFloat("Beam Width", &m_beamWidth, 0.5f, 5.0f);
                ImGui::SliderFloat("Beam Length", &m_beamLength, 5.0f, 40.0f);
                ImGui::SliderFloat("Beam DPS", &m_beamDPS, 5.0f, 50.0f);
                ImGui::SliderFloat("Beam Stun on Parry", &m_beamStunOnParry, 10.0f, 80.0f);
            }

            // =============================================
            // AI �^�C�~���O����
            // =============================================
            if (ImGui::CollapsingHeader("AI Phase Timing"))
            {
                ImGui::Text("--- Jump Attack ---");
                ImGui::SliderFloat("Jump Windup", &m_jumpWindupTime, 0.1f, 2.0f, "%.2f sec");
                ImGui::SliderFloat("Jump Air Time", &m_jumpAirTime, 0.2f, 2.0f, "%.2f sec");
                ImGui::SliderFloat("Slam Recovery", &m_slamRecoveryTime, 0.5f, 4.0f, "%.2f sec");

                ImGui::Separator();
                ImGui::Text("--- Slash Attack ---");
                ImGui::SliderFloat("Slash Windup", &m_slashWindupTime, 0.2f, 2.0f, "%.2f sec");
                ImGui::SliderFloat("Slash Recovery", &m_slashRecoveryTime, 0.3f, 3.0f, "%.2f sec");

                ImGui::Separator();
                ImGui::Text("--- Roar Beam ---");
                ImGui::SliderFloat("Roar Windup", &m_roarWindupTime, 0.5f, 5.0f, "%.2f sec");
                ImGui::SliderFloat("Roar Fire Time", &m_roarFireTime, 1.0f, 6.0f, "%.2f sec");
                ImGui::SliderFloat("Roar Recovery", &m_roarRecoveryTime, 0.5f, 4.0f, "%.2f sec");

                ImGui::Separator();
                ImGui::SliderFloat("Attack Cooldown", &m_bossAttackCooldownBase, 1.0f, 8.0f, "%.1f sec");

                if (ImGui::Button("Copy Current Values"))
                {
                    char buf[512];
                    sprintf_s(buf,
                        "// Jump: Windup=%.2f Air=%.2f Recovery=%.2f\n"
                        "// Slash: Windup=%.2f Recovery=%.2f\n"
                        "// Roar: Windup=%.2f Fire=%.2f Recovery=%.2f\n"
                        "// Parry Window=%.3f Cooldown=%.1f\n",
                        m_jumpWindupTime, m_jumpAirTime, m_slamRecoveryTime,
                        m_slashWindupTime, m_slashRecoveryTime,
                        m_roarWindupTime, m_roarFireTime, m_roarRecoveryTime,
                        m_parryWindowDuration, m_bossAttackCooldownBase);
                    ImGui::SetClipboardText(buf);
                    //OutputDebugStringA("[DEBUG] Boss timing values copied!\n");
                }
            }

        }

        if (ImGui::CollapsingHeader("Melee Charge System"))
        {
            // ���݂̏�ԕ\��
            ImGui::Text("Charges: %d / %d", m_meleeCharges, m_meleeMaxCharges);

            // �`���[�W�Q�[�W�i���o�I�j
            float chargeRatio = (float)m_meleeCharges / (float)m_meleeMaxCharges;
            ImGui::ProgressBar(chargeRatio, ImVec2(-1, 20),
                m_meleeCharges > 0 ? "READY" : "EMPTY");

            // ���`���[�W�^�C�}�[
            if (m_meleeCharges < m_meleeMaxCharges)
            {
                float rechargeProgress = m_meleeRechargeTimer / m_meleeRechargeTime;
                ImGui::ProgressBar(rechargeProgress, ImVec2(-1, 14), "Recharging...");
            }

            ImGui::Separator();

            // �����p�����[�^
            ImGui::SliderInt("Max Charges", &m_meleeMaxCharges, 1, 6);
            ImGui::SliderFloat("Recharge Time (sec)", &m_meleeRechargeTime, 1.0f, 15.0f);
            ImGui::SliderInt("Ammo per Punch", &m_meleeAmmoRefill, 1, 20);

            // �����Z�b�g�{�^��
            if (ImGui::Button("Refill Charges"))
            {
                m_meleeCharges = m_meleeMaxCharges;
                m_meleeRechargeTimer = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Empty Charges"))
            {
                m_meleeCharges = 0;
            }
        }

        // === �W�����v�f�o�b�O ===
        if (ImGui::CollapsingHeader("Jump Debug"))
        {
            DirectX::XMFLOAT3 pos = m_player->GetPosition();
            float feetY = pos.y - Player::EYE_HEIGHT;
            float floorBelow = GetFloorHeightBelow(pos.x, feetY, pos.z);
            float meshFloor = GetMeshFloorHeight(pos.x, pos.z, -9999.0f);

            ImGui::Text("Player Y:       %.3f", pos.y);
            ImGui::Text("Feet Y:         %.3f", feetY);
            ImGui::Text("VelocityY:      %.3f", m_player->GetVelocityY());
            ImGui::Text("IsGrounded:     %s", m_player->IsGrounded() ? "TRUE" : "FALSE");
            ImGui::Separator();
            ImGui::Text("FloorBelow:     %.3f (from feet)", floorBelow);
            ImGui::Text("MeshFloor:      %.3f (from Y=100)", meshFloor);
            ImGui::Separator();
            float groundLevel = (floorBelow > -9000.0f)
                ? floorBelow + Player::EYE_HEIGHT : -9999.0f;
            ImGui::Text("GroundLevel:    %.3f", groundLevel);
            ImGui::Text("Diff (Y-Ground):%.3f", pos.y - groundLevel);
        }

        ImGui::End();
    }

    // ImGui�`��
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

// ============================================================
//  DrawCapsule - �J�v�Z���`��̃��C���[�t���[���`��
//
//  �y�p�r�zBullet Physics�̃J�v�Z���R���C�_�[������
//  �y�\���z�V�����_�[�i�c��+�㉺�̉~�j+ �㉺�̔���
//  �y�������z16�Z�O�����g�i�p�t�H�[�}���X�ƌ��₷���̃o�����X�j
// ============================================================
void Game::DrawCapsule(
    DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
    const DirectX::XMFLOAT3& center,
    float radius,
    float cylinderHeight,
    const DirectX::XMFLOAT4& color)
{
    constexpr int SEGMENTS = 16;
    constexpr float PI = 3.14159f;
    constexpr int HEMISPHERE_DIRECTIONS = 4; // ������4��������`��

    float halfCylinder = cylinderHeight / 2.0f;

    // --- �V�����_�[�����i�c�̐��j ---
    for (int i = 0; i < SEGMENTS; i++)
    {
        float angle = (float)i / SEGMENTS * 2.0f * PI;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;

        DirectX::XMFLOAT3 bottom(center.x + x, center.y - halfCylinder, center.z + z);
        DirectX::XMFLOAT3 top(center.x + x, center.y + halfCylinder, center.z + z);

        batch->DrawLine(
            DirectX::VertexPositionColor(bottom, color),
            DirectX::VertexPositionColor(top, color));
    }

    // --- �V�����_�[�̏㉺�̉~ ---
    for (int i = 0; i < SEGMENTS; i++)
    {
        float angle1 = (float)i / SEGMENTS * 2.0f * PI;
        float angle2 = (float)(i + 1) / SEGMENTS * 2.0f * PI;

        // ���̉~
        batch->DrawLine(
            DirectX::VertexPositionColor(
                { center.x + cosf(angle1) * radius, center.y - halfCylinder, center.z + sinf(angle1) * radius }, color),
            DirectX::VertexPositionColor(
                { center.x + cosf(angle2) * radius, center.y - halfCylinder, center.z + sinf(angle2) * radius }, color));

        // ��̉~
        batch->DrawLine(
            DirectX::VertexPositionColor(
                { center.x + cosf(angle1) * radius, center.y + halfCylinder, center.z + sinf(angle1) * radius }, color),
            DirectX::VertexPositionColor(
                { center.x + cosf(angle2) * radius, center.y + halfCylinder, center.z + sinf(angle2) * radius }, color));
    }

    // --- �㉺�̔����i�ʂ�4��������`��j ---
    for (int i = 0; i < SEGMENTS / 2; i++)
    {
        float angle1 = (float)i / (SEGMENTS / 2) * PI / 2.0f;
        float angle2 = (float)(i + 1) / (SEGMENTS / 2) * PI / 2.0f;

        for (int j = 0; j < HEMISPHERE_DIRECTIONS; j++)
        {
            float dir = (float)j / HEMISPHERE_DIRECTIONS * 2.0f * PI;

            // ���̔���
            batch->DrawLine(
                DirectX::VertexPositionColor(
                    { center.x + cosf(dir) * cosf(angle1) * radius,
                      center.y - halfCylinder - sinf(angle1) * radius,
                      center.z + sinf(dir) * cosf(angle1) * radius }, color),
                DirectX::VertexPositionColor(
                    { center.x + cosf(dir) * cosf(angle2) * radius,
                      center.y - halfCylinder - sinf(angle2) * radius,
                      center.z + sinf(dir) * cosf(angle2) * radius }, color));

            // ��̔���
            batch->DrawLine(
                DirectX::VertexPositionColor(
                    { center.x + cosf(dir) * cosf(angle1) * radius,
                      center.y + halfCylinder + sinf(angle1) * radius,
                      center.z + sinf(dir) * cosf(angle1) * radius }, color),
                DirectX::VertexPositionColor(
                    { center.x + cosf(dir) * cosf(angle2) * radius,
                      center.y + halfCylinder + sinf(angle2) * radius,
                      center.z + sinf(dir) * cosf(angle2) * radius }, color));
        }
    }
}

// ============================================================
//  DrawHitboxes - �G�̃q�b�g�{�b�N�X�����C���[�t���[���\��
//
//  �y�`��Ώہz
//  - m_showHitboxes: �{�f�B�i�����̃��C���[�t���[���j
//  - m_showHeadHitboxes: ���i3���̉~���C���[�t���[���j
//  - m_showPhysicsHitboxes: Bullet�J�v�Z���iDrawCapsule�j
//  - m_showBulletTrajectory: ���C�L���X�g�������[�U�[
//
//  �y�p�t�H�[�}���X�zPrimitiveBatch�𖈃t���[��new���Ă���B
//  �{���̓����o�ϐ��ɃL���b�V�����ׂ������A�f�o�b�O��p�Ȃ̂ŋ��e�B
// ============================================================
void Game::DrawHitboxes()
{
    if (!m_showHitboxes && !m_showHeadHitboxes && !m_showPhysicsHitboxes)
        return;

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    // �J�����s����\�z
    DirectX::XMVECTOR cameraPosition = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR cameraTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f);
    DirectX::XMVECTOR upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(cameraPosition, cameraTarget, upVector);

    constexpr float FOV_DEGREES = 70.0f;
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 1000.0f;
    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(FOV_DEGREES), aspectRatio, NEAR_PLANE, FAR_PLANE);

    auto context = m_d3dContext.Get();
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    for (const auto& enemy : m_enemySystem->GetEnemies())
    {
        if (!enemy.isAlive) continue;

        // �w���p�[�Ńf�o�b�O/�f�t�H���g�ݒ���擾
        EnemyTypeConfig config = Game_GetDebugConfig(
            enemy.type, m_useDebugHitboxes,
            m_normalConfigDebug, m_runnerConfigDebug, m_tankConfigDebug,
            m_midbossConfigDebug, m_bossConfigDebug);

        // --- �{�f�B�q�b�g�{�b�N�X�i�����̃��C���[�t���[���j ---
        if (m_showHitboxes)
        {
            float width = config.bodyWidth;
            float height = config.bodyHeight;

            // �^�C�v�ʂɐF��ς���
            DirectX::XMFLOAT4 bodyColor;
            switch (enemy.type)
            {
            case EnemyType::NORMAL: bodyColor = { 1.0f, 0.0f, 0.0f, 1.0f }; break;
            case EnemyType::RUNNER: bodyColor = { 1.0f, 1.0f, 0.0f, 1.0f }; break;
            case EnemyType::TANK:   bodyColor = { 0.0f, 0.5f, 1.0f, 1.0f }; break;
            default:                bodyColor = { 1.0f, 1.0f, 1.0f, 1.0f }; break;
            }

            float hw = width / 2.0f;
            float ex = enemy.position.x, ey = enemy.position.y, ez = enemy.position.z;

            // ����4���_
            DirectX::XMFLOAT3 p1(ex - hw, ey, ez - hw), p2(ex + hw, ey, ez - hw);
            DirectX::XMFLOAT3 p3(ex + hw, ey, ez + hw), p4(ex - hw, ey, ez + hw);
            // ���4���_
            DirectX::XMFLOAT3 p5(ex - hw, ey + height, ez - hw), p6(ex + hw, ey + height, ez - hw);
            DirectX::XMFLOAT3 p7(ex + hw, ey + height, ez + hw), p8(ex - hw, ey + height, ez + hw);

            // ����
            primitiveBatch->DrawLine({ p1, bodyColor }, { p2, bodyColor });
            primitiveBatch->DrawLine({ p2, bodyColor }, { p3, bodyColor });
            primitiveBatch->DrawLine({ p3, bodyColor }, { p4, bodyColor });
            primitiveBatch->DrawLine({ p4, bodyColor }, { p1, bodyColor });
            // ���
            primitiveBatch->DrawLine({ p5, bodyColor }, { p6, bodyColor });
            primitiveBatch->DrawLine({ p6, bodyColor }, { p7, bodyColor });
            primitiveBatch->DrawLine({ p7, bodyColor }, { p8, bodyColor });
            primitiveBatch->DrawLine({ p8, bodyColor }, { p5, bodyColor });
            // �c��
            primitiveBatch->DrawLine({ p1, bodyColor }, { p5, bodyColor });
            primitiveBatch->DrawLine({ p2, bodyColor }, { p6, bodyColor });
            primitiveBatch->DrawLine({ p3, bodyColor }, { p7, bodyColor });
            primitiveBatch->DrawLine({ p4, bodyColor }, { p8, bodyColor });
        }

        // --- ���̃q�b�g�{�b�N�X�i�����C���[�t���[���j ---
        if (m_showHeadHitboxes && !enemy.headDestroyed)
        {
            DirectX::XMFLOAT3 headPos(
                enemy.position.x,
                enemy.position.y + config.headHeight,
                enemy.position.z);
            float headRadius = config.headRadius;
            DirectX::XMFLOAT4 headColor(1.0f, 0.0f, 0.0f, 1.0f);

            constexpr int HEAD_SEGMENTS = 16;
            constexpr float TWO_PI = 6.28318f;
            for (int i = 0; i < HEAD_SEGMENTS; i++)
            {
                float a1 = (i / (float)HEAD_SEGMENTS) * TWO_PI;
                float a2 = ((i + 1) / (float)HEAD_SEGMENTS) * TWO_PI;

                // XZ���ʂ̉~
                primitiveBatch->DrawLine(
                    { { headPos.x + cosf(a1) * headRadius, headPos.y, headPos.z + sinf(a1) * headRadius }, headColor },
                    { { headPos.x + cosf(a2) * headRadius, headPos.y, headPos.z + sinf(a2) * headRadius }, headColor });
                // XY���ʂ̉~
                primitiveBatch->DrawLine(
                    { { headPos.x + cosf(a1) * headRadius, headPos.y + sinf(a1) * headRadius, headPos.z }, headColor },
                    { { headPos.x + cosf(a2) * headRadius, headPos.y + sinf(a2) * headRadius, headPos.z }, headColor });
                // YZ���ʂ̉~
                primitiveBatch->DrawLine(
                    { { headPos.x, headPos.y + cosf(a1) * headRadius, headPos.z + sinf(a1) * headRadius }, headColor },
                    { { headPos.x, headPos.y + cosf(a2) * headRadius, headPos.z + sinf(a2) * headRadius }, headColor });
            }
        }

        // --- Bullet Physics�J�v�Z�� ---
        if (m_showPhysicsHitboxes)
        {
            auto it = m_enemyPhysicsBodies.find(enemy.id);
            if (it != m_enemyPhysicsBodies.end())
            {
                btRigidBody* body = it->second;
                btCollisionShape* shape = body->getCollisionShape();
                if (shape && shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE)
                {
                    btCapsuleShape* capsule = static_cast<btCapsuleShape*>(shape);
                    btTransform transform = body->getWorldTransform();
                    btVector3 origin = transform.getOrigin();

                    DrawCapsule(primitiveBatch.get(),
                        { origin.x(), origin.y(), origin.z() },
                        capsule->getRadius(),
                        capsule->getHalfHeight() * 2.0f,
                        { 0.0f, 1.0f, 0.0f, 1.0f }); // �ΐF
                }
            }
        }
    }

    // --- �e���\���i���C�L���X�g�����j ---
    if (m_showBulletTrajectory)
    {
        // �e���̋O��
        for (const auto& trace : m_bulletTraces)
        {
            constexpr float TRACE_FADE_TIME = 0.5f;
            float alpha = trace.lifetime / TRACE_FADE_TIME;
            DirectX::XMFLOAT4 traceColor(1.0f, 0.5f, 0.0f, alpha);
            primitiveBatch->DrawLine(
                DirectX::VertexPositionColor(trace.start, traceColor),
                DirectX::VertexPositionColor(trace.end, traceColor));
        }

        // �������[�U�[�i�ˌ�������Ԃ����ŕ\���j
        constexpr float LASER_LENGTH = 50.0f;
        constexpr float LASER_ALPHA = 0.8f;
        float dirX = sinf(playerRot.y) * cosf(playerRot.x);
        float dirY = -sinf(playerRot.x);
        float dirZ = cosf(playerRot.y) * cosf(playerRot.x);

        DirectX::XMFLOAT3 laserEnd(
            playerPos.x + dirX * LASER_LENGTH,
            playerPos.y + dirY * LASER_LENGTH,
            playerPos.z + dirZ * LASER_LENGTH);

        DirectX::XMFLOAT4 laserColor(1.0f, 0.0f, 0.0f, LASER_ALPHA);
        primitiveBatch->DrawLine(
            DirectX::VertexPositionColor(playerPos, laserColor),
            DirectX::VertexPositionColor(laserEnd, laserColor));
    }

    primitiveBatch->End();
}

// ============================================================
//  DrawBulletTracers - �e���̃r���{�[�h�N�A�b�h�`��
//
//  �y�`������z
//  ���Z�u�����h + �[�x��������OFF �ŁA����g���[�T�[�������B
//  �e���̕����ɑ΂��Đ����Ȗʂ����N�A�b�h�i�|���S���j��
//  �J�����Ɍ����ăr���{�[�h���B
//
//  �y���o�\���z
//  �O��: ����F�̃O���[�itrace.color �~ alpha�j
//  ����: �����R�A���C���itrace.width �~ 0.3�j
// ============================================================
void Game::DrawBulletTracers()
{
    if (m_bulletTraces.empty()) return;

    DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
    DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

    // �J�����s��
    DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR camTarget = DirectX::XMVectorSet(
        playerPos.x + sinf(playerRot.y) * cosf(playerRot.x),
        playerPos.y - sinf(playerRot.x),
        playerPos.z + cosf(playerRot.y) * cosf(playerRot.x),
        0.0f);
    DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(camPos, camTarget, upVec);

    constexpr float FOV_DEGREES = 70.0f;
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 1000.0f;
    float aspectRatio = (float)m_outputWidth / (float)m_outputHeight;
    DirectX::XMMATRIX projMatrix = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(FOV_DEGREES), aspectRatio, NEAR_PLANE, FAR_PLANE);

    auto context = m_d3dContext.Get();

    // ���Z�u�����h�i�����d�Ȃ��Ė��邭�Ȃ�j
    context->OMSetBlendState(m_states->Additive(), nullptr, 0xFFFFFFFF);
    // �[�x��������OFF�i���̃I�u�W�F�N�g�ɉB��邪�A�����͐[�x�������Ȃ��j
    context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    batch->Begin();

    for (const auto& trace : m_bulletTraces)
    {
        // �t�F�[�h���i���ŋ}���Ƀt�F�[�h�j
        float alpha = trace.lifetime / trace.maxLifetime;
        alpha = alpha * alpha;

        constexpr float MIN_VISIBLE_ALPHA = 0.01f;
        if (alpha < MIN_VISIBLE_ALPHA) continue;

        // --- �r���{�[�h�v�Z ---
        DirectX::XMVECTOR vStart = DirectX::XMLoadFloat3(&trace.start);
        DirectX::XMVECTOR vEnd = DirectX::XMLoadFloat3(&trace.end);
        DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(vEnd, vStart);
        DirectX::XMVECTOR dirNorm = DirectX::XMVector3Normalize(dir);

        // FPS���_�p�F�n�_���e���ʒu�ɃI�t�Z�b�g
        DirectX::XMVECTOR camDir = DirectX::XMVector3Normalize(
            DirectX::XMVectorSubtract(camTarget, camPos));
        DirectX::XMVECTOR camRight = DirectX::XMVector3Normalize(
            DirectX::XMVector3Cross(upVec, camDir));

        constexpr float MUZZLE_FORWARD = 1.5f;
        constexpr float MUZZLE_RIGHT = 0.3f;
        constexpr float MUZZLE_DOWN = 0.2f;
        vStart = DirectX::XMVectorAdd(camPos, DirectX::XMVectorScale(camDir, MUZZLE_FORWARD));
        vStart = DirectX::XMVectorAdd(vStart, DirectX::XMVectorScale(camRight, MUZZLE_RIGHT));
        vStart = DirectX::XMVectorAdd(vStart, DirectX::XMVectorScale(upVec, -MUZZLE_DOWN));

        // �J�����ɐ��΂���u���̕����v���O�ςŋ��߂�
        DirectX::XMVECTOR midpoint = DirectX::XMVectorScale(
            DirectX::XMVectorAdd(vStart, vEnd), 0.5f);
        DirectX::XMVECTOR toCam = DirectX::XMVector3Normalize(
            DirectX::XMVectorSubtract(camPos, midpoint));
        DirectX::XMVECTOR right = DirectX::XMVector3Cross(dirNorm, toCam);

        // �J�������e���ƈ꒼���̏ꍇ�̃t�H�[���o�b�N
        constexpr float CROSS_PRODUCT_EPSILON = 0.001f;
        if (DirectX::XMVectorGetX(DirectX::XMVector3Length(right)) < CROSS_PRODUCT_EPSILON)
            right = DirectX::XMVector3Cross(dirNorm, upVec);
        right = DirectX::XMVector3Normalize(right);

        // ���̃I�t�Z�b�g
        float halfW = trace.width * 0.5f;
        DirectX::XMVECTOR offset = DirectX::XMVectorScale(right, halfW);

        // 4���_���v�Z
        DirectX::XMFLOAT3 v0, v1, v2, v3;
        DirectX::XMStoreFloat3(&v0, DirectX::XMVectorSubtract(vStart, offset));
        DirectX::XMStoreFloat3(&v1, DirectX::XMVectorAdd(vStart, offset));
        DirectX::XMStoreFloat3(&v2, DirectX::XMVectorAdd(vEnd, offset));
        DirectX::XMStoreFloat3(&v3, DirectX::XMVectorSubtract(vEnd, offset));

        // �F�i���Z�u�����h�Ȃ̂�RGB�����ږ��邳�j
        constexpr float TAIL_BRIGHTNESS = 0.3f;
        constexpr float TAIL_ALPHA_SCALE = 0.5f;
        DirectX::XMFLOAT4 coreColor = { trace.color.x, trace.color.y, trace.color.z, alpha };
        DirectX::XMFLOAT4 tailColor = {
            trace.color.x * TAIL_BRIGHTNESS,
            trace.color.y * TAIL_BRIGHTNESS,
            trace.color.z * TAIL_BRIGHTNESS,
            alpha * TAIL_ALPHA_SCALE };

        // �O���N�A�b�h�i����F�O���[�j
        batch->DrawTriangle({ v0, tailColor }, { v1, tailColor }, { v2, coreColor });
        batch->DrawTriangle({ v0, tailColor }, { v2, coreColor }, { v3, coreColor });

        // �����R�A���C���i�����c�j
        constexpr float CORE_WIDTH_RATIO = 0.3f;
        float coreHalfW = halfW * CORE_WIDTH_RATIO;
        DirectX::XMVECTOR coreOffset = DirectX::XMVectorScale(right, coreHalfW);

        DirectX::XMFLOAT3 c0, c1, c2, c3;
        DirectX::XMStoreFloat3(&c0, DirectX::XMVectorSubtract(vStart, coreOffset));
        DirectX::XMStoreFloat3(&c1, DirectX::XMVectorAdd(vStart, coreOffset));
        DirectX::XMStoreFloat3(&c2, DirectX::XMVectorAdd(vEnd, coreOffset));
        DirectX::XMStoreFloat3(&c3, DirectX::XMVectorSubtract(vEnd, coreOffset));

        DirectX::XMFLOAT4 whiteCore = { 1.0f, 1.0f, 1.0f, alpha };
        DirectX::XMFLOAT4 whiteTail = { TAIL_BRIGHTNESS, TAIL_BRIGHTNESS, TAIL_BRIGHTNESS, alpha * TAIL_ALPHA_SCALE };

        batch->DrawTriangle({ c0, whiteTail }, { c1, whiteTail }, { c2, whiteCore });
        batch->DrawTriangle({ c0, whiteTail }, { c2, whiteCore }, { c3, whiteCore });
    }

    batch->End();

    // �u�����h��Ԃ����ɖ߂�
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(nullptr, 0);
}