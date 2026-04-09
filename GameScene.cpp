// ============================================================
//  GameScene.cpp
//  �V�[���Ǘ��i�^�C�g��/���[�f�B���O/�Q�[���I�[�o�[/�����L���O/�t�F�[�h�j
//
//  �y�Ӗ��z
//  - �e�V�[����Update�i���͎�t�E��ԑJ�ځE�^�C�}�[�i�s�j
//  - �e�V�[����Render�iSpriteBatch/PrimitiveBatch��UI�`��j
//  - �t�F�[�h�C��/�t�F�[�h�A�E�g�̐���
//
//  �y�݌v�Ӑ}�z
//  �Q�[���v���C�ȊO�́u��ʑJ�ځv���W��B
//  �����I��State�p�^�[���Ɉڍs����ꍇ���A
//  ���̃t�@�C���𕪊����邾���ōςށB
//
//  �y�܂܂��V�[���z
//  1. Title    - TitleScene�N���X�̍X�V/�`�� + SPACE��LOADING
//  2. Loading  - �t�F�C�N���[�f�B���O�o�[ + Enter��PLAYING
//  3. GameOver - YOU DIED���o + �X�R�A�\�� + ���O����
//  4. Ranking  - ���[�_�[�{�[�h�\��
//  5. Fade     - �V�[���J�ڎ��̍��t�F�[�h
// ============================================================

#include "Game.h"
#include <algorithm>

using namespace DirectX;

// ============================================================
//  �V�[�����ʒ萔
// ============================================================
namespace SceneConstants
{
    // deltaTime�i�Œ�t���[�����[�g�z��j
    constexpr float FIXED_DT = 1.0f / 60.0f;

    // �t�F�[�h���x�i�b������̃A���t�@�ω��ʁj
    constexpr float FADE_SPEED = 2.0f;

    // �Q�[���I�[�o�[���o�^�C�~���O
    constexpr float GO_COUNTUP_START = 1.5f;     // �X�R�A�J�E���g�A�b�v�J�n����
    constexpr float GO_COUNTUP_DURATION = 2.0f;   // �J�E���g�A�b�v�ɂ����鎞��
    constexpr float GO_STAT_DELAY = 0.15f;         // �X�^�b�c�̒i�K�I�\���̊Ԋu
    constexpr float GO_STAT_DURATION = 0.6f;       // �e�X�^�b�c�̃J�E���g�A�b�v����
    constexpr float GO_NOISE_DURATION = 0.5f;      // �m�C�Y�g�����W�V�����̒���
    constexpr float GO_NAME_INPUT_START = 4.5f;    // ���O���͊J�n�^�C�~���O
    constexpr float GO_INPUT_DELAY = 0.5f;         // ���͎�t�J�n�̒x��

    // ���[�f�B���O��ʃ^�C�~���O
    constexpr float LOAD_PHASE_0_END = 0.5f;
    constexpr float LOAD_PHASE_1_END = 1.2f;
    constexpr float LOAD_PHASE_2_END = 1.8f;
    constexpr float LOAD_PHASE_3_END = 2.5f;
    constexpr float LOAD_PHASE_4_END = 3.0f;
    constexpr float LOAD_BAR_LERP_SPEED = 3.0f;

    // �X�R�A�v�Z�̔{��
    constexpr int KILL_POINTS = 10;
    constexpr int HEADSHOT_POINTS = 25;
    constexpr int MELEE_KILL_POINTS = 30;
    constexpr int COMBO_POINTS = 20;
    constexpr int PARRY_POINTS = 50;
    constexpr int SURVIVAL_POINTS_PER_SEC = 2;
    constexpr int DAMAGE_DIVISOR = 10;
    constexpr int NO_DAMAGE_BONUS = 1000;
    constexpr int STYLE_RANK_POINTS = 200;

    // �����N臒l
    constexpr int RANK_S_THRESHOLD = 8000;
    constexpr int RANK_A_THRESHOLD = 5000;
    constexpr int RANK_B_THRESHOLD = 2500;

    // ���O���͂̍ő啶����
    constexpr int MAX_NAME_LENGTH = 15;
}

// ============================================================
//  �C�[�W���O�֐��iRenderGameOver / RenderRanking ���ʁj
// ============================================================
namespace Easing
{
    // ������ƌ����i�I�[�o�[���C�����j
    float OutQuad(float t)
    {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    // �����������������i�X���C�h�C�������j
    float OutCubic(float t)
    {
        float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    // �ړI�n���s���߂��ăo�l�Ŗ߂�i�����������j
    float OutBack(float t)
    {
        constexpr float OVERSHOOT = 1.70158f;
        float u = t - 1.0f;
        return 1.0f + (OVERSHOOT + 1.0f) * u * u * u + OVERSHOOT * u * u;
    }

    // �o�E���h�iYOU DIED�����j
    float OutBounce(float t)
    {
        constexpr float N1 = 7.5625f;
        constexpr float D1 = 2.75f;
        if (t < 1.0f / D1)
            return N1 * t * t;
        else if (t < 2.0f / D1) {
            t -= 1.5f / D1;
            return N1 * t * t + 0.75f;
        }
        else if (t < 2.5f / D1) {
            t -= 2.25f / D1;
            return N1 * t * t + 0.9375f;
        }
        else {
            t -= 2.625f / D1;
            return N1 * t * t + 0.984375f;
        }
    }
}


// ============================================================
//  UpdateTitle - �^�C�g����ʂ̍X�V
//
//  �y���́zSPACE �� ���[�f�B���O�ցAL �� �����L���O��
//  �y�����zTitleScene�̍X�V + �t�F�[�h�X�V
// ============================================================
void Game::UpdateTitle()
{
    // TitleScene �̍X�V
    if (m_titleScene)
    {
        m_titleScene->Update(SceneConstants::FIXED_DT);
    }

    // �t�F�[�h�X�V
    UpdateFade();

    // SPACE�L�[�ŃQ�[���J�n
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        ResetGame();

        // ���[�f�B���O��ʂ̏�����
        m_loadingTimer = 0.0f;
        m_loadingPhase = 0;
        m_loadingBarTarget = 0.0f;
        m_loadingBarCurrent = 0.0f;

        m_gameState = GameState::LOADING;
    }

    // L�L�[�Ń����L���O���
    if (GetAsyncKeyState('L') & 0x8000)
    {
        m_rankingTimer = 0.0f;
        m_newRecordRank = -1;
        m_gameState = GameState::RANKING;
    }
}

// ============================================================
//  UpdateLoading - ���[�f�B���O��ʂ̍X�V
//
//  �y�t�F�C�N���[�f�B���O�z���ۂ̃��\�[�X�ǂݍ��݂�
//  Initialize()�Ŋ����ς݁B���o�Ƃ��Ẵ��[�f�B���O��ʁB
//  5�t�F�[�Y�iINIT��ARSENAL��ENTITIES��CALIBRATE��READY�j
// ============================================================
void Game::UpdateLoading()
{
    m_loadingTimer += SceneConstants::FIXED_DT;

    // �t�F�[�Y�i�s�i���ԂŃ��b�Z�[�W�ƃo�[�̖ڕW�l��؂�ւ��j
    if (m_loadingTimer < SceneConstants::LOAD_PHASE_0_END)
    {
        m_loadingPhase = 0;       // "INITIALIZING SYSTEMS..."
        m_loadingBarTarget = 0.1f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_1_END)
    {
        m_loadingPhase = 1;       // "LOADING ARSENAL..."
        m_loadingBarTarget = 0.3f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_2_END)
    {
        m_loadingPhase = 2;       // "SPAWNING ENTITIES..."
        m_loadingBarTarget = 0.55f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_3_END)
    {
        m_loadingPhase = 3;       // "CALIBRATING GOTHIC FREQUENCIES..."
        m_loadingBarTarget = 0.78f;
    }
    else if (m_loadingTimer < SceneConstants::LOAD_PHASE_4_END)
    {
        m_loadingPhase = 4;       // "READY"
        m_loadingBarTarget = 1.0f;
    }

    // �o�[�̃X���[�Y��ԁiLerp�j
    float lerpSpeed = SceneConstants::LOAD_BAR_LERP_SPEED * SceneConstants::FIXED_DT;
    m_loadingBarCurrent += (m_loadingBarTarget - m_loadingBarCurrent) * lerpSpeed;

    // �҂�����ڕW�ɋ߂Â�����X�i�b�v
    constexpr float SNAP_THRESHOLD = 0.001f;
    if (fabsf(m_loadingBarCurrent - m_loadingBarTarget) < SNAP_THRESHOLD)
        m_loadingBarCurrent = m_loadingBarTarget;

    // READY��AEnter�L�[�őJ��
    constexpr int READY_PHASE = 4;
    if (m_loadingPhase == READY_PHASE)
    {
        m_loadingBarCurrent = 1.0f;

        static bool enterPressed = false;
        if (GetAsyncKeyState(VK_RETURN) & 0x8000)
        {
            if (!enterPressed)
            {
                m_fadeAlpha = 1.0f;
                m_fadingIn = true;
                m_fadeActive = true;
                m_gameState = GameState::PLAYING;
                enterPressed = true;
            }
        }
        else
        {
            enterPressed = false;
        }
    }
}

// ============================================================
//  UpdateFade - �t�F�[�h�C��/�t�F�[�h�A�E�g�̍X�V
//
//  �y�d�g�݁zm_fadingIn=true �� �A���t�@�����i���邭�Ȃ�j
//           m_fadingIn=false �� �A���t�@�����i�Â��Ȃ�j
// ============================================================
void Game::UpdateFade()
{
    if (!m_fadeActive) return;

    float fadeSpeed = SceneConstants::FADE_SPEED * SceneConstants::FIXED_DT;

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

// ============================================================
//  UpdateRanking - �����L���O��ʂ̍X�V
//
//  �y���́zESC �� �^�C�g���ɖ߂�AR �� ���X�^�[�g
// ============================================================
void Game::UpdateRanking()
{
    m_rankingTimer += SceneConstants::FIXED_DT;

    // ESC�L�[�Ń^�C�g���ɖ߂�i�둀��h�~��0.5�b�ォ���t�j
    if (m_rankingTimer > SceneConstants::GO_INPUT_DELAY &&
        (GetAsyncKeyState(VK_ESCAPE) & 0x8000))
    {
        m_gameState = GameState::TITLE;
        m_rankingTimer = 0.0f;
    }

    // R�L�[�Ń��X�^�[�g
    if (m_rankingTimer > SceneConstants::GO_INPUT_DELAY &&
        (GetAsyncKeyState('R') & 0x8000))
    {
        ResetGame();
        m_gameOverTimer = 0.0f;
        m_gameOverScore = 0;
        m_gameOverWave = 0;
        m_gameOverCountUp = 0.0f;
        m_gameOverRank = 0;
        m_gameOverNoiseT = 0.0f;
        m_gameState = GameState::TITLE;
    }
}


// ============================================================
//  UpdateGameOver - �Q�[���I�[�o�[��ʂ̍X�V
//
//  �y�����t���[�z
//  1. ����: �X�R�A�E�X�^�b�c�v�Z + �����N����
//  2. ���o: �J�E���g�A�b�v�A�j���[�V����
//  3. ���O����: A-Z, 0-9, Backspace, Enter
//  4. �ۑ���: R�����X�^�[�g�AL�������L���O
//
//  �y�X�R�A�v�Z���z
//  ���v = �L���~10 + HS�~25 + �ߐځ~30 + �R���{�~20
//       + �p���B�~50 + �����b�~2 + �^�_��/10
//       + (��_��0�Ȃ�1000, �ۂȂ�5000/(��_��+1))
//       + �X�^�C�������N�~200
// ============================================================
void Game::UpdateGameOver()
{
    // �o�ߎ��Ԃ����Z�i�S���o�̃^�C�~���O����Ɏg�p�j
    m_gameOverTimer += SceneConstants::FIXED_DT;

    // --- ����̂݁F�X�R�A�ƃX�^�b�c���v�Z ---
    if (m_gameOverWave == 0)
    {
        m_gameOverScore = m_player->GetPoints();
        m_gameOverWave = m_waveManager->GetCurrentWave();

        // �X�^�b�c�����U���g�p�ɃR�s�[
        m_goKills = m_statKills;
        m_goHeadshots = m_statHeadshots;
        m_goMeleeKills = m_statMeleeKills;
        m_goMaxCombo = m_statMaxCombo;
        m_goParryCount = m_parrySuccessCount;
        m_goDamageDealt = m_statDamageDealt;
        m_goDamageTaken = m_statDamageTaken;
        m_goMaxStyleRank = m_statMaxStyleRank;
        m_goSurvivalTime = m_statSurvivalTime;

        // �e�X�^�b�c�Ƀ{�[�i�X�|�C���g���v�Z
        m_goStatScores[0] = m_goKills * SceneConstants::KILL_POINTS;
        m_goStatScores[1] = m_goHeadshots * SceneConstants::HEADSHOT_POINTS;
        m_goStatScores[2] = m_goMeleeKills * SceneConstants::MELEE_KILL_POINTS;
        m_goStatScores[3] = m_goMaxCombo * SceneConstants::COMBO_POINTS;
        m_goStatScores[4] = m_goParryCount * SceneConstants::PARRY_POINTS;
        m_goStatScores[5] = (int)(m_goSurvivalTime)*SceneConstants::SURVIVAL_POINTS_PER_SEC;
        m_goStatScores[6] = m_goDamageDealt / SceneConstants::DAMAGE_DIVISOR;
        m_goStatScores[7] = (m_goDamageTaken == 0) ? SceneConstants::NO_DAMAGE_BONUS :
            (int)(5000.0f / (float)(m_goDamageTaken + 1));
        m_goStatScores[8] = m_goMaxStyleRank * SceneConstants::STYLE_RANK_POINTS;

        // ���v�X�R�A
        m_goTotalScore = 0;
        for (int i = 0; i < 9; i++)
            m_goTotalScore += m_goStatScores[i];

        // �����N����
        if (m_goTotalScore >= SceneConstants::RANK_S_THRESHOLD)
            m_gameOverRank = 3; // S
        else if (m_goTotalScore >= SceneConstants::RANK_A_THRESHOLD)
            m_gameOverRank = 2; // A
        else if (m_goTotalScore >= SceneConstants::RANK_B_THRESHOLD)
            m_gameOverRank = 1; // B
        else
            m_gameOverRank = 0; // C

        // �J�E���g�A�b�v������
        for (int i = 0; i < 9; i++)
            m_goStatCountUp[i] = 0.0f;

        // ���O���͂̏���
        if (!m_rankingSaved && !m_nameInputActive)
        {
            memset(m_nameBuffer, 0, sizeof(m_nameBuffer));
            m_nameLength = 0;
            m_nameKeyTimer = 0.0f;
        }
    }

    // --- �X�R�A�J�E���g�A�b�v���o�iEaseOutExpo�j ---
    if (m_gameOverTimer > SceneConstants::GO_COUNTUP_START)
    {
        float countProgress = (m_gameOverTimer - SceneConstants::GO_COUNTUP_START)
            / SceneConstants::GO_COUNTUP_DURATION;
        if (countProgress > 1.0f) countProgress = 1.0f;
        m_gameOverCountUp = 1.0f - powf(2.0f, -10.0f * countProgress);
    }

    // --- �X�^�b�c�̒i�K�I�J�E���g�A�b�v�i0.15�b�����炵��9�s�j ---
    for (int i = 0; i < 9; i++)
    {
        float startTime = SceneConstants::GO_COUNTUP_START + i * SceneConstants::GO_STAT_DELAY;
        if (m_gameOverTimer > startTime)
        {
            float progress = (m_gameOverTimer - startTime) / SceneConstants::GO_STAT_DURATION;
            if (progress > 1.0f) progress = 1.0f;
            m_goStatCountUp[i] = 1.0f - powf(2.0f, -10.0f * progress);
        }
    }

    // --- �m�C�Y�g�����W�V�����i���S����ɉ�ʂ��U�U�b�ƃm�C�Y�Ő؂�ւ��j ---
    if (m_gameOverTimer < SceneConstants::GO_NOISE_DURATION)
    {
        m_gameOverNoiseT = m_gameOverTimer / SceneConstants::GO_NOISE_DURATION;
    }
    else
    {
        m_gameOverNoiseT = 1.0f;
    }

    // =============================================
    // ���O���͂̏����i4.5�b��ɊJ�n�j
    // =============================================
    if (m_gameOverTimer > SceneConstants::GO_NAME_INPUT_START && !m_rankingSaved)
    {
        // ���O���̓��[�h�J�n
        if (!m_nameInputActive)
        {
            m_nameInputActive = true;
            memset(m_nameBuffer, 0, sizeof(m_nameBuffer));
            m_nameLength = 0;
            m_nameKeyWasDown = false;
            OutputDebugStringA("[RANKING] Name input started\n");
        }

        // ���t���[���ŉ����L�[��������Ă��邩���ׂ�
        bool anyKeyDown = false;
        int pressedKey = 0;
        bool isBackspace = false;
        bool isEnter = false;

        if (GetAsyncKeyState(VK_RETURN) & 0x8000) { anyKeyDown = true; isEnter = true; }
        if (GetAsyncKeyState(VK_BACK) & 0x8000) { anyKeyDown = true; isBackspace = true; }

        // A-Z �`�F�b�N
        if (!anyKeyDown)
        {
            for (int key = 'A'; key <= 'Z'; key++)
            {
                if (GetAsyncKeyState(key) & 0x8000)
                {
                    anyKeyDown = true;
                    pressedKey = key;
                    break;
                }
            }
        }

        // 0-9 �`�F�b�N
        if (!anyKeyDown)
        {
            for (int key = '0'; key <= '9'; key++)
            {
                if (GetAsyncKeyState(key) & 0x8000)
                {
                    anyKeyDown = true;
                    pressedKey = key;
                    break;
                }
            }
        }

        // �u�������u�ԁv������������i�O�t���[���ŉ�����ĂȂ��āA�������ꂽ�j
        if (anyKeyDown && !m_nameKeyWasDown)
        {
            if (isEnter)
            {
                if (m_nameLength == 0)
                {
                    strcpy_s(m_nameBuffer, "NONAME");
                    m_nameLength = 6;
                }

                // �����L���O�ɕۑ�
                RankingEntry entry;
                entry.score = m_goTotalScore;
                entry.wave = m_gameOverWave;
                entry.kills = m_goKills;
                entry.headshots = m_goHeadshots;
                entry.rank = m_gameOverRank;
                entry.survivalTime = m_goSurvivalTime;
                strcpy_s(entry.name, m_nameBuffer);

                m_newRecordRank = m_rankingSystem->AddEntry(entry);
                m_rankingSaved = true;
                m_nameInputActive = false;
            }
            else if (isBackspace)
            {
                if (m_nameLength > 0)
                {
                    m_nameLength--;
                    m_nameBuffer[m_nameLength] = '\0';
                }
            }
            else if (pressedKey != 0 && m_nameLength < SceneConstants::MAX_NAME_LENGTH)
            {
                m_nameBuffer[m_nameLength] = (char)pressedKey;
                m_nameLength++;
                m_nameBuffer[m_nameLength] = '\0';
            }
        }

        m_nameKeyWasDown = anyKeyDown;
        return;  // ���O���͒��� R/L �L�[���u���b�N
    }

    // --- �ۑ���FR�����X�^�[�g�AL�������L���O ---
    if (m_gameOverTimer > SceneConstants::GO_NAME_INPUT_START &&
        (GetAsyncKeyState('R') & 0x8000))
    {
        ResetGame();
        m_gameOverTimer = 0.0f;
        m_gameOverScore = 0;
        m_gameOverWave = 0;
        m_gameOverCountUp = 0.0f;
        m_gameOverRank = 0;
        m_gameOverNoiseT = 0.0f;
        m_gameState = GameState::TITLE;
    }

    if (m_gameOverTimer > SceneConstants::GO_NAME_INPUT_START &&
        (GetAsyncKeyState('L') & 0x8000))
    {
        m_rankingTimer = 0.0f;
        m_gameState = GameState::RANKING;
    }
}


// ============================================================
//  RenderTitle - �^�C�g����ʂ̕`��
//
//  �y�`����e�zTitleScene��3D�`�� + �t�F�[�h�I�[�o�[���C
//  TitleScene�����݂��Ȃ��ꍇ�͉����`�悵�Ȃ��i���S�j
// ============================================================
void Game::RenderTitle()
{
    // === ��ʃN���A ===
    Clear();

    // === �^�C�g���V�[����`�� ===
    if (m_titleScene)
    {
        try
        {
            m_titleScene->Render(
                m_d3dContext.Get(),
                m_renderTargetView.Get(),    // �o�b�N�o�b�t�@
                m_depthStencilView.Get()     // �[�x�o�b�t�@
            );
        }
        catch (const std::exception& e)
        {
            //OutputDebugStringA("[RENDER ERROR] TitleScene render failed: ");
            //OutputDebugStringA(e.what());
            //OutputDebugStringA("\n");
        }
    }
    else
    {
        // TitleScene ���Ȃ��ꍇ�̓G���[���b�Z�[�W��\��
        //OutputDebugStringA("[RENDER WARNING] TitleScene is null\n");
    }

    // === UI�`��i�I�v�V�����j===
    // �^�C�g���e�L�X�g�Ȃǂ�//����ꍇ
    /*
    m_spriteBatch->Begin();

    DirectX::XMFLOAT2 titlePos(
        m_outputWidth / 2.0f,
        m_outputHeight / 2.0f
    );

    m_fontLarge->DrawString(
        m_spriteBatch.get(),
        L"GOTHIC SWARM",
        titlePos,
        DirectX::Colors::White,
        0.0f,
        DirectX::XMFLOAT2(0.5f, 0.5f)
    );

    DirectX::XMFLOAT2 startPos(
        m_outputWidth / 2.0f,
        m_outputHeight / 2.0f + 100.0f
    );

    m_font->DrawString(
        m_spriteBatch.get(),
        L"Press ENTER to Start",
        startPos,
        DirectX::Colors::White,
        0.0f,
        DirectX::XMFLOAT2(0.5f, 0.5f)
    );

    m_spriteBatch->End();
    */

    // === �t�F�[�h�`�� ===
    RenderFade();
}

// ============================================================
//  RenderLoading - ���[�f�B���O��ʂ̕`��
//
//  �y�`��\���z
//  1. "GOTHIC SWARM" �^�C�g�����S�i�t�F�[�h�C���j
//  2. �t�F�[�Y���b�Z�[�W�i5�i�K�j
//  3. �v���O���X�o�[�i�p���X����ԁj
//  4. �p�[�Z���g�\��
//  5. "Press Enter" �_�Ńe�L�X�g�iREADY��j
// ============================================================
void Game::RenderLoading()
{
    // === ��ʃN���A�i�^�����j ===
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), black);

    float W = (float)m_outputWidth;
    float H = (float)m_outputHeight;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->NonPremultiplied()
    );

    // =========================================
    // ���[�h���b�Z�[�W
    // =========================================
    const wchar_t* messages[] = {
        L"INITIALIZING SYSTEMS...",
        L"LOADING ARSENAL...",
        L"SPAWNING ENTITIES...",
        L"CALIBRATING GOTHIC FREQUENCIES...",
        L"READY"
    };

    const wchar_t* msg = messages[m_loadingPhase];

    // ���b�Z�[�W�̐F�i�ŏI�t�F�[�Y�����Ԃ��j
    DirectX::XMVECTORF32 msgColor;
    if (m_loadingPhase == 4)
        msgColor = { 0.9f, 0.15f, 0.1f, 1.0f };   // �ԁiREADY�j
    else
        msgColor = { 0.6f, 0.6f, 0.6f, 1.0f };     // �O���[

    // ������������ɕ\��
    DirectX::XMVECTOR msgSize = m_font->MeasureString(msg);
    float msgW = DirectX::XMVectorGetX(msgSize);
    float msgX = (W - msgW) * 0.5f;
    float msgY = H * 0.58f;

    m_font->DrawString(m_spriteBatch.get(), msg,
        DirectX::XMFLOAT2(msgX, msgY), msgColor);

    // =========================================
    // �v���O���X�o�[
    // =========================================
    if (m_whitePixel)
    {
        float barW = W * 0.5f;       // �o�[�̍ő啝�i��ʕ���50%�j
        float barH = 6.0f;            // �o�[�̍���
        float barX = (W - barW) * 0.5f;
        float barY = H * 0.65f;

        // --- �w�i�i�Â��O���[�j ---
        RECT bgRect = {
            (LONG)barX,
            (LONG)barY,
            (LONG)(barX + barW),
            (LONG)(barY + barH)
        };
        DirectX::XMVECTORF32 bgColor = { 0.15f, 0.15f, 0.15f, 1.0f };
        m_spriteBatch->Draw(m_whitePixel.Get(), bgRect, bgColor);

        // --- �i���o�[�i�ԁj ---
        float fillW = barW * m_loadingBarCurrent;
        RECT fillRect = {
            (LONG)barX,
            (LONG)barY,
            (LONG)(barX + fillW),
            (LONG)(barY + barH)
        };

        // �p���X���ʁi�����ɖ��Łj
        float pulse = 0.7f + sinf(m_loadingTimer * 4.0f) * 0.3f;
        DirectX::XMVECTORF32 barColor = { 0.8f * pulse, 0.1f * pulse, 0.05f * pulse, 1.0f };
        m_spriteBatch->Draw(m_whitePixel.Get(), fillRect, barColor);

        // --- �p�[�Z���g�\�� ---
        int percent = (int)(m_loadingBarCurrent * 100.0f);
        wchar_t percentText[16];
        swprintf_s(percentText, L"%d%%", percent);

        DirectX::XMVECTOR pSize = m_font->MeasureString(percentText);
        float pW = DirectX::XMVectorGetX(pSize);
        float pX = (W - pW) * 0.5f;
        float pY = barY + barH + 10.0f;

        DirectX::XMVECTORF32 percentColor = { 0.5f, 0.5f, 0.5f, 0.8f };
        m_font->DrawString(m_spriteBatch.get(), percentText,
            DirectX::XMFLOAT2(pX, pY), percentColor);
    }

    // =========================================
    // �^�C�g�����S�i��ʏ㕔�j
    // =========================================
    if (m_fontLarge)
    {
        const wchar_t* title = L"GOTHIC SWARM";
        DirectX::XMVECTOR titleSize = m_fontLarge->MeasureString(title);
        float titleW = DirectX::XMVectorGetX(titleSize);
        float titleX = (W - titleW) * 0.5f;
        float titleY = H * 0.3f;

        // ������ƃt�F�[�h�C��
        float titleAlpha = (m_loadingTimer < 0.8f) ? (m_loadingTimer / 0.8f) : 1.0f;
        DirectX::XMVECTORF32 titleColor = { 0.9f, 0.1f, 0.05f, titleAlpha };

        m_fontLarge->DrawString(m_spriteBatch.get(), title,
            DirectX::XMFLOAT2(titleX, titleY), titleColor);
    }

    // =========================================
    //  �_�ł���h�b�g���o�i...�̃A�j���[�V�����j
    // =========================================
    if (m_loadingPhase < 4)
    {
        // 0.5�b���ƂɃh�b�g��������i1?3�j
        int dots = ((int)(m_loadingTimer * 2.0f) % 3) + 1;
        wchar_t dotText[8] = L"";
        for (int i = 0; i < dots; i++)
            wcscat_s(dotText, L".");

        float dotX = msgX + msgW + 2.0f;
        DirectX::XMVECTORF32 dotColor = { 0.4f, 0.4f, 0.4f, 0.6f };
        m_font->DrawString(m_spriteBatch.get(), dotText,
            DirectX::XMFLOAT2(dotX, msgY), dotColor);
    }

    // =========================================
    // "Press Enter" �\���iREADY��j
    // =========================================
    if (m_loadingPhase == 4)
    {
        const wchar_t* pressText = L"- - Press Enter - -";

        // �_��
        float blink = (sinf(m_loadingTimer * 4.0f) + 1.0f) * 0.5f;
        float alpha = 0.4f + blink * 0.6f;

        DirectX::XMVECTOR peSize = m_font->MeasureString(pressText);
        float peW = DirectX::XMVectorGetX(peSize);
        float peX = (W - peW) * 0.5f;
        float peY = H * 0.75f;

        DirectX::XMVECTORF32 peColor = { 0.8f, 0.8f, 0.8f, alpha };
        m_font->DrawString(m_spriteBatch.get(), pressText,
            DirectX::XMFLOAT2(peX, peY), peColor);
    }

    m_spriteBatch->End();
}

// ============================================================
//  RenderGameOver - �Q�[���I�[�o�[��ʂ̕`��i��800�s�j
//
//  �y�`�惌�C���[�z
//  Layer 0: �m�C�Y�g�����W�V�����i�������m�C�Y + �ԃt���b�V���j
//  Layer 1: �ԍ��r�l�b�g�i��ʑS�̂𕢂��Łj
//  Layer 2: SpriteBatch�Ńe�L�X�g+����
//    - "YOU DIED"�i�o�E���X�ŗ����Ă��� + �O���[�j
//    - ���p�l��: �X�^�b�c9�s�i�J�E���g�A�b�v + �h�b�g���[�_�[�j
//    - �E�p�l��: �����N�����i�t���b�V�����ˁj + �g�[�^���X�R�A
//    - ���O����UI + "Press R" / "L: RANKING"
// ============================================================
void Game::RenderGameOver()
{
    // =============================================
    // �C�[�W���O�֐��iUI���o�̐S�����j
    // �S�� t=0.0�i�J�n�j�� t=1.0�i�����j�Œl��Ԃ�
    // =============================================

    // EaseOutQuad: ������ƌ����i�I�[�o�[���C�����j

    // EaseOutCubic: �����������������i�X���C�h�C�������j

    // EaseOutBack: �ړI�n���s���߂��ăo�l�Ŗ߂�i�����������j

    // EaseOutBounce: �o�E���h�iYOU DIED�����j

    // �o�ߎ��Ԃ̃V���[�g�J�b�g
    float timer = m_gameOverTimer;
    float screenW = (float)m_outputWidth;
    float screenH = (float)m_outputHeight;
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    // =============================================
    // Layer 0: �m�C�Y�g�����W�V�����i���S����ɑ������m�C�Y�j
    // =============================================
    if (m_gameOverNoiseT < 1.0f && m_whitePixel && m_spriteBatch && m_states)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // �����_���Ȑ������C����`��i�������m�C�Y���j
        float noiseAlpha = 1.0f - m_gameOverNoiseT;  // ���񂾂������
        int numLines = (int)(60 * (1.0f - m_gameOverNoiseT));  // ���C����������

        for (int i = 0; i < numLines; i++)
        {
            // �^�������_���i�t���[�����Ƃɕς��j
            float seed = sinf((float)i * 127.1f + timer * 311.7f) * 43758.5453f;
            seed = seed - floorf(seed);  // 0?1�ɐ��K��

            float lineY = seed * screenH;
            float lineH = 1.0f + seed * 3.0f;  // 1?4px�̑���

            // �����_����X�����I�t�Z�b�g�i��ʂ��Y���銴���j
            float offsetX = sinf(seed * 100.0f + timer * 50.0f) * 20.0f * noiseAlpha;

            RECT noiseRect = {
                (LONG)(offsetX), (LONG)lineY,
                (LONG)(screenW + offsetX), (LONG)(lineY + lineH)
            };

            // ����������
            DirectX::XMVECTORF32 noiseColor = { 1.0f, 1.0f, 1.0f, noiseAlpha * 0.3f * seed };
            m_spriteBatch->Draw(m_whitePixel.Get(), noiseRect, noiseColor);
        }

        // ��ʑS�̂ɐԂ��t���b�V���i��u�����j
        if (m_gameOverNoiseT < 0.15f)
        {
            float flashAlpha = (0.15f - m_gameOverNoiseT) / 0.15f;
            RECT fullScreen = { 0, 0, (LONG)screenW, (LONG)screenH };
            DirectX::XMVECTORF32 flashColor = { 0.8f, 0.0f, 0.0f, flashAlpha * 0.5f };
            m_spriteBatch->Draw(m_whitePixel.Get(), fullScreen, flashColor);
        }

        m_spriteBatch->End();
    }

    auto context = m_d3dContext.Get();

    // =============================================
    // Layer 1: �ԍ��r�l�b�g�i��ʑS�̂𕢂��Ö��j
    // =============================================
    {
        // ������ƈÂ��Ȃ�iEaseOutQuad�Ŏ��R�Ȍ����j
        float overlayT = min(timer / 1.5f, 1.0f);
        float overlayAlpha = Easing::OutQuad(overlayT) * 0.88f;

        DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
            screenW, screenH, 0.1f, 10.0f);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(DirectX::XMMatrixIdentity());
        m_effect->SetVertexColorEnabled(true);
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        auto primBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        primBatch->Begin();

        float hw = screenW * 0.5f;
        float hh = screenH * 0.5f;

        // �����͐ԍ��A�[�͂����ƈÂ��i�r�l�b�g���ʁj
        // �����̐F�i���Ԃ��j
        DirectX::XMFLOAT4 centerColor(0.12f, 0.0f, 0.0f, overlayAlpha * 0.7f);
        // �[�̐F�i�قڐ^�����j
        DirectX::XMFLOAT4 edgeColor(0.03f, 0.0f, 0.0f, overlayAlpha);

        // ���S�_
        DirectX::VertexPositionColor vc(DirectX::XMFLOAT3(0, 0, 1.0f), centerColor);

        // �l��
        DirectX::VertexPositionColor vTL(DirectX::XMFLOAT3(-hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vTR(DirectX::XMFLOAT3(hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBR(DirectX::XMFLOAT3(hw, -hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBL(DirectX::XMFLOAT3(-hw, -hh, 1.0f), edgeColor);

        // ���S����l���ւ̎O�p�`4���i�O���f�[�V�������r�l�b�g�j
        primBatch->DrawTriangle(vc, vTL, vTR);  // ��
        primBatch->DrawTriangle(vc, vTR, vBR);  // �E
        primBatch->DrawTriangle(vc, vBR, vBL);  // ��
        primBatch->DrawTriangle(vc, vBL, vTL);  // ��

        primBatch->End();
    }

    // =============================================
    // Layer 2: SpriteBatch�Ńe�L�X�g�{����
    // =============================================
    if (m_spriteBatch && m_states && m_font && m_whitePixel)
    {
        m_spriteBatch->Begin(
            DirectX::SpriteSortMode_Deferred,
            m_states->AlphaBlend(),
            nullptr,
            m_states->DepthNone()
        );

        // --- �������i��j: 0.3�b��ɍ��E���璆���֐L�т� ---
        if (timer > 0.3f)
        {
            float lineT = min((timer - 0.3f) / 0.5f, 1.0f);
            // EaseOutBack�ōs���߂��ăo�l�Ŗ߂�
            float lineProgress = Easing::OutBack(lineT);
            float lineHalfW = 220.0f * lineProgress;  // �ő�Б�220px
            float lineY = centerY - 340.0f;             // YOU DIED�̏�

            // �����̐��i�������獶�֐L�т�j
            RECT leftLine = {
                (LONG)(centerX - lineHalfW), (LONG)lineY,
                (LONG)centerX, (LONG)(lineY + 2)
            };
            // �E���̐��i��������E�֐L�т�j
            RECT rightLine = {
                (LONG)centerX, (LONG)lineY,
                (LONG)(centerX + lineHalfW), (LONG)(lineY + 2)
            };

            DirectX::XMVECTORF32 lineColor = { 0.7f, 0.15f, 0.0f, 0.9f * lineT };
            m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
            m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

            // �����̏����ȃ_�C�������i�S�V�b�N���j
            RECT diamond = {
                (LONG)(centerX - 4), (LONG)(lineY - 3),
                (LONG)(centerX + 4), (LONG)(lineY + 5)
            };
            m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
        }

        // --- �uYOU DIED�v: 0.6�b��Ƀo�E���h���Ȃ���ォ�痎���� ---
        if (timer > 0.6f)
        {
            float diedT = min((timer - 0.6f) / 0.8f, 1.0f);
            // EaseOutBounce�Ńh�X���Ɨ����ăo�E���h
            float bounceT = Easing::OutBounce(diedT);
            float textAlpha = min((timer - 0.6f) * 3.0f, 1.0f);

            // �ォ��X���C�h�i-80px �� 0px�j
            float slideY = (1.0f - bounceT) * -80.0f;
            float diedY = centerY - 320.0f + slideY;

            //  �O���[�i�����j�F�����e�L�X�g���������炵�Ĕ������ŕ�����`��
            if (m_fontLarge)
            {
                // �e�L�X�g���𑪂��Ď蓮�Œ�������
                DirectX::XMVECTOR diedSize = m_fontLarge->MeasureString(L"YOU DIED");
                float diedW = DirectX::XMVectorGetX(diedSize);
                float diedX = centerX - diedW * 0.5f;

                // �O���[�i4�����ɂ��炵�Ĕ������ŕ`�����������Č�����j
                float glowAlpha = textAlpha * 0.25f;
                DirectX::XMVECTORF32 glowColor = { 1.0f, 0.2f, 0.0f, glowAlpha };
                float glowSize = 3.0f;
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX - glowSize, diedY), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX + glowSize, diedY), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY - glowSize), glowColor);
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY + glowSize), glowColor);

                // �{�̃e�L�X�g
                DirectX::XMVECTORF32 diedColor = { 0.95f, 0.15f, 0.05f, textAlpha };
                m_fontLarge->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(diedX, diedY), diedColor);
            }
            else
            {
                // fontLarge�������ꍇ�͒ʏ�t�H���g�~2�{�X�P�[��
                DirectX::XMVECTORF32 diedColor = { 0.95f, 0.15f, 0.05f, textAlpha };
                m_font->DrawString(m_spriteBatch.get(), L"YOU DIED",
                    DirectX::XMFLOAT2(centerX, diedY), diedColor, 0.0f,
                    DirectX::XMFLOAT2(0.5f, 0.5f), 2.0f);
            }
        }

        // ===================================================================
        //  2�J�������C�A�E�g
        //  ���p�l��: �X�^�b�c9�s   �E�p�l��: �����N + �g�[�^���X�R�A
        // ===================================================================

        // --- �p�l�����W ---
        const float leftL = 55.0f;
        const float leftR = 605.0f;
        const float rightL = 645.0f;
        const float rightR = 1225.0f;
        const float rightCX = (rightL + rightR) * 0.5f;
        const float panelTop = 125.0f;
        const float panelBot = 555.0f;

        // --- �p�l���w�i�i�������_�[�N�{�b�N�X�j---
        if (timer > 0.8f)
        {
            float bgT = min((timer - 0.8f) / 0.4f, 1.0f);
            float bgA = Easing::OutCubic(bgT) * 0.2f;

            RECT leftBg = { (LONG)leftL,  (LONG)panelTop, (LONG)leftR,  (LONG)panelBot };
            RECT rightBg = { (LONG)rightL, (LONG)panelTop, (LONG)rightR, (LONG)panelBot };
            DirectX::XMVECTORF32 bgCol = { 0.05f, 0.01f, 0.0f, bgA };
            m_spriteBatch->Draw(m_whitePixel.Get(), leftBg, bgCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), rightBg, bgCol);

            // �p�l���g���i�Â��ԁj
            float borderA = bgT * 0.35f;
            DirectX::XMVECTORF32 borderCol = { 0.5f, 0.1f, 0.0f, borderA };
            RECT bTop1 = { (LONG)leftL, (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + 1) };
            RECT bBot1 = { (LONG)leftL, (LONG)(panelBot - 1), (LONG)leftR, (LONG)panelBot };
            RECT bLft1 = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + 1), (LONG)panelBot };
            RECT bRgt1 = { (LONG)(leftR - 1), (LONG)panelTop, (LONG)leftR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), bTop1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bBot1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bLft1, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bRgt1, borderCol);

            RECT bTop2 = { (LONG)rightL, (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + 1) };
            RECT bBot2 = { (LONG)rightL, (LONG)(panelBot - 1), (LONG)rightR, (LONG)panelBot };
            RECT bLft2 = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + 1), (LONG)panelBot };
            RECT bRgt2 = { (LONG)(rightR - 1), (LONG)panelTop, (LONG)rightR, (LONG)panelBot };

            //  �R�[�i�[�����iL���^�A�e�p�l���̎l���j
            float cornerLen = 20.0f;
            float cornerThick = 2.0f;
            DirectX::XMVECTORF32 cornerCol = { 0.8f, 0.2f, 0.0f, borderA * 1.5f };

            // ���p�l���l��
            // ����
            RECT cLT_H = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + cornerLen), (LONG)(panelTop + cornerThick) };
            RECT cLT_V = { (LONG)leftL, (LONG)panelTop, (LONG)(leftL + cornerThick), (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), cLT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cLT_V, cornerCol);
            // �E��
            RECT cRT_H = { (LONG)(leftR - cornerLen), (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + cornerThick) };
            RECT cRT_V = { (LONG)(leftR - cornerThick), (LONG)panelTop, (LONG)leftR, (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), cRT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cRT_V, cornerCol);
            // ����
            RECT cLB_H = { (LONG)leftL, (LONG)(panelBot - cornerThick), (LONG)(leftL + cornerLen), (LONG)panelBot };
            RECT cLB_V = { (LONG)leftL, (LONG)(panelBot - cornerLen), (LONG)(leftL + cornerThick), (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), cLB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cLB_V, cornerCol);
            // �E��
            RECT cRB_H = { (LONG)(leftR - cornerLen), (LONG)(panelBot - cornerThick), (LONG)leftR, (LONG)panelBot };
            RECT cRB_V = { (LONG)(leftR - cornerThick), (LONG)(panelBot - cornerLen), (LONG)leftR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), cRB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), cRB_V, cornerCol);

            // �E�p�l���l��
            RECT c2LT_H = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + cornerLen), (LONG)(panelTop + cornerThick) };
            RECT c2LT_V = { (LONG)rightL, (LONG)panelTop, (LONG)(rightL + cornerThick), (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LT_V, cornerCol);
            RECT c2RT_H = { (LONG)(rightR - cornerLen), (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + cornerThick) };
            RECT c2RT_V = { (LONG)(rightR - cornerThick), (LONG)panelTop, (LONG)rightR, (LONG)(panelTop + cornerLen) };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RT_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RT_V, cornerCol);
            RECT c2LB_H = { (LONG)rightL, (LONG)(panelBot - cornerThick), (LONG)(rightL + cornerLen), (LONG)panelBot };
            RECT c2LB_V = { (LONG)rightL, (LONG)(panelBot - cornerLen), (LONG)(rightL + cornerThick), (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2LB_V, cornerCol);
            RECT c2RB_H = { (LONG)(rightR - cornerLen), (LONG)(panelBot - cornerThick), (LONG)rightR, (LONG)panelBot };
            RECT c2RB_V = { (LONG)(rightR - cornerThick), (LONG)(panelBot - cornerLen), (LONG)rightR, (LONG)panelBot };
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RB_H, cornerCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), c2RB_V, cornerCol);

            m_spriteBatch->Draw(m_whitePixel.Get(), bTop2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bBot2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bLft2, borderCol);
            m_spriteBatch->Draw(m_whitePixel.Get(), bRgt2, borderCol);
        }

        // ======================
        //  ���p�l��: �X�^�b�c9�s
        // ======================
        if (timer > 1.2f)
        {
            const wchar_t* styleRankNames[] = { L"D", L"C", L"B", L"A", L"S", L"SS", L"SSS" };

            const float tableL = leftL + 20.0f;
            const float colVal = leftR - 140.0f;
            const float colBonus = leftR - 20.0f;
            const float statStartY = panelTop + 20.0f;
            const float rowH = 25.0f;

            const wchar_t* labels[] = {
                L"KILLS", L"HEADSHOTS", L"MELEE KILLS", L"MAX COMBO",
                L"PARRIES", L"SURVIVAL", L"DMG DEALT", L"DMG TAKEN", L"BEST STYLE"
            };
            int rawValues[] = {
                m_goKills, m_goHeadshots, m_goMeleeKills, m_goMaxCombo,
                m_goParryCount, 0, m_goDamageDealt, m_goDamageTaken, m_goMaxStyleRank
            };

            for (int i = 0; i < 9; i++)
            {
                float rowStart = 1.2f + i * 0.12f;
                if (timer <= rowStart) continue;

                float t = min((timer - rowStart) / 0.3f, 1.0f);
                float ease = Easing::OutCubic(t);
                float alpha = ease;
                float slideUp = (1.0f - ease) * 8.0f;

                float y = statStartY + i * rowH + slideUp;



                // ���x���i�������j
                DirectX::XMVECTORF32 labelColor = { 0.6f, 0.6f, 0.6f, alpha };
                m_font->DrawString(m_spriteBatch.get(), labels[i],
                    DirectX::XMFLOAT2(tableL, y), labelColor);

                // �h�b�g���[�_�[
                DirectX::XMVECTOR labelSize = m_font->MeasureString(labels[i]);
                float labelEnd = tableL + DirectX::XMVectorGetX(labelSize);
                float dotS = labelEnd + 6.0f;
                float dotE = colVal - 70.0f;
                if (dotE > dotS && m_whitePixel)
                {
                    for (float dx = dotS; dx < dotE; dx += 6.0f)
                    {
                        RECT dot = { (LONG)dx, (LONG)(y + 8), (LONG)(dx + 2), (LONG)(y + 10) };
                        DirectX::XMVECTORF32 dotColor = { 0.35f, 0.35f, 0.35f, alpha * 0.5f };
                        m_spriteBatch->Draw(m_whitePixel.Get(), dot, dotColor);
                    }
                }

                // �l�i�E�����j
                wchar_t valBuf[64];
                if (i == 5)
                {
                    int sec = (int)(m_goSurvivalTime * m_goStatCountUp[i]);
                    swprintf_s(valBuf, L"%d:%02d", sec / 60, sec % 60);
                }
                else if (i == 8)
                {
                    int dr = (int)(m_goMaxStyleRank * m_goStatCountUp[i]);
                    if (dr > 6) dr = 6;
                    swprintf_s(valBuf, L"%s", styleRankNames[dr]);
                }
                else
                {
                    swprintf_s(valBuf, L"%d", (int)(rawValues[i] * m_goStatCountUp[i]));
                }

                DirectX::XMVECTORF32 valColor = { 1.0f, 1.0f, 1.0f, alpha };
                DirectX::XMVECTOR valSize = m_font->MeasureString(valBuf);
                float valW = DirectX::XMVectorGetX(valSize);
                m_font->DrawString(m_spriteBatch.get(), valBuf,
                    DirectX::XMFLOAT2(colVal - valW, y), valColor);

                // �{�[�i�X�|�C���g�i�E�����A���F�j
                int dispScore = (int)(m_goStatScores[i] * m_goStatCountUp[i]);
                wchar_t sBuf[64];
                swprintf_s(sBuf, L"+%d", dispScore);
                DirectX::XMVECTORF32 bonusColor = { 1.0f, 0.85f, 0.2f, alpha };
                DirectX::XMVECTOR sSize = m_font->MeasureString(sBuf);
                float sW = DirectX::XMVectorGetX(sSize);
                m_font->DrawString(m_spriteBatch.get(), sBuf,
                    DirectX::XMFLOAT2(colBonus - sW, y), bonusColor);
            }

            // --- ���p�l������: WAVE�\�� ---
            float waveStart = 1.2f + 9 * 0.12f + 0.2f;
            if (timer > waveStart)
            {
                float wt = min((timer - waveStart) / 0.4f, 1.0f);
                float wa = Easing::OutCubic(wt);

                float waveY = statStartY + 9 * rowH + 20.0f;
                wchar_t waveBuf[64];
                swprintf_s(waveBuf, L"WAVE  %d", m_gameOverWave);
                DirectX::XMVECTORF32 waveCol = { 0.8f, 0.3f, 0.1f, wa };
                m_font->DrawString(m_spriteBatch.get(), waveBuf,
                    DirectX::XMFLOAT2(tableL, waveY), waveCol);
            }
        }

        // =====================================
        //  �E�p�l��: �����N + �g�[�^���X�R�A
        // =====================================

        // --- �uRANK�v���x�� ---
        if (timer > 2.0f)
        {
            float rlT = min((timer - 2.0f) / 0.3f, 1.0f);
            float rlA = Easing::OutCubic(rlT);
            DirectX::XMVECTORF32 rlCol = { 0.5f, 0.5f, 0.5f, rlA * 0.7f };
            DirectX::XMVECTOR rlSize = m_font->MeasureString(L"RANK");
            float rlW = DirectX::XMVectorGetX(rlSize);
            m_font->DrawString(m_spriteBatch.get(), L"RANK",
                DirectX::XMFLOAT2(rightCX - rlW * 0.5f, panelTop + 25.0f), rlCol);
        }

        // // �����N�o��t���b�V���i�����̌`�ɉ����ĕ��ˏ�Ɍ���j
        if (timer > 2.15f && timer < 4.0f && m_fontLarge)
        {
            float flashT = (timer - 2.15f) / 1.85f;  // 0��1�i1.85�b�����Ă�����茸���j
            float flashA = (1.0f - flashT) * (1.0f - flashT);
            float intensity = (m_gameOverRank >= 2) ? 1.0f : 0.5f;

            const wchar_t* rankNames[] = { L"C", L"B", L"A", L"S" };
            const wchar_t* rn = rankNames[m_gameOverRank];
            float rankY = panelTop + 70.0f;

            DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rn);
            float rW = DirectX::XMVectorGetX(rSize);
            float rH = DirectX::XMVectorGetY(rSize);
            float rX = rightCX - rW * 0.5f;

            // �t���b�V���F
            DirectX::XMFLOAT3 flashRGB;
            if (m_gameOverRank == 3)      flashRGB = { 1.0f, 0.4f, 0.1f };
            else if (m_gameOverRank == 2) flashRGB = { 1.0f, 0.9f, 0.3f };
            else if (m_gameOverRank == 1) flashRGB = { 0.5f, 0.7f, 1.0f };
            else                          flashRGB = { 0.8f, 0.8f, 0.8f };

            // === �O�����C���[: 16�����ɑ傫������ ===
            float outerDist = 12.0f + (1.0f - flashT) * 60.0f;
            float outerDirs[][2] = {
                {1,0},{-1,0},{0,1},{0,-1},
                {0.707f,0.707f},{-0.707f,0.707f},{0.707f,-0.707f},{-0.707f,-0.707f},
                {0.92f,0.38f},{-0.92f,0.38f},{0.92f,-0.38f},{-0.92f,-0.38f},
                {0.38f,0.92f},{-0.38f,0.92f},{0.38f,-0.92f},{-0.38f,-0.92f}
            };
            for (int d = 0; d < 16; d++)
            {
                float dx = outerDirs[d][0] * outerDist;
                float dy = outerDirs[d][1] * outerDist;
                DirectX::XMVECTORF32 gc = { flashRGB.x, flashRGB.y, flashRGB.z, flashA * intensity * 0.2f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === ���ԃ��C���[: 8���� ===
            float midDist = 5.0f + (1.0f - flashT) * 30.0f;
            for (int d = 0; d < 8; d++)
            {
                float dx = outerDirs[d][0] * midDist;
                float dy = outerDirs[d][1] * midDist;
                DirectX::XMVECTORF32 gc = { flashRGB.x, flashRGB.y, flashRGB.z, flashA * intensity * 0.35f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === �������C���[: 8�����i���邢�j===
            float innerDist = 2.0f + (1.0f - flashT) * 12.0f;
            for (int d = 0; d < 8; d++)
            {
                float dx = outerDirs[d][0] * innerDist;
                float dy = outerDirs[d][1] * innerDist;
                DirectX::XMVECTORF32 gc = {
                    min(flashRGB.x + 0.3f, 1.0f),
                    min(flashRGB.y + 0.3f, 1.0f),
                    min(flashRGB.z + 0.2f, 1.0f),
                    flashA * intensity * 0.5f };
                m_fontLarge->DrawString(m_spriteBatch.get(), rn,
                    DirectX::XMFLOAT2(rX + dx, rankY + dy), gc);
            }

            // === S/A�����N: �w�i�ɂڂ�����̉~ ===
            if (m_gameOverRank >= 2 && m_whitePixel)
            {
                float glowRadius = 80.0f + (1.0f - flashT) * 120.0f;
                float cx = rightCX;
                float cy = rankY + rH * 0.5f;
                // ���S�~�I��3�w�̌�
                for (int ring = 0; ring < 3; ring++)
                {
                    float r = glowRadius * (0.4f + ring * 0.3f);
                    float ringA = flashA * intensity * 0.08f / (ring + 1);
                    RECT glowRect = {
                        (LONG)(cx - r), (LONG)(cy - r),
                        (LONG)(cx + r), (LONG)(cy + r)
                    };
                    DirectX::XMVECTORF32 glowCol = { flashRGB.x, flashRGB.y, flashRGB.z, ringA };
                    m_spriteBatch->Draw(m_whitePixel.Get(), glowRect, glowCol);
                }
            }
        }


        // --- �����N�����i�傫�������Ɂj---
        if (timer > 2.2f)
        {
            float rankT = min((timer - 2.2f) / 0.5f, 1.0f);
            float rankEase = Easing::OutBack(rankT);
            float rankAlpha = min((timer - 2.2f) * 3.0f, 1.0f);

            const wchar_t* rankNames[] = { L"C", L"B", L"A", L"S" };
            const wchar_t* rankName = rankNames[m_gameOverRank];

            DirectX::XMVECTORF32 rankColors[] = {
                { 0.5f, 0.5f, 0.5f, rankAlpha },
                { 0.3f, 0.5f, 1.0f, rankAlpha },
                { 1.0f, 0.85f, 0.0f, rankAlpha },
                { 1.0f, 0.2f, 0.0f, rankAlpha },
            };
            DirectX::XMVECTORF32 rankColor = rankColors[m_gameOverRank];

            float rankY = panelTop + 70.0f;

            if (m_fontLarge)
            {
                // �O���[�iA/S�����N�j
                if (m_gameOverRank >= 2)
                {
                    float glowPulse = (sinf(timer * 4.0f) + 1.0f) * 0.5f;
                    DirectX::XMVECTORF32 glowColor = { rankColor.f[0], rankColor.f[1], rankColor.f[2],
                        rankAlpha * 0.2f * (0.5f + glowPulse * 0.5f) };
                    float glow = 5.0f;
                    DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rankName);
                    float rW = DirectX::XMVectorGetX(rSize);
                    float rH = DirectX::XMVectorGetY(rSize);
                    float rX = rightCX - rW * 0.5f;
                    DirectX::XMFLOAT2 glowOrigin(rW * 0.5f, rH * 0.5f);
                    DirectX::XMFLOAT2 glowCenter(rightCX, rankY + rH * 0.5f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x - glow, glowCenter.y), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x + glow, glowCenter.y), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x, glowCenter.y - glow), glowColor, 0.0f, glowOrigin, 2.0f);
                    m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                        DirectX::XMFLOAT2(glowCenter.x, glowCenter.y + glow), glowColor, 0.0f, glowOrigin, 2.0f);
                }

                DirectX::XMVECTOR rSize = m_fontLarge->MeasureString(rankName);
                float rW = DirectX::XMVectorGetX(rSize);
                float rH = DirectX::XMVectorGetY(rSize);
                // 2�{�X�P�[���ŕ`��iorigin�𒆉��ɂ��Ċg��j
                DirectX::XMFLOAT2 origin(rW * 0.5f, rH * 0.5f);
                DirectX::XMFLOAT2 pos(rightCX, rankY + rH * 0.5f);
                m_fontLarge->DrawString(m_spriteBatch.get(), rankName,
                    pos, rankColor, 0.0f, origin, 2.0f);
            }
            else
            {
                DirectX::XMVECTOR rSize = m_font->MeasureString(rankName);
                float rW = DirectX::XMVectorGetX(rSize);
                m_font->DrawString(m_spriteBatch.get(), rankName,
                    DirectX::XMFLOAT2(rightCX - rW * 0.5f, rankY), rankColor);
            }
        }

        // --- �E�p�l��: ��؂�� ---
        if (timer > 2.5f)
        {
            float sepT = min((timer - 2.5f) / 0.4f, 1.0f);
            float sepE = Easing::OutCubic(sepT);
            float sepY = panelTop + 220.0f;
            float sepHW = 200.0f * sepE;
            RECT sepLine = {
                (LONG)(rightCX - sepHW), (LONG)sepY,
                (LONG)(rightCX + sepHW), (LONG)(sepY + 1)
            };
            DirectX::XMVECTORF32 sepCol = { 0.5f, 0.12f, 0.0f, sepE * 0.6f };
            m_spriteBatch->Draw(m_whitePixel.Get(), sepLine, sepCol);
        }

        // --- �E�p�l��: TOTAL SCORE ---
        if (timer > 2.7f)
        {
            float tsT = min((timer - 2.7f) / 0.4f, 1.0f);
            float tsA = Easing::OutCubic(tsT);

            float scoreY = panelTop + 240.0f;
            DirectX::XMVECTORF32 tsLabelCol = { 0.6f, 0.6f, 0.6f, tsA * 0.8f };
            DirectX::XMVECTOR tsLabelSize = m_font->MeasureString(L"TOTAL SCORE");
            float tsLabelW = DirectX::XMVectorGetX(tsLabelSize);
            m_font->DrawString(m_spriteBatch.get(), L"TOTAL SCORE",
                DirectX::XMFLOAT2(rightCX - tsLabelW * 0.5f, scoreY), tsLabelCol);

            int dispTotal = (int)(m_goTotalScore * m_gameOverCountUp);
            wchar_t tBuf[64];
            swprintf_s(tBuf, L"%d", dispTotal);
            DirectX::XMVECTORF32 tCol = { 1.0f, 0.85f, 0.0f, tsA };
            float numY = scoreY + 25.0f;

            if (m_fontLarge)
            {
                DirectX::XMVECTOR ts = m_fontLarge->MeasureString(tBuf);
                float tw = DirectX::XMVectorGetX(ts);
                m_fontLarge->DrawString(m_spriteBatch.get(), tBuf,
                    DirectX::XMFLOAT2(rightCX - tw * 0.5f, numY), tCol);
            }
            else
            {
                DirectX::XMVECTOR ts = m_font->MeasureString(tBuf);
                float tw = DirectX::XMVectorGetX(ts);
                m_font->DrawString(m_spriteBatch.get(), tBuf,
                    DirectX::XMFLOAT2(rightCX - tw * 0.5f, numY), tCol);
            }
        }

        // --- �p�[�e�B�N���iA/S�����N�ŉE�p�l�����ӂɉ΂̕��j---
        if (timer > 3.5f && m_gameOverRank >= 2 && m_whitePixel)
        {
            float particleAlpha = min((timer - 3.5f) * 2.0f, 1.0f);
            int numParticles = (m_gameOverRank == 3) ? 30 : 15;

            for (int i = 0; i < numParticles; i++)
            {
                float seed1 = sinf((float)i * 127.1f + timer * 0.7f) * 0.5f + 0.5f;
                float seed2 = cosf((float)i * 311.7f + timer * 0.5f) * 0.5f + 0.5f;
                float seed3 = sinf((float)i * 269.5f + timer * 1.3f) * 0.5f + 0.5f;

                float px = rightCX + (seed1 - 0.5f) * 500.0f;
                float py = panelTop + seed2 * (panelBot - panelTop);
                float pSize = 2.0f + seed3 * 4.0f;

                float flickerAlpha = particleAlpha * (0.3f + seed3 * 0.7f);
                DirectX::XMVECTORF32 pColor = {
                    0.9f + seed1 * 0.1f,
                    0.3f + seed2 * 0.4f,
                    0.0f,
                    flickerAlpha
                };

                RECT pRect = {
                    (LONG)(px - pSize * 0.5f), (LONG)(py - pSize * 0.5f),
                    (LONG)(px + pSize * 0.5f), (LONG)(py + pSize * 0.5f)
                };
                m_spriteBatch->Draw(m_whitePixel.Get(), pRect, pColor);
            }
        }

        // --- Press R to Restart�i�E�p�l�����j---
        if (timer > 4.0f && m_rankingSaved)
        {
            float restartT = min((timer - 4.0f) / 0.3f, 1.0f);
            float restartAlpha = Easing::OutCubic(restartT);
            float blink = (sinf(timer * 3.0f) + 1.0f) * 0.5f;
            float finalAlpha = restartAlpha * (0.4f + blink * 0.6f);

            DirectX::XMVECTORF32 restartColor = { 0.6f, 0.6f, 0.6f, finalAlpha };
            DirectX::XMVECTOR restartSize = m_font->MeasureString(L"Press R to Restart");
            float restartW = DirectX::XMVectorGetX(restartSize);
            m_font->DrawString(m_spriteBatch.get(), L"Press R to Restart",
                DirectX::XMFLOAT2(rightCX - restartW * 0.5f, panelBot + 15.0f), restartColor);
        }

        // --- L: RANKING �ē��e�L�X�g ---
        if (timer > 4.0f && m_rankingSaved)
        {
            float lT = min((timer - 4.0f) / 0.3f, 1.0f);
            float lAlpha = Easing::OutCubic(lT);
            float lBlink = (sinf(timer * 2.5f) + 1.0f) * 0.5f;
            float lFinal = lAlpha * (0.3f + lBlink * 0.5f);

            DirectX::XMVECTORF32 lColor = { 0.8f, 0.6f, 0.2f, lFinal };
            DirectX::XMVECTOR lSize = m_font->MeasureString(L"L: RANKING");
            float lW = DirectX::XMVectorGetX(lSize);
            m_font->DrawString(m_spriteBatch.get(), L"L: RANKING",
                DirectX::XMFLOAT2(rightCX - lW * 0.5f, panelBot + 40.0f), lColor);
        }

        // =============================================
        // ���O���� UI
        // =============================================
        if (m_nameInputActive)
        {
            float inputY = panelBot + 75.0f;
            float inputCenterX = rightCX;

            // --- ���̓{�b�N�X�w�i ---
            RECT inputBg = {
                (LONG)(inputCenterX - 140), (LONG)(inputY - 5),
                (LONG)(inputCenterX + 140), (LONG)(inputY + 35)
            };
            DirectX::XMVECTORF32 bgColor = { 0.0f, 0.0f, 0.0f, 0.7f };
            m_spriteBatch->Draw(m_whitePixel.Get(), inputBg, bgColor);

            // --- ���̓{�b�N�X�g ---
            float borderPulse = sinf(timer * 3.0f) * 0.3f + 0.7f;
            DirectX::XMVECTORF32 borderColor = { 0.9f, 0.6f, 0.1f, borderPulse };

            // ��g
            RECT borderTop = { inputBg.left, inputBg.top, inputBg.right, inputBg.top + 2 };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderTop, borderColor);
            // ���g
            RECT borderBot = { inputBg.left, inputBg.bottom - 2, inputBg.right, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderBot, borderColor);
            // ���g
            RECT borderL = { inputBg.left, inputBg.top, inputBg.left + 2, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderL, borderColor);
            // �E�g
            RECT borderR = { inputBg.right - 2, inputBg.top, inputBg.right, inputBg.bottom };
            m_spriteBatch->Draw(m_whitePixel.Get(), borderR, borderColor);

            // --- "ENTER YOUR NAME" ���x�� ---
            DirectX::XMVECTORF32 labelColor = { 1.0f, 0.8f, 0.3f, 1.0f };
            DirectX::XMVECTOR labelSize = m_font->MeasureString(L"ENTER YOUR NAME");
            float labelW = DirectX::XMVectorGetX(labelSize);
            m_font->DrawString(m_spriteBatch.get(), L"ENTER YOUR NAME",
                DirectX::XMFLOAT2(inputCenterX - labelW * 0.5f, inputY - 28.0f), labelColor);

            // --- ���̓e�L�X�g�\�� ---
            wchar_t wideNameBuf[32] = {};
            for (int c = 0; c < m_nameLength && c < 15; c++)
                wideNameBuf[c] = (wchar_t)m_nameBuffer[c];
            wideNameBuf[m_nameLength] = L'\0';

            // �_�ŃJ�[�\����//
            float cursorBlink = sinf(timer * 6.0f);
            if (cursorBlink > 0.0f && m_nameLength < 15)
            {
                wideNameBuf[m_nameLength] = L'_';
                wideNameBuf[m_nameLength + 1] = L'\0';
            }

            DirectX::XMVECTORF32 nameColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            DirectX::XMVECTOR nameSize = m_font->MeasureString(wideNameBuf);
            float nameW = DirectX::XMVectorGetX(nameSize);
            m_font->DrawString(m_spriteBatch.get(), wideNameBuf,
                DirectX::XMFLOAT2(inputCenterX - nameW * 0.5f, inputY + 3.0f), nameColor);

            // --- "PRESS ENTER TO CONFIRM" ---
            DirectX::XMVECTORF32 hintColor = { 0.6f, 0.6f, 0.5f, 0.6f + sinf(timer * 2.0f) * 0.2f };
            DirectX::XMVECTOR hintSize = m_font->MeasureString(L"ENTER: OK    BACKSPACE: DELETE");
            float hintW = DirectX::XMVectorGetX(hintSize);
            m_font->DrawString(m_spriteBatch.get(), L"ENTER: OK    BACKSPACE: DELETE",
                DirectX::XMFLOAT2(inputCenterX - hintW * 0.5f, inputY + 42.0f), hintColor);
        }

        m_spriteBatch->End();
    }
}

// ============================================================
//  RenderFade - ���t�F�[�h�I�[�o�[���C�̕`��
//
//  �y�d�g�݁z��ʑS�̂𕢂������l�p�`��m_fadeAlpha�œ��߁B
//  PrimitiveBatch�ŎO�p�`2�� = �l�p�`1���B
//  �`����VertexColorEnabled��true�ɖ߂��̂��|�C���g�B
// ============================================================
void Game::RenderFade()
{
    if (m_fadeAlpha <= 0.0f)
        return;

    // �t�F�[�h�p��2D�`��ݒ�
    auto context = m_d3dContext.Get();

    // 2D�p�̒P�ʍs��
    DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
        (float)m_outputWidth, (float)m_outputHeight, 0.1f, 10.0f);

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());

    // ���_�J���[�𖳌������ĒP�F�`�惂�[�h��
    m_effect->SetVertexColorEnabled(false);
    DirectX::XMVECTOR diffuseColor = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    m_effect->SetDiffuseColor(diffuseColor);

    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // �t���X�N���[���̎l�p�`��`��
    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    DirectX::XMFLOAT4 fadeColor(0.0f, 0.0f, 0.0f, m_fadeAlpha);
    float halfWidth = m_outputWidth * 0.5f;
    float halfHeight = m_outputHeight * 0.5f;

    // ��ʑS�̂𕢂��l�p�`
    DirectX::VertexPositionColor v1(DirectX::XMFLOAT3(-halfWidth, -halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v2(DirectX::XMFLOAT3(-halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v3(DirectX::XMFLOAT3(halfWidth, halfHeight, 1.0f), fadeColor);
    DirectX::VertexPositionColor v4(DirectX::XMFLOAT3(halfWidth, -halfHeight, 1.0f), fadeColor);

    // �O�p�`2�Ŏl�p�`���\��
    primitiveBatch->DrawTriangle(v1, v2, v3);
    primitiveBatch->DrawTriangle(v1, v3, v4);

    primitiveBatch->End();

    // ���_�J���[���ēx�L����
    m_effect->SetVertexColorEnabled(true);
}

// ============================================================
//  RenderRanking - �����L���O��ʂ̕`��
//
//  �y�`��\���z
//  Layer 0: �ԍ��r�l�b�g�w�i
//  Layer 1: SpriteBatch UI
//    - �������i�㉺�j + "LEADERBOARD"�^�C�g���i�o�E���X+�O���[�j
//    - �w�b�_�[�s�i#, NAME, SCORE, WAVE, KILLS�j
//    - �G���g���[�ő�10�s�i�X���C�h�C�� + �J�E���g�A�b�v�j
//    - NEW!�}�[�N�i�V�L�^�G���g���[�ɋ��F�p���X�j
//    - �t�b�^�[����ē�
//
//  �y���P�z�C�[�W���O�֐���Easing���O��Ԃɓ���A
//  DrawTextWithShadow�����_�͎c���i���̊֐����ł̂ݎg�p�j
// ============================================================
void Game::RenderRanking()
{
    // =============================================
    // �C�[�W���O�֐�
    // =============================================




    // �e�L�X�g�ɉe��`���w���p�[
    // �{�̂�2px�E���ɍ����e��`���Ă���{�̂�`��
    auto DrawTextWithShadow = [&](DirectX::SpriteFont* font,
        const wchar_t* text, DirectX::XMFLOAT2 pos,
        DirectX::XMVECTORF32 color)
        {
            // �e�i���E�������j
            DirectX::XMVECTORF32 shadowColor = { 0.0f, 0.0f, 0.0f, color.f[3] * 0.8f };
            font->DrawString(m_spriteBatch.get(), text,
                DirectX::XMFLOAT2(pos.x + 2.0f, pos.y + 2.0f), shadowColor);
            // �{��
            font->DrawString(m_spriteBatch.get(), text, pos, color);
        };

    float timer = m_rankingTimer;
    float screenW = (float)m_outputWidth;
    float screenH = (float)m_outputHeight;
    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    auto context = m_d3dContext.Get();

    // =============================================
    // Layer 0: �w�i�i���Z���r�l�b�g�j
    // =============================================
    {
        float overlayT = (std::min)(timer / 0.8f, 1.0f);
        float overlayAlpha = Easing::OutQuad(overlayT) * 0.95f;  // // 0.92��0.95 ���Â�

        DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(
            screenW, screenH, 0.1f, 10.0f);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(DirectX::XMMatrixIdentity());
        m_effect->SetVertexColorEnabled(true);
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        auto primBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        primBatch->Begin();

        float hw = screenW * 0.5f;
        float hh = screenH * 0.5f;

        // // ���������Ȃ�Â����ĕ����Ƃ̑Δ����������
        DirectX::XMFLOAT4 centerColor(0.06f, 0.0f, 0.02f, overlayAlpha * 0.85f);
        DirectX::XMFLOAT4 edgeColor(0.01f, 0.0f, 0.0f, overlayAlpha);

        DirectX::VertexPositionColor vc(DirectX::XMFLOAT3(0, 0, 1.0f), centerColor);
        DirectX::VertexPositionColor vTL(DirectX::XMFLOAT3(-hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vTR(DirectX::XMFLOAT3(hw, hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBR(DirectX::XMFLOAT3(hw, -hh, 1.0f), edgeColor);
        DirectX::VertexPositionColor vBL(DirectX::XMFLOAT3(-hw, -hh, 1.0f), edgeColor);

        primBatch->DrawTriangle(vc, vTL, vTR);
        primBatch->DrawTriangle(vc, vTR, vBR);
        primBatch->DrawTriangle(vc, vBR, vBL);
        primBatch->DrawTriangle(vc, vBL, vTL);

        primBatch->End();
    }

    // =============================================
    // Layer 1: SpriteBatch�őSUI�`��
    // =============================================
    if (!m_spriteBatch || !m_states || !m_font || !m_whitePixel)
        return;

    m_spriteBatch->Begin(
        DirectX::SpriteSortMode_Deferred,
        m_states->AlphaBlend(),
        nullptr,
        m_states->DepthNone()
    );

    const auto& entries = m_rankingSystem->GetEntries();
    int entryCount = (int)entries.size();

    // =============================================
    // �������i��j
    // =============================================
    if (timer > 0.2f)
    {
        float lineT = (std::min)((timer - 0.2f) / 0.5f, 1.0f);
        float lineProgress = Easing::OutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;  // // 280��320 ��������
        float lineY = 55.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 2)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 2)
        };

        // // �F����薾�邢�ԋ���
        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.95f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

        RECT diamond = {
            (LONG)(centerX - 5), (LONG)(lineY - 4),
            (LONG)(centerX + 5), (LONG)(lineY + 6)
        };
        m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
    }

    // =============================================
    // �^�C�g�� "LEADERBOARD" ? �o�E���X�ŗ����Ă���
    // =============================================
    if (timer > 0.3f && m_fontLarge)
    {
        float titleT = (std::min)((timer - 0.3f) / 0.6f, 1.0f);
        float bounceT = Easing::OutBounce(titleT);
        float titleAlpha = (std::min)((timer - 0.3f) * 4.0f, 1.0f);

        float slideY = (1.0f - bounceT) * -60.0f;
        float titleY = 65.0f + slideY;

        DirectX::XMVECTOR titleSize = m_fontLarge->MeasureString(L"LEADERBOARD");
        float titleW = DirectX::XMVectorGetX(titleSize);
        float titleX = centerX - titleW * 0.5f;

        // // �O���[�F����薾�邭
        float glowAlpha = titleAlpha * 0.3f;
        DirectX::XMVECTORF32 glowColor = { 1.0f, 0.3f, 0.0f, glowAlpha };
        for (int dx = -2; dx <= 2; dx += 4)
        {
            for (int dy = -2; dy <= 2; dy += 4)
            {
                m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
                    DirectX::XMFLOAT2(titleX + dx, titleY + dy), glowColor);
            }
        }

        // // �e��//
        DirectX::XMVECTORF32 shadowColor = { 0.0f, 0.0f, 0.0f, titleAlpha * 0.7f };
        m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
            DirectX::XMFLOAT2(titleX + 3, titleY + 3), shadowColor);

        // // �{�̐F�𖾂邢�S�[���h��
        DirectX::XMVECTORF32 titleColor = { 1.0f, 0.9f, 0.75f, titleAlpha };
        m_fontLarge->DrawString(m_spriteBatch.get(), L"LEADERBOARD",
            DirectX::XMFLOAT2(titleX, titleY), titleColor);
    }

    // =============================================
    // �������i�^�C�g���̉��j
    // =============================================
    if (timer > 0.4f)
    {
        float lineT = (std::min)((timer - 0.4f) / 0.5f, 1.0f);
        float lineProgress = Easing::OutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;
        float lineY = 115.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 1)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 1)
        };

        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.8f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);
    }

    // =============================================
    // �w�b�_�[�s: #  NAME  SCORE  WAVE  KILLS
    // =============================================
    if (timer > 0.6f)
    {
        float headerAlpha = (std::min)((timer - 0.6f) * 3.0f, 1.0f);
        // // �w�b�_�[�F����薾�邢���F�Ɂi�����Ɠ��e�ƕ���킵���j
        DirectX::XMVECTORF32 headerColor = { 1.0f, 0.75f, 0.35f, headerAlpha * 0.9f };

        float headerY = 130.0f;

        // // ��̔z�u��ύX�iNAME���//�j
        float colNum = centerX - 310.0f;   // #
        float colName = centerX - 250.0f;   // NAME  //�V�K
        float colScore = centerX - 60.0f;    // SCORE
        float colWave = centerX + 100.0f;   // WAVE
        float colKills = centerX + 210.0f;   // KILLS

        DrawTextWithShadow(m_font.get(), L"#",
            DirectX::XMFLOAT2(colNum, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"NAME",
            DirectX::XMFLOAT2(colName, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"SCORE",
            DirectX::XMFLOAT2(colScore, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"WAVE",
            DirectX::XMFLOAT2(colWave, headerY), headerColor);
        DrawTextWithShadow(m_font.get(), L"KILLS",
            DirectX::XMFLOAT2(colKills, headerY), headerColor);
    }

    // =============================================
    // �e�G���g���[
    // =============================================
    float entryStartTime = 0.8f;
    float entryDelay = 0.12f;
    float entryBaseY = 160.0f;
    float entryHeight = 42.0f;

    // ���X���W�i�w�b�_�[�Ɠ����j
    float colNum = centerX - 310.0f;
    float colName = centerX - 250.0f;
    float colScore = centerX - 60.0f;
    float colWave = centerX + 100.0f;
    float colKills = centerX + 210.0f;

    for (int i = 0; i < entryCount && i < 10; i++)
    {
        float startT = entryStartTime + i * entryDelay;

        if (timer < startT)
            continue;

        // --- �X���C�h�C���A�j���[�V���� ---
        float slideT = (std::min)((timer - startT) / 0.5f, 1.0f);
        float slideProgress = Easing::OutCubic(slideT);

        float slideX = (1.0f - slideProgress) * -300.0f;
        float alpha = slideProgress;

        float rowY = entryBaseY + i * entryHeight;

        bool isNewRecord = (i == m_newRecordRank);

        // --- �w�i�o�[ ---
        {
            float barAlpha = alpha * 0.25f;  // // 0.15��0.25 �o�[�����ڗ�������

            if (isNewRecord)
            {
                float pulse = sinf(timer * 4.0f) * 0.5f + 0.5f;
                barAlpha = alpha * (0.3f + pulse * 0.2f);
            }

            if (i == 0)
                barAlpha = alpha * 0.3f;

            RECT bar = {
                (LONG)(centerX - 330.0f + slideX), (LONG)(rowY - 2),
                (LONG)(centerX + 330.0f + slideX), (LONG)(rowY + entryHeight - 8)
            };

            DirectX::XMVECTORF32 barColor;
            if (isNewRecord)
                barColor = { 1.0f, 0.8f, 0.1f, barAlpha };
            else if (i == 0)
                barColor = { 1.0f, 0.75f, 0.2f, barAlpha };
            else if (i == 1)
                barColor = { 0.75f, 0.75f, 0.85f, barAlpha };
            else if (i == 2)
                barColor = { 0.75f, 0.5f, 0.2f, barAlpha };
            else
                barColor = { 0.4f, 0.2f, 0.15f, barAlpha };

            m_spriteBatch->Draw(m_whitePixel.Get(), bar, barColor);
        }

        // --- ���ʂ̐F�i �S�̓I�ɖ��邭�j ---
        DirectX::XMVECTORF32 rankNumColor;
        if (i == 0)
            rankNumColor = { 1.0f, 0.9f, 0.3f, alpha };      // 1��: ���邢��
        else if (i == 1)
            rankNumColor = { 0.9f, 0.9f, 1.0f, alpha };       // 2��: ���邢��
        else if (i == 2)
            rankNumColor = { 1.0f, 0.65f, 0.3f, alpha };      // 3��: ���邢��
        else
            rankNumColor = { 0.85f, 0.8f, 0.7f, alpha };      //  ��: ���邢�x�[�W��

        if (isNewRecord)
        {
            float pulse = sinf(timer * 5.0f) * 0.5f + 0.5f;
            rankNumColor = { 1.0f, 0.8f + pulse * 0.2f, 0.2f + pulse * 0.3f, alpha };
        }

        // --- �e�L�X�g�F�i ���邭�j ---
        DirectX::XMVECTORF32 textColor;
        if (isNewRecord)
        {
            float pulse = sinf(timer * 5.0f) * 0.5f + 0.5f;
            textColor = { 1.0f, 0.9f + pulse * 0.1f, 0.6f + pulse * 0.3f, alpha };
        }
        else
        {
            textColor = { 1.0f, 0.95f, 0.85f, alpha };  // // ���邢�N���[���F
        }

        const RankingEntry& e = entries[i];

        // --- ���ʔԍ� ---
        wchar_t rankBuf[16];
        swprintf_s(rankBuf, L"%d.", i + 1);
        DrawTextWithShadow(m_font.get(), rankBuf,
            DirectX::XMFLOAT2(colNum + slideX, rowY), rankNumColor);

        // ---  ���O�i�V�K��j ---
        wchar_t nameBuf[32] = {};
        if (e.name[0] != '\0')
        {
            // char[16] �� wchar_t �ɕϊ�
            for (int c = 0; c < 15 && e.name[c] != '\0'; c++)
                nameBuf[c] = (wchar_t)e.name[c];
        }
        else
        {
            wcscpy_s(nameBuf, L"---");  // ���O�Ȃ��i���f�[�^�݊��j
        }

        // ���O�͏��ʂƓ����F�n���i1�ʋ�, 2�ʋ�...�j
        DrawTextWithShadow(m_font.get(), nameBuf,
            DirectX::XMFLOAT2(colName + slideX, rowY), rankNumColor);

        // --- �X�R�A�i�J�E���g�A�b�v���o�j ---
        float countUpT = (std::min)((timer - startT) / 1.0f, 1.0f);
        float easedCount = Easing::OutQuad(countUpT);
        int displayScore = (int)(e.score * easedCount);

        wchar_t scoreBuf[32];
        swprintf_s(scoreBuf, L"%d", displayScore);
        DrawTextWithShadow(m_font.get(), scoreBuf,
            DirectX::XMFLOAT2(colScore + slideX, rowY), textColor);

        // --- �E�F�[�u ---
        wchar_t waveBuf[16];
        swprintf_s(waveBuf, L"W%d", e.wave);
        DrawTextWithShadow(m_font.get(), waveBuf,
            DirectX::XMFLOAT2(colWave + slideX, rowY), textColor);

        // --- �L���� ---
        wchar_t killBuf[16];
        swprintf_s(killBuf, L"%d", e.kills);
        DrawTextWithShadow(m_font.get(), killBuf,
            DirectX::XMFLOAT2(colKills + slideX, rowY), textColor);

        // --- �V�L�^�}�[�N ---
        if (isNewRecord && slideT > 0.5f)
        {
            float newAlpha = (std::min)((slideT - 0.5f) * 4.0f, 1.0f);
            float pulse = sinf(timer * 6.0f) * 0.5f + 0.5f;

            DirectX::XMVECTORF32 newColor = {
                1.0f, 0.85f + pulse * 0.15f, 0.0f, newAlpha
            };

            DrawTextWithShadow(m_font.get(), L"NEW!",
                DirectX::XMFLOAT2(colKills + 70.0f + slideX, rowY), newColor);
        }
    }

    // =============================================
    // �G���g���[0��
    // =============================================
    if (entryCount == 0 && timer > 1.0f)
    {
        float emptyAlpha = (std::min)((timer - 1.0f) * 2.0f, 1.0f);
        DirectX::XMVECTORF32 emptyColor = { 0.8f, 0.7f, 0.5f, emptyAlpha };

        const wchar_t* emptyText = L"NO RECORDS YET";
        DirectX::XMVECTOR emptySize = m_font->MeasureString(emptyText);
        float emptyW = DirectX::XMVectorGetX(emptySize);

        DrawTextWithShadow(m_font.get(), emptyText,
            DirectX::XMFLOAT2(centerX - emptyW * 0.5f, centerY - 20.0f), emptyColor);
    }

    // =============================================
    // �������i�����j
    // =============================================
    if (timer > 1.5f)
    {
        float lineT = (std::min)((timer - 1.5f) / 0.5f, 1.0f);
        float lineProgress = Easing::OutBack(lineT);
        float lineHalfW = 320.0f * lineProgress;
        float lineY = entryBaseY + 10 * entryHeight + 10.0f;
        if (lineY > screenH - 80.0f) lineY = screenH - 80.0f;

        RECT leftLine = {
            (LONG)(centerX - lineHalfW), (LONG)lineY,
            (LONG)centerX, (LONG)(lineY + 1)
        };
        RECT rightLine = {
            (LONG)centerX, (LONG)lineY,
            (LONG)(centerX + lineHalfW), (LONG)(lineY + 1)
        };

        DirectX::XMVECTORF32 lineColor = { 0.9f, 0.3f, 0.05f, 0.6f * lineT };
        m_spriteBatch->Draw(m_whitePixel.Get(), leftLine, lineColor);
        m_spriteBatch->Draw(m_whitePixel.Get(), rightLine, lineColor);

        RECT diamond = {
            (LONG)(centerX - 3), (LONG)(lineY - 2),
            (LONG)(centerX + 3), (LONG)(lineY + 3)
        };
        m_spriteBatch->Draw(m_whitePixel.Get(), diamond, lineColor);
    }

    // =============================================
    // �t�b�^�[: ����ē�
    // =============================================
    if (timer > 2.0f)
    {
        float footerAlpha = (std::min)((timer - 2.0f) * 2.0f, 1.0f);
        float pulse = sinf(timer * 2.0f) * 0.15f + 0.85f;

        //  �t�b�^�[�F�������Ɩ��邭
        DirectX::XMVECTORF32 footerColor = { 0.85f, 0.75f, 0.55f, footerAlpha * pulse };

        float footerY = screenH - 50.0f;

        const wchar_t* backText = L"ESC: BACK     R: RESTART";
        DirectX::XMVECTOR backSize = m_font->MeasureString(backText);
        float backW = DirectX::XMVectorGetX(backSize);

        DrawTextWithShadow(m_font.get(), backText,
            DirectX::XMFLOAT2(centerX - backW * 0.5f, footerY), footerColor);
    }

    m_spriteBatch->End();
}