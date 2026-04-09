// ============================================================
//  GameRender.cpp
//  3D�`��p�C�v���C���i�G/����/��/�p�[�e�B�N��/�w���X�s�b�N�A�b�v�j
//
//  �y�Ӗ��z
//  - ���C����Render()�f�B�X�p�b�`���i�V�[���ʂɌĂѕ����j
//  - RenderPlaying(): �v���C���̑S3D/2D�`��̓���
//  - �G�̕`��i�C���X�^���X�`�� + �P�̕`��j
//  - ����/����FPS���_�`��
//  - �r���{�[�h/�p�[�e�B�N���`��
//  - ����X�|�[��/�w���X�s�b�N�A�b�v�̕`��
//  - ���C��������i����X�|�[���̃C���^���N�V�����p�j
//
//  �y�݌v�Ӑ}�z
//  �u��ʂ�3D�I�u�W�F�N�g��`���v������S�ďW��B
//  GameHUD.cpp�i2D UI�j�Ƃ͖��m�ɕ����B
//  �`�揇���̊Ǘ������̃t�@�C���̍ŏd�v�Ӗ��B
//
//  �y�`�揇���iRenderPlaying���j�z
//  1. �}�b�v�i�s�����j
//  2. �G�i�C���X�^���X�`��j
//  3. �ؒf�s�[�X / ����
//  4. ����X�|�[�� / �w���X�s�b�N�A�b�v
//  5. ����iFPS���_�A�[�x�N���A��j
//  6. ��
//  7. �p�[�e�B�N���i���Z�u�����h�j
//  8. �r���{�[�h
//  9. HUD�i2D�A�[�xOFF�j
//  10. �f�o�b�O�\��
// ============================================================

#include "Game.h"

using namespace DirectX;


// ============================================================
//  Render - ���C���`��f�B�X�p�b�`��
//
//  �y�Ăяo�����zTick() ���疈�t���[��1��
//  �y�����z���݂�GameState�ɉ����ăV�[���ʂ̕`����Ăѕ�����
//  PLAYING�ȊO�̃V�[����GameScene.cpp�ɈϏ��ς�
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
// �y�Q�[�����z�̕`�揈��
// =================================================================

// ============================================================
//  RenderPlaying - �v���C���̑S�`��𓝊�
//
//  �y�����������d�v�I�z�s�����������������Z�u�����h�̏��B
//  �[�x�e�X�g�̊֌W�ŁA�`�揇���ԈႦ��Ɠ��߂�����B
//
//  �y��580�s���闝�R�z�J�����s��̍\�z�A�e�V�X�e���ւ�
//  �`��Ϗ��A�u�����h��Ԃ̐؂�ւ����S�Ă����ɏW��B
//  �����I�ɂ�RenderPass�N���X�ɕ������ׂ��B
// ============================================================
void Game::RenderPlaying()
{
    // �f�o�b�O���O
   /* char debugRender[256];
    sprintf_s(debugRender, "[RENDER] DOFActive=%d, Intensity=%.2f, OffscreenRTV=%p\n",
        m_gloryKillDOFActive, m_gloryKillDOFIntensity, m_offscreenRTV.Get());
    //OutputDebugStringA(debugRender);*/

    if (m_gloryKillDOFActive && m_offscreenRTV)
    {
        // ============================================================
        // === �O���[���[�L�����F�u���[���� ===
        // ============================================================

        // �I�t�X�N���[���ɒʏ�`��
        ID3D11RenderTargetView* offscreenRTV = m_offscreenRTV.Get();
        m_d3dContext->OMSetRenderTargets(1, &offscreenRTV, m_offscreenDepthStencilView.Get());  // // �ύX

        // �I�t�X�N���[�����N���A
        DirectX::XMVECTORF32 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_d3dContext->ClearRenderTargetView(m_offscreenRTV.Get(), clearColor);
        m_d3dContext->ClearDepthStencilView(
            m_offscreenDepthStencilView.Get(),  // // �ύX
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0
        );

        //  �[�x�e�X�g�Ɛ[�x�������݂�L����
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        //  �r���[�|�[�g�ݒ�
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_outputWidth);
        viewport.Height = static_cast<float>(m_outputHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_d3dContext->RSSetViewports(1, &viewport);

        // �J�����ݒ�
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        playerPos.y += m_player->GetLandingCameraOffset();  // ���n���o�F�J�����𒾂܂���

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

        //  �[�x�e�X�g�Ɛ[�x�������݂�L����
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        // �}�b�v�`��
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }

        DrawNavGridDebug(viewMatrix, projectionMatrix);
        DrawSlicedPieces(viewMatrix, projectionMatrix);

        //  �n�ʂ̑ۂ�`��
           /* if (m_furRenderer && m_furReady)
            {
                m_furRenderer->DrawGroundMoss(
                    m_d3dContext.Get(),
                    viewMatrix,
                    projectionMatrix,
                    m_accumulatedAnimTime
                );
            }*/

            //  �[�x�e�X�g�Ɛ[�x�������݂�L����
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        //// ����X�|�[���̃e�N�X�`���`��
        //if (m_weaponSpawnSystem)
        //{
        //    m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        //}
        //  �[�x�e�X�g�Ɛ[�x�������݂�L����
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        // 3D�`��i�D�揇�ʏ��j
        //DrawParticles();
        //  �[�x�e�X�g�Ɛ[�x�������݂�L����
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

        DrawEnemies(viewMatrix, projectionMatrix, true);// true�Ń^�[�Q�b�g�G���X�L�b�v

        DrawKeyPrompt(viewMatrix, projectionMatrix);

        //  �[�x�e�X�g�Ɛ[�x�������݂�L����
        m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);



        //DrawBillboard();
        DrawWeapon();
        DrawShield();



        //DrawWeaponSpawns();

        DrawGibs(viewMatrix, projectionMatrix);

        // �O���[���[�L���r�E�i�C�t�`��iFBX���f���Łj
        if ((m_gloryKillArmAnimActive || m_debugShowGloryKillArm) && m_gloryKillArmModel)
        {
            DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
            forward = DirectX::XMVector3Normalize(forward);

            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
            right = DirectX::XMVector3Normalize(right);
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // === �A�[���̈ʒu�v�Z�i�����ϐ��g�p�j ===
            DirectX::XMVECTOR armWorldPos = cameraPosition;
            armWorldPos += forward * m_gloryKillArmOffset.x;  // �O��
            armWorldPos += up * m_gloryKillArmOffset.y;       // �㉺
            armWorldPos += right * m_gloryKillArmOffset.z;    // ���E

            float cameraYaw = atan2f(
                DirectX::XMVectorGetX(forward),
                DirectX::XMVectorGetZ(forward)
            );

            // === �A�[���̃��[���h�s�� ===
            DirectX::XMMATRIX armScale = DirectX::XMMatrixScaling(
                m_gloryKillArmScale, m_gloryKillArmScale, m_gloryKillArmScale);

            // �x�[�X��]�i���f���𐳂��������ɕ␳�j
            DirectX::XMMATRIX baseRotX = DirectX::XMMatrixRotationX(m_gloryKillArmBaseRot.x);
            DirectX::XMMATRIX baseRotY = DirectX::XMMatrixRotationY(m_gloryKillArmBaseRot.y);
            DirectX::XMMATRIX baseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmBaseRot.z);
            DirectX::XMMATRIX baseRot = baseRotX * baseRotY * baseRotZ;

            // �A�j���[�V������]�i�O���[���[�L���̓����j
            DirectX::XMMATRIX animRotX = DirectX::XMMatrixRotationX(m_gloryKillArmAnimRot.x);
            DirectX::XMMATRIX animRotY = DirectX::XMMatrixRotationY(m_gloryKillArmAnimRot.y);
            DirectX::XMMATRIX animRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmAnimRot.z);
            DirectX::XMMATRIX animRot = animRotX * animRotY * animRotZ;

            // �J�����Ǐ]��]
            DirectX::XMMATRIX cameraRot = DirectX::XMMatrixRotationY(cameraYaw);
            DirectX::XMMATRIX armTrans = DirectX::XMMatrixTranslationFromVector(armWorldPos);

            // �����F�X�P�[�� �� �x�[�X�␳ �� �A�j���[�V���� �� �J�����Ǐ] �� �ړ�
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
            // === �i�C�t�`�� ===
            // ============================================
            if (m_gloryKillKnifeModel)
            {
                // �I�t�Z�b�g�x�N�g�����쐬�i���[�J����ԁj
                DirectX::XMVECTOR offset = DirectX::XMVectorSet(
                    m_gloryKillKnifeOffset.z,   // X = Left/Right
                    m_gloryKillKnifeOffset.y,   // Y = Up/Down  
                    m_gloryKillKnifeOffset.x,   // Z = Forward
                    0.0f
                );

                // �A�j���[�V������] + �J������]�ŃI�t�Z�b�g��ϊ�
                DirectX::XMMATRIX offsetRotation = animRot * cameraRot;
                DirectX::XMVECTOR rotatedOffset = DirectX::XMVector3TransformNormal(offset, offsetRotation);

                // �r�̈ʒu�ɕϊ���̃I�t�Z�b�g��������
                DirectX::XMVECTOR knifeWorldPos = DirectX::XMVectorAdd(armWorldPos, rotatedOffset);
                // �i�C�t�̃X�P�[��
                DirectX::XMMATRIX knifeScale = DirectX::XMMatrixScaling(
                    m_gloryKillKnifeScale, m_gloryKillKnifeScale, m_gloryKillKnifeScale);

                // �i�C�t�̃x�[�X��]
                DirectX::XMMATRIX knifeBaseRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeBaseRot.x);
                DirectX::XMMATRIX knifeBaseRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeBaseRot.y);
                DirectX::XMMATRIX knifeBaseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeBaseRot.z);
                DirectX::XMMATRIX knifeBaseRot = knifeBaseRotX * knifeBaseRotY * knifeBaseRotZ;

                // �i�C�t�̃A�j���[�V������]
                DirectX::XMMATRIX knifeAnimRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeAnimRot.x);
                DirectX::XMMATRIX knifeAnimRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeAnimRot.y);
                DirectX::XMMATRIX knifeAnimRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeAnimRot.z);
                DirectX::XMMATRIX knifeAnimRot = knifeAnimRotX * knifeAnimRotY * knifeAnimRotZ;

                // �i�C�t�̈ړ��s��
                DirectX::XMMATRIX knifeTrans = DirectX::XMMatrixTranslationFromVector(knifeWorldPos);

                // ����
                DirectX::XMMATRIX knifeWorld = knifeScale * knifeBaseRot * knifeAnimRot * cameraRot * knifeTrans;

                m_gloryKillKnifeModel->Draw(
                    m_d3dContext.Get(),
                    knifeWorld,
                    viewMatrix,
                    projectionMatrix,
                    DirectX::Colors::White
                );
            }

            // === �O���[���[�L���^�[�Q�b�g�̓G���Ō�ɕ`��i�r�̌��ɕ\���j ===
            if (m_gloryKillTargetEnemy)
            {
                DrawSingleEnemy(*m_gloryKillTargetEnemy, viewMatrix, projectionMatrix);
            }

        }
        m_bloodSystem->DrawScreenBlood(m_outputWidth, m_outputHeight);
        DrawHealthPickups(viewMatrix, projectionMatrix);
        m_bloodSystem->DrawBloodDecals(viewMatrix, projectionMatrix, m_player->GetPosition());

        m_gpuParticles->Draw(viewMatrix, projectionMatrix, m_player->GetPosition());
        //  ���̃����_�����O: �V�[�����R�s�[���Ă���DrawFluid
        //m_d3dContext->CopyResource(m_sceneCopyTex.Get(), m_offscreenTexture.Get());
        /*m_gpuParticles->DrawFluid(viewMatrix, projectionMatrix, m_player->GetPosition(),
            nullptr, m_offscreenRTV.Get());*/

            // UI�`��i�I�t�X�N���[���ցj
        DrawUI();
        DrawScorePopups();
        RenderWaveBanner();
        RenderScoreHUD();

        //  �ŏI��ʂɃu���[��K�p���ĕ`��
        ID3D11RenderTargetView* finalRTV = m_renderTargetView.Get();
        m_d3dContext->OMSetRenderTargets(1, &finalRTV, nullptr);

        // �ŏI��ʂ��N���A
        m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

        // �u���[�p�����[�^��ݒ�
        BlurParams params;
        params.texelSize = DirectX::XMFLOAT2(1.0f / m_outputWidth, 1.0f / m_outputHeight);
        params.blurStrength = m_gloryKillDOFIntensity * 5.0f;
        params.focalDepth = m_gloryKillFocalDepth;  // // padding �� focalDepth�ɕύX

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        m_d3dContext->Map(m_blurConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &params, sizeof(BlurParams));
        m_d3dContext->Unmap(m_blurConstantBuffer.Get(), 0);

        // �V�F�[�_�[���\�[�X�ƃT���v���[��ݒ�(�[�x�e�N�X�`��//�j
        ID3D11ShaderResourceView* srvs[2] = {
        m_offscreenSRV.Get(),      // t0: �J���[
        m_offscreenDepthSRV.Get()  // t1: �[�x 
        };
        m_d3dContext->PSSetShaderResources(0, 2, srvs);

        ID3D11SamplerState* sampler = m_linearSampler.Get();
        m_d3dContext->PSSetSamplers(0, 1, &sampler);

        ID3D11Buffer* cb = m_blurConstantBuffer.Get();
        m_d3dContext->PSSetConstantBuffers(0, 1, &cb);

        // �����_�[�X�e�[�g�ݒ�
        m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
        m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
        m_d3dContext->RSSetState(m_states->CullNone());

        // �t���X�N���[���l�p�`��`��i�u���[�K�p�j
        DrawFullscreenQuad();

        // ���\�[�X���N���A�i�[�x���܂߂�j
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        m_d3dContext->PSSetShaderResources(0, 2, nullSRVs);
    }
    else
    {
        // ============================================================
        // === �ʏ펞�F�u���[�Ȃ� ===
        // ============================================================

        Clear();

        // �J�����ݒ�
        DirectX::XMFLOAT3 playerPos = m_player->GetPosition();
        DirectX::XMFLOAT3 playerRot = m_player->GetRotation();

        playerPos.y += m_player->GetLandingCameraOffset();  // ���n���o�F�J�����𒾂܂���

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

        // �}�b�v�`��
        if (m_mapSystem)
        {
            m_mapSystem->Draw(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        }

        DrawNavGridDebug(viewMatrix, projectionMatrix);
        DrawSlicedPieces(viewMatrix, projectionMatrix);

        ////  �n�ʂ̑ۂ�`��
        //    if (m_furRenderer && m_furReady)
        //    {
        //        m_furRenderer->DrawGroundMoss(
        //            m_d3dContext.Get(),
        //            viewMatrix,
        //            projectionMatrix,
        //            m_accumulatedAnimTime  // �o�ߎ��ԁi���̗h��p�j
        //        );
        //    }

        //// ����X�|�[���̃e�N�X�`���`��
        //if (m_weaponSpawnSystem)
        //{
        //    m_weaponSpawnSystem->DrawWallTextures(m_d3dContext.Get(), viewMatrix, projectionMatrix);
        //}

        // 3D�`��i�D�揇�ʏ��j
        //DrawParticles();
        DrawEnemies(viewMatrix, projectionMatrix);

        DrawKeyPrompt(viewMatrix, projectionMatrix);

        //DrawBillboard();
        DrawWeapon();
        DrawShield();



        //  �V�[���h�g���C���`��
        //  Effekseer�`��
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

        // �O���[���[�L���r�E�i�C�t�`��iFBX���f���Łj
        if ((m_gloryKillArmAnimActive || m_debugShowGloryKillArm) && m_gloryKillArmModel)
        {
            DirectX::XMVECTOR forward = DirectX::XMVectorSubtract(cameraTarget, cameraPosition);
            forward = DirectX::XMVector3Normalize(forward);

            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(worldUp, forward);
            right = DirectX::XMVector3Normalize(right);
            DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

            // === �A�[���̈ʒu�v�Z�i�����ϐ��g�p�j ===
            DirectX::XMVECTOR armWorldPos = cameraPosition;
            armWorldPos += forward * m_gloryKillArmOffset.x;  // �O��
            armWorldPos += up * m_gloryKillArmOffset.y;       // �㉺
            armWorldPos += right * m_gloryKillArmOffset.z;    // ���E

            float cameraYaw = atan2f(
                DirectX::XMVectorGetX(forward),
                DirectX::XMVectorGetZ(forward)
            );

            // === �A�[���̃��[���h�s�� ===
            DirectX::XMMATRIX armScale = DirectX::XMMatrixScaling(
                m_gloryKillArmScale, m_gloryKillArmScale, m_gloryKillArmScale);

            // �x�[�X��]�i���f���𐳂��������ɕ␳�j
            DirectX::XMMATRIX baseRotX = DirectX::XMMatrixRotationX(m_gloryKillArmBaseRot.x);
            DirectX::XMMATRIX baseRotY = DirectX::XMMatrixRotationY(m_gloryKillArmBaseRot.y);
            DirectX::XMMATRIX baseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmBaseRot.z);
            DirectX::XMMATRIX baseRot = baseRotX * baseRotY * baseRotZ;

            // �A�j���[�V������]�i�O���[���[�L���̓����j
            DirectX::XMMATRIX animRotX = DirectX::XMMatrixRotationX(m_gloryKillArmAnimRot.x);
            DirectX::XMMATRIX animRotY = DirectX::XMMatrixRotationY(m_gloryKillArmAnimRot.y);
            DirectX::XMMATRIX animRotZ = DirectX::XMMatrixRotationZ(m_gloryKillArmAnimRot.z);
            DirectX::XMMATRIX animRot = animRotX * animRotY * animRotZ;

            // �J�����Ǐ]��]
            DirectX::XMMATRIX cameraRot = DirectX::XMMatrixRotationY(cameraYaw);
            DirectX::XMMATRIX armTrans = DirectX::XMMatrixTranslationFromVector(armWorldPos);

            // �����F�X�P�[�� �� �x�[�X�␳ �� �A�j���[�V���� �� �J�����Ǐ] �� �ړ�
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
            // === �i�C�t�`�� ===
            // ============================================
            if (m_gloryKillKnifeModel)
            {
                // �I�t�Z�b�g�x�N�g�����쐬�i���[�J����ԁj
                DirectX::XMVECTOR offset = DirectX::XMVectorSet(
                    m_gloryKillKnifeOffset.z,   // X = Left/Right
                    m_gloryKillKnifeOffset.y,   // Y = Up/Down  
                    m_gloryKillKnifeOffset.x,   // Z = Forward
                    0.0f
                );

                // �A�j���[�V������] + �J������]�ŃI�t�Z�b�g��ϊ�
                DirectX::XMMATRIX offsetRotation = animRot * cameraRot;
                DirectX::XMVECTOR rotatedOffset = DirectX::XMVector3TransformNormal(offset, offsetRotation);

                // �r�̈ʒu�ɕϊ���̃I�t�Z�b�g��������
                DirectX::XMVECTOR knifeWorldPos = DirectX::XMVectorAdd(armWorldPos, rotatedOffset);

                // �i�C�t�̃X�P�[��
                DirectX::XMMATRIX knifeScale = DirectX::XMMatrixScaling(
                    m_gloryKillKnifeScale, m_gloryKillKnifeScale, m_gloryKillKnifeScale);

                // �i�C�t�̃x�[�X��]
                DirectX::XMMATRIX knifeBaseRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeBaseRot.x);
                DirectX::XMMATRIX knifeBaseRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeBaseRot.y);
                DirectX::XMMATRIX knifeBaseRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeBaseRot.z);
                DirectX::XMMATRIX knifeBaseRot = knifeBaseRotX * knifeBaseRotY * knifeBaseRotZ;

                // �i�C�t�̃A�j���[�V������]
                DirectX::XMMATRIX knifeAnimRotX = DirectX::XMMatrixRotationX(m_gloryKillKnifeAnimRot.x);
                DirectX::XMMATRIX knifeAnimRotY = DirectX::XMMatrixRotationY(m_gloryKillKnifeAnimRot.y);
                DirectX::XMMATRIX knifeAnimRotZ = DirectX::XMMatrixRotationZ(m_gloryKillKnifeAnimRot.z);
                DirectX::XMMATRIX knifeAnimRot = knifeAnimRotX * knifeAnimRotY * knifeAnimRotZ;

                // �i�C�t�̈ړ��s��
                DirectX::XMMATRIX knifeTrans = DirectX::XMMatrixTranslationFromVector(knifeWorldPos);

                // ����
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

        m_gpuParticles->Draw(viewMatrix, projectionMatrix, m_player->GetPosition());        //  ���̃����_�����O: �o�b�N�o�b�t�@���R�s�[���Ă���DrawFluid
        /* m_gpuParticles->DrawFluid(viewMatrix, projectionMatrix, m_player->GetPosition(),
             nullptr, m_renderTargetView.Get());*/

             // UI�`��
        DrawUI();
        DrawScorePopups();
        RenderWaveBanner();
        RenderScoreHUD();

    }


    //  �m�����l�b�g
    RenderLowHealthVignette();

    RenderClawDamage();

    //  �X�s�[�h���C��
    RenderSpeedLines();

    //  �_�b�V���I�[�o�[���C
    //RenderDashOverlay();

    //  �e�؂�x��
    RenderReloadWarning();

    // �f�o�b�O�`��
    DrawHitboxes();
    DrawBulletTracers();
    DrawDebugUI();
}

// �yUI�z���ׂĂ�UI��`�悷��

// ============================================================
//  DrawEnemies - �S�G�̕`��i�C���X�^���X�`��Ή��j
//
//  �y�`������z
//  - Normal/Runner/Tank: �C���X�^���X�o�b�t�@�ňꊇ�`��
//    �i�������f���̓G��1���DrawCall�őS�ĕ`��j
//  - MidBoss/Boss: �P�̕`��iDrawSingleEnemy�j
//  - �O���[���[�L���Ώ�: skipGloryKillTarget�Ő���
//
//  �y��1,260�s���闝�R�z
//  �C���X�^���X�o�b�t�@�\�z + �A�j���[�V�����X�V +
//  �^�C�v�ʂ̃V�F�[�_�[�ݒ肪�S�Ă��̊֐��ɋl�܂��Ă���B
//  �����I�ɂ�EnemyRenderer�N���X�ɕ������ׂ��B
// ============================================================
void Game::DrawEnemies(DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix, bool skipGloryKillTarget)
{
    static int frameCount = 0;
    frameCount++;

    ////	===	1�b���ƂɃf�o�b�O�����o��	===
    //if (frameCount % 60 == 0)
    //{
    //    auto& enemies = m_enemySystem->GetEnemies();

    //    char debugMsg[512];
    //    sprintf_s(debugMsg, "=== Enemies: %zu ===\n", enemies.size());
    //    //OutputDebugStringA(debugMsg);
    //}

    //  �[�x�������݂������I�ɗL����
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    DirectX::XMFLOAT3 lightDir = { 1.0f, -1.0f, 1.0f };

    //	========================================================================
    //  �C���X�^���X�f�[�^���쐬
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

    //  === ���S�A�j���[�V�����Đ����Ԃ̎擾    ===
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

        // ���[���h�s����v�Z
        float s = 0.01f;
        if (enemy.type == EnemyType::BOSS) s = 0.015f;  // 1.5�{
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(s, s, s);
        DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);
        DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
            enemy.position.x,
            enemy.position.y,
            enemy.position.z
        );
        DirectX::XMMATRIX world = scale * rotation * translation;

        // ========================================================================
        //  ���S���̏�������
        // ========================================================================
        if (enemy.isDying)
        {
            // �A�j���[�V�����I������i�I�[����0.1�b�ȓ��Ȃ�I���Ƃ݂Ȃ��j
            bool animationFinished = (enemy.animationTime >= deathDuration - 0.1f);

            if (animationFinished)
            {
                // MIDBOSS/BOSS�����C���X�^���X�f�[�^���K�v
                if (enemy.type == EnemyType::MIDBOSS || enemy.type == EnemyType::BOSS ||
                    (enemy.type == EnemyType::TANK && enemy.headDestroyed && enemy.currentAnimation == "Attack"))
                {
                    InstanceData instance;
                    DirectX::XMStoreFloat4x4(&instance.world, world);

                    if (enemy.isStaggered)
                    {
                        //  �������C�g�p: Color.a �ɃX�^�K�[�����d����
                         // a > 1.5 �� PS�ŃX�^�K�[�Ɣ���
                         // frac(a) �� �p���X�A�j���[�V�����̈ʑ�
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
                        // �F�͒ʏ�F���ێ��i���̃������C�g�̓V�F�[�_�[�������j
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
                // �A�j���[�V�����Đ��� �� �ʕ`��
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
                    enemy.animationTime  // �� �ʂ̎���
                );

                m_enemyModel->SetBoneScale("Head", 1.0f);
            }

            // �e��`��i�߂��G�����j
            if (m_shadow)
            {
                float sdx = enemy.position.x - playerPos.x;
                float sdz = enemy.position.z - playerPos.z;
                float shadowDist2 = sdx * sdx + sdz * sdz;
                if (shadowDist2 < 400.0f)  // 20m�ȓ������e�`��
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



            continue;  // �����Ă���G�̏����̓X�L�b�v
        }
        //=== �X�^�������O�`�� ===
        if (m_stunRing && enemy.isStaggered && enemy.isAlive && !enemy.isDying)
        {
            float sdx = enemy.position.x - playerPos.x;
            float sdz = enemy.position.z - playerPos.z;
            float distSq = sdx * sdx + sdz * sdz;

            //  20m�ȏ�͕`�悵�Ȃ��A15?20m�̓t�F�[�h�A�E�g
            if (distSq < 300.0f)  // 20m�ȓ�
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

                //  15?20m�ŏ��X�ɏk���i�t�F�[�h�A�E�g�j
                float dist = sqrtf(distSq);
                float fadeStart = 12.0f;  // ��������t�F�[�h�J�n
                float fadeEnd = 17.3f;    // �����Ŋ��S�ɏ�����
                if (dist > fadeStart)
                {
                    float fade = 1.0f - (dist - fadeStart) / (fadeEnd - fadeStart);
                    ringSize *= fade;  // �����قǏ����������R�ɏ�����
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

            // �G�^�C�v�ʂ̃����O�T�C�Y
        }

        // ========================================================================
        // �����Ă���G�̓C���X�^���X���X�g��//
        // ========================================================================
        InstanceData instance;
        DirectX::XMStoreFloat4x4(&instance.world, world);

        //  === ���߂���ԂȂ�_�ł�����   ===
        if (enemy.isStaggered)
        {
            //  �������C�g�p: Color.a �ɃX�^�K�[�����d����
             // a > 1.5 �� PS�ŃX�^�K�[�Ɣ���
             // frac(a) �� �p���X�A�j���[�V�����̈ʑ�
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
            // �F�͒ʏ�F���ێ��i���̃������C�g�̓V�F�[�_�[�������j
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
            //  �X�^�K�[���͌ʕ`��
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
            //  �X�^�K�[���̓C���X�^���V���O�����ʕ`��iIdle�ŐÎ~�j
            if (enemy.isStaggered)
            {
                std::string stunAnim = m_bossModel->HasAnimation("Stun") ? "Stun" : "Idle";
                float animTime = m_bossModel->HasAnimation("Stun")
                    ? enemy.animationTime
                    : 0.0f;  // Idle�̐擪�t���[���ŌŒ�
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

        // �e��`��i�߂��G�����j
        if (m_shadow)
        {
            float sdx = enemy.position.x - playerPos.x;
            float sdz = enemy.position.z - playerPos.z;
            float shadowDist2 = sdx * sdx + sdz * sdz;
            if (shadowDist2 < 400.0f)  // 20m�ȓ������e�`��
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
    // �C���X�^���V���O�`��i�^�C�v�ʁj
    // ========================================================================

    // ====================================================================
    // NORMAL�iY_Bot�j�̕`��
    // ====================================================================
    if (m_enemyModel)
    {
        // ================================================================
        // [PERF] NORMAL: Head ON -  �C���X�^���V���O�`��
        // ================================================================
        m_enemyModel->SetBoneScale("Head", 1.0f);
        {
            {
                m_instWorlds.clear();   // �T�C�Y0�ɂ��邾���B�������͉�����Ȃ��I
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
                if (enemy.currentAnimation == "Attack") continue;  // // ���̃��W�b�N�ێ�

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
    // MIDBOSS �̕`��
    // ====================================================================
    if (m_midBossModel)
    {
        m_midBossModel->SetBoneScale("Head", 1.0f);

        // Walking - ������
        if (!m_midBossWalking.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[3],
                m_midBossModel->GetAnimationDuration("Walk"));
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // Attacking - ������
        if (!m_midBossAttacking.empty())
        {
            float duration = m_midBossModel->GetAnimationDuration("Attack");
            float attackTime = fmod(m_typeAttackTimer[3], duration);

            // �r�[�����˒� or ���J�o���[�� �� �ŏI�t���[���Œ�
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive &&
                    (enemy.bossPhase == BossAttackPhase::ROAR_FIRE ||
                        enemy.bossPhase == BossAttackPhase::ROAR_RECOVERY))
                {
                    attackTime = duration - 0.01f;  // �Ō�̃t���[��
                    break;
                }
            }

            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossAttacking,
                viewMatrix, projectionMatrix, "Attack", attackTime);
        }

        // Dead - ������
        if (!m_midBossDead.empty())
        {
            float finalTime = m_midBossModel->GetAnimationDuration("Death") - 0.001f;
            m_midBossModel->DrawInstanced(m_d3dContext.Get(), m_midBossDead,
                viewMatrix, projectionMatrix, "Death", finalTime);
        }

        // ���Ȃ��`��
        m_midBossModel->SetBoneScale("Head", 0.0f);

        if (!m_midBossAttacking.empty())
        {
            float duration = m_midBossModel->GetAnimationDuration("Attack");
            float attackTime = fmod(m_typeAttackTimer[3], duration);

            // �r�[�����˒� or ���J�o���[�� �� �ŏI�t���[���Œ�
            for (const auto& enemy : m_enemySystem->GetEnemies())
            {
                if (enemy.type == EnemyType::MIDBOSS && enemy.isAlive &&
                    (enemy.bossPhase == BossAttackPhase::ROAR_FIRE ||
                        enemy.bossPhase == BossAttackPhase::ROAR_RECOVERY))
                {
                    attackTime = duration - 0.01f;  // �Ō�̃t���[��
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
    // BOSS �̕`��
    // ====================================================================
    if (m_bossModel)
    {
        m_bossModel->SetBoneScale("Head", 1.0f);

        // Walking - ������
        if (!m_bossWalking.empty())
        {
            float walkTime = fmod(m_typeWalkTimer[4],
                m_bossModel->GetAnimationDuration("Walk"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossWalking,
                viewMatrix, projectionMatrix, "Walk", walkTime);
        }

        // �W�����v�@��
        if (!m_bossAttackingJump.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackJump"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingJump,
                viewMatrix, projectionMatrix, "AttackJump", attackTime);
        }

        // ����グ�a��
        if (!m_bossAttackingSlash.empty())
        {
            float attackTime = fmod(m_typeAttackTimer[4],
                m_bossModel->GetAnimationDuration("AttackSlash"));
            m_bossModel->DrawInstanced(m_d3dContext.Get(), m_bossAttackingSlash,
                viewMatrix, projectionMatrix, "AttackSlash", attackTime);
        }

        // Dead - ������
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

    // === �^�[�Q�b�g�}�[�J�[�`�� ===               // ��������//�i4377�s�ځj
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
    //	HP�o�[��`��
    //	========================================================================
    auto context = m_d3dContext.Get();
    m_effect->SetView(viewMatrix);
    m_effect->SetProjection(projectionMatrix);
    m_effect->SetWorld(DirectX::XMMatrixIdentity());
    m_effect->SetVertexColorEnabled(true);
    m_effect->SetDiffuseColor(DirectX::Colors::White);
    m_effect->Apply(context);
    context->IASetInputLayout(m_inputLayout.Get());

    // ���ʕ`��i�r���{�[�h�̗��\��������j
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;  // �J�����O���Ȃ�
    rsDesc.DepthClipEnable = TRUE;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> noCullRS;
    m_d3dDevice->CreateRasterizerState(&rsDesc, &noCullRS);
    context->RSSetState(noCullRS.Get());

    auto primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
    primitiveBatch->Begin();

    // �J�����̉E�����E��������v�Z�i�r���{�[�h�p�j
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

        // �o�[�̒��S�i�G�̓���j
        DirectX::XMVECTOR center = DirectX::XMVectorSet(
            enemy.position.x, enemy.position.y + 2.5f, enemy.position.z, 0.0f
        );

        // �E�����Ə�����̃I�t�Z�b�g
        DirectX::XMVECTOR halfRight = DirectX::XMVectorScale(camRight, barWidth * 0.5f);
        DirectX::XMVECTOR halfUp = DirectX::XMVectorScale(camUp, barHeight * 0.5f);

        // === �w�i�o�[�i�Â��ԁA�S���j===
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

        // === HP�o�[�i�ԁ������΂̃O���f�AHP�������̕��j===
       // �w�i����O�ɕ`��iZ�t�@�C�e�B���O����j
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

        // HP�����ŐF��ς���i��=�΁A��=���A��=�ԁj
        DirectX::XMFLOAT4 hpColor;
        if (healthPercent > 0.5f)
        {
            float t = (healthPercent - 0.5f) * 2.0f;
            hpColor = { 1.0f - t, 1.0f, 0.0f, 1.0f };  // ������
        }
        else
        {
            float t = healthPercent * 2.0f;
            hpColor = { 1.0f, t, 0.0f, 1.0f };  // �ԁ���
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

        // === �g���i���j ===
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

    // === ���k�����O���E�U���\�� ===
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

                // --- �����O���S�i�G�̋�������j ---
                DirectX::XMVECTOR ringCenter = DirectX::XMVectorSet(
                    enemy.position.x, enemy.position.y + 1.5f, enemy.position.z, 0.0f
                );

                // --- ���k�����O�̔��a�i�偨���ɏk�ށj ---
                float maxRadius = 2.5f;
                float minRadius = 0.5f;  // �G�̑̂ɏd�Ȃ�T�C�Y
                float ringRadius = maxRadius - urgency * (maxRadius - minRadius);

                // --- �F�̑J�� ---
                DirectX::XMFLOAT4 ringColor;
                float alpha;
                float parryZone = 0.25f;

                if (timeToHit < parryZone)
                {
                    // // �� = �u���p���B�I�v
                    float pulse = (sinf(m_gameTime * 30.0f) + 1.0f) * 0.5f;
                    alpha = 0.8f + pulse * 0.2f;
                    ringColor = { 0.0f, 1.0f, 0.3f, alpha };
                }
                else if (timeToHit < 0.5f)
                {
                    // �� = �u���������I�v
                    float pulse = (sinf(m_gameTime * 16.0f) + 1.0f) * 0.5f;
                    alpha = 0.5f + urgency * 0.3f + pulse * 0.2f;
                    ringColor = { 1.0f, 0.1f, 0.0f, alpha };
                }
                else
                {
                    // �Â��� = �u�U�������邼�v
                    alpha = 0.2f + urgency * 0.3f;
                    ringColor = { 0.8f, 0.2f, 0.0f, alpha };
                }

                // --- �Z�p�`�����O�`��i�J�������΃r���{�[�h�j ---
                const int SEGMENTS = 6;
                float rotation = m_gameTime * 1.5f;  // ��������]

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

                // �O�������O��
                for (int s = 0; s < SEGMENTS; s++)
                {
                    int next = (s + 1) % SEGMENTS;
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(ringVerts[s], ringColor),
                        DirectX::VertexPositionColor(ringVerts[next], ringColor)
                    );
                }

                // --- �����Œ胊���O�i�^�[�Q�b�g�T�C�Y = �p���B�^�C�~���O�\���j---
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

                // --- �������i�O�����O���������O���q��6�{�̃K�C�h���j---
                DirectX::XMFLOAT4 lineColor = { ringColor.x, ringColor.y, ringColor.z, alpha * 0.15f };
                for (int s = 0; s < SEGMENTS; s++)
                {
                    primitiveBatch->DrawLine(
                        DirectX::VertexPositionColor(ringVerts[s], lineColor),
                        DirectX::VertexPositionColor(innerVerts[s], innerColor)
                    );
                }

                // --- �p���B�����F�������O�𖾂邭���� ---
                if (timeToHit < parryZone)
                {
                    DirectX::XMFLOAT4 glowColor = { 0.0f, 1.0f, 0.3f, 0.6f };
                    for (int s = 0; s < SEGMENTS; s++)
                    {
                        int next = (s + 1) % SEGMENTS;
                        // �������O�̎O�p�`�œh��Ԃ��i���S���Ӂj
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

    // ���X�^���C�U�����ɖ߂�
    context->RSSetState(nullptr);
}

// ============================================================
//  DrawKeyPrompt - �O���[���[�L���uF�v�v�����v�g�`��
//
//  �y�O��zDrawEnemies() �̌�ɌĂԂ��Ɓi�[�x�e�X�gOFF�{��`��ōőO�ʁj
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
//  DrawSingleEnemy - �{�X/�~�b�h�{�X�̒P�̕`��
//
//  �y�p�r�z�C���X�^���X�`��ɏ��Ȃ�����ȓG�̕`��
//  �{�X�͐�p�V�F�[�_�[��X�P�[�����قȂ邽�ߌʏ���
// ============================================================
void Game::DrawSingleEnemy(const Enemy& enemy, DirectX::XMMATRIX viewMatrix, DirectX::XMMATRIX projectionMatrix)
{
    if (!enemy.isAlive && !enemy.isDying)
        return;

    if (enemy.isExploded)
        return;

    // ���[���h�s����v�Z
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.01f, 0.01f, 0.01f);
    DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(enemy.rotationY);
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(
        enemy.position.x,
        enemy.position.y,
        enemy.position.z
    );
    DirectX::XMMATRIX world = scale * rotation * translation;

    // �K�؂ȃ��f����I��
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

    // �[�x�e�X�g��L����
    m_d3dContext->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    // �A�j���[�V�����`��
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
        DirectX::XMLoadFloat4(&enemy.color),  // XMFLOAT4 �� XMVECTOR
        animName,
        enemy.animationTime
    );

    // ���̃X�P�[�������Z�b�g
    if (enemy.headDestroyed)
    {
        model->SetBoneScaleByPrefix("Head", 1.0f);
    }
}

//  ��]�Ή��ŁF���C��OBB�i��]���锠�j�̌�������

// ============================================================
//  CheckRayIntersection - ���C��AABB�̌�������
//
//  �y�p�r�z����X�|�[���̃C���^���N�V��������
//  �v���C���[�̎������C���I�u�W�F�N�g��AABB�ƌ������邩�v�Z
//  �y�A���S���Y���z�X���u�@�iSlab Method�j
//  �e����tmin/tmax���v�Z���A�S���ŏd�Ȃ��Ԃ�����Ό���
// ============================================================
float Game::CheckRayIntersection(
    DirectX::XMFLOAT3 rayStart,
    DirectX::XMFLOAT3 rayDir,
    DirectX::XMFLOAT3 enemyPos,
    float enemyRotationY,
    EnemyType enemyType)
{
    using namespace DirectX;

    // === �ݒ���擾�i�f�o�b�O���[�h�Ή��j===
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

    // === ���C�����[�J�����W�n�ɕϊ� ===
    XMVECTOR vRayOrigin = XMLoadFloat3(&rayStart);
    XMVECTOR vRayDir = XMLoadFloat3(&rayDir);
    XMVECTOR vEnemyPos = XMLoadFloat3(&enemyPos);

    // ���s�ړ��i�G�����_�Ɂj
    XMVECTOR vLocalOrigin = XMVectorSubtract(vRayOrigin, vEnemyPos);

    // �t��]�i�G�̉�]�̋t�����C�ɂ�����j
    XMMATRIX matInvRot = XMMatrixRotationY(-enemyRotationY);
    vLocalOrigin = XMVector3TransformCoord(vLocalOrigin, matInvRot);
    vRayDir = XMVector3TransformNormal(vRayDir, matInvRot);

    // Float3�ɖ߂�
    XMFLOAT3 localOrigin, localDir;
    XMStoreFloat3(&localOrigin, vLocalOrigin);
    XMStoreFloat3(&localDir, vRayDir);

    // === AABB�i�����s�{�b�N�X�j�Ƃ̔��� ===
    // ���̍ŏ��_�E�ő�_�i������Y=0�ɂȂ�悤�Ɂj
    float minX = -width / 2.0f;
    float minY = 0.0f;
    float minZ = -width / 2.0f;  // �� Z����//

    float maxX = width / 2.0f;
    float maxY = height;
    float maxZ = width / 2.0f;  // �� Z����//

    // �X���u�@�iSlab Method�j�ɂ�锻��
    float tMin = 0.0f;
    float tMax = 10000.0f;

    // === X���`�F�b�N ===
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

    // === Y���`�F�b�N ===
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

    // === Z���`�F�b�N�i���ꂪ�����Ă��I�j===
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

    // ���̓G�ɂ͓�����Ȃ�
    if (tMin < 0.0f) return -1.0f;

    return tMin;  // �Փˋ���
}




// ============================================================
//  DrawBillboard - �r���{�[�h�`��
//
//  �y�p�r�z�X�^�������O��^�[�Q�b�g�}�[�J�[�ȂǁA
//  ��ɃJ�����ɐ��΂���2D�X�v���C�g�̕`��
// ============================================================
void Game::DrawBillboard()
{
    if (!m_showDamageDisplay)
        return;

    // ������DrawCubes�Ɠ����`��ݒ�
   // �ʒu�Ɖ�]��ϐ��ɕۑ�
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

    // �����ȉ��F���L���[�u���_���[�W�\���ʒu�ɕ`��
    DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f) *  // ����������
        DirectX::XMMatrixTranslation(m_damageDisplayPos.x,
            m_damageDisplayPos.y,
            m_damageDisplayPos.z);

    m_cube->Draw(world, viewMatrix, projectionMatrix, DirectX::Colors::Yellow);
}


// ============================================================
//  DrawParticles - �p�[�e�B�N���V�X�e���̕`��
//
//  �y�`��Ώہz
//  - GPU�p�[�e�B�N���i�����Ԃ�/�~�X�g�j
//  - Effekseer�i���{���g���C�����j
//  - �΃p�[�e�B�N��
//  �y�u�����h�z���Z�u�����h �� �Ō�Ƀf�t�H���g�ɖ߂�
// ============================================================
void Game::DrawParticles()
{
    if (m_particleSystem->IsEmpty())
        return;

    auto context = m_d3dContext.Get();

    // === �J�����s�� ===
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

    // === �e�N�X�`���t���G�t�F�N�g ===
    m_particleEffect->SetView(viewMatrix);
    m_particleEffect->SetProjection(projectionMatrix);
    m_particleEffect->SetWorld(DirectX::XMMatrixIdentity());
    m_particleEffect->Apply(context);
    context->IASetInputLayout(m_particleInputLayout.Get());

    // === �A���t�@�u�����h + ���Z�u�����h ===
    float blendFactor[4] = { 0, 0, 0, 0 };
    context->OMSetBlendState(m_states->AlphaBlend(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    // === �r���{�[�h���� ===
    DirectX::XMFLOAT4X4 viewF;
    DirectX::XMStoreFloat4x4(&viewF, viewMatrix);
    DirectX::XMFLOAT3 right(viewF._11, viewF._21, viewF._31);
    DirectX::XMFLOAT3 up(viewF._12, viewF._22, viewF._32);

    // === �e�N�X�`���t���r���{�[�h�`�� ===
    auto batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColorTexture>>(context);
    batch->Begin();

    for (const auto& particle : m_particleSystem->GetParticles())
    {
        float s = particle.size;
        DirectX::XMFLOAT3 c = particle.position;

        // 4���_ + UV���W
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

    // === ���ɖ߂� ===
    context->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
}


// ============================================================
//  DrawWeapon - FPS���_�̕���`��
//
//  �y�`������z�J�����̉E�O���ɃI�t�Z�b�g���ĕ`��
//  ����{�u�i���s���̗h��j�ƃ��R�C���i�ˌ����̒��ˏオ��j��K�p
//  �[�x�o�b�t�@���N���A���Ă���`��i�ǂɂ߂荞�܂Ȃ��j
// ============================================================
void Game::DrawWeapon()
{
    if (!m_weaponModel)
    {
        return;
    }

    //  �p���B���͏e���\���ɂ���
    if (m_shieldState == ShieldState::Parrying ||
        m_shieldState == ShieldState::Guarding ||
        m_shieldBashTimer > 0.0f)
    {
        return; //  �����o�Ă�Ԃ͕\�����Ȃ�
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

    // === FPS���̈ʒu�i�E���j===
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

    // === �X�P�[�� ===
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(m_weaponScale, m_weaponScale, m_weaponScale);

    // === ��]�i�V���v��3���j ===
    DirectX::XMMATRIX modelRotation = DirectX::XMMatrixRotationRollPitchYaw(
        m_weaponRotX - m_weaponKickRot + m_reloadAnimTilt,
        m_weaponRotY, m_weaponRotZ
    );
    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(weaponPos);

    // ����
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
// ��������ɕ`��iDOOM: The Dark Ages �X�^�C���j
// =================================================================

// ============================================================
//  DrawShield - ���̕`��i�K�[�h/����/�`���[�W��ԁj
//
//  �y��ԕʕ`��z
//  - Guarding/Charging: �v���C���[�̑O���ɕ\��
//  - Throwing: �������̋O����ɕ\���i��]����j
//  - Idle: FPS���_�ō���ɕ\��
//  - Broken: ��\��
// ============================================================
void Game::DrawShield()
{
    if (!m_shieldModel)
        return;

    // // ���������͕ʂ̕`��i���[���h��ԂŔ��ł鏂�j
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

        // ���[���h��Ԃł̏��̈ʒu
        float shieldScale = 0.25f;
        DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(shieldScale, shieldScale, shieldScale);

        // ��]: ���̌����␳ + �����X�s��
        DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PI);
        DirectX::XMMATRIX standUp = DirectX::XMMatrixRotationX(-DirectX::XM_PIDIV2);
        DirectX::XMMATRIX spin = DirectX::XMMatrixRotationY(m_thrownShieldSpin);

        // ��ԕ����Ɍ�����
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
        return;  // �ʏ�`��̓X�L�b�v
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

    // �J�����̕����x�N�g��
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

    // === ���̈ʒu�A�j���[�V���� ===
    // �p���B��: ���� �� ���ʒ����ɃX���C�h
    float guardProgress = 0.0f;  // 0.0=�ҋ@�ʒu�A1.0=���ʃK�[�h�ʒu
    if (m_shieldBashTimer > 0.0f)
    {
        float t = 1.0f - (m_shieldBashTimer / m_shieldBashDuration);
        // �ŏ���20%: �T�b�Ɛ��ʂɍ\����i�C�[�W���O�őf�����j
        // �Ō��30%: �������߂�
        // ����50%: ���ʃL�[�v
        if (t < 0.2f)
        {
            // 0��1 �ɑf�����ieaseOut�j
            float p = t / 0.2f;
            guardProgress = 1.0f - (1.0f - p) * (1.0f - p);  // easeOutQuad
        }
        else if (t < 0.7f)
        {
            // ���ʃL�[�v
            guardProgress = 1.0f;
        }
        else
        {
            // 1��0 �ɂ������߂�
            float p = (t - 0.7f) / 0.3f;
            guardProgress = 1.0f - p * p;  // easeInQuad
        }
    }

    // === �ҋ@�ʒu�i�����j===
    float restRight = -0.35f;    // ����
    float restUp = -0.30f;       // ��
    float restForward = 0.45f;   // �O

    // === �K�[�h�ʒu�i���ʒ����j===
    float guardRight = -0.10f;   // �قڒ����i�������j
    float guardUp = -0.30f;      // �ڐ��̏�����
    float guardForward = 0.40f;  // �����O�ɏo���ď�����������

    // ���`��ԁilerp�j�őҋ@���K�[�h���X���[�Y�ɑJ��
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

    // === �X�P�[�� ===
    float shieldScale = 0.25f;
    DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(shieldScale, shieldScale, shieldScale);

    // === ��] ===
    DirectX::XMMATRIX modelFix = DirectX::XMMatrixRotationY(DirectX::XM_PI);
    DirectX::XMMATRIX standUp = DirectX::XMMatrixIdentity();
    // �K�[�h���͏����܂������A�ҋ@���͏����X����
    float tiltAngle = 0.1f * (1.0f - guardProgress);
    DirectX::XMMATRIX tilt = DirectX::XMMatrixRotationX(tiltAngle);

    DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        playerRot.x, playerRot.y, 0.0f
    );
    DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(shieldPos);

    // �����ibashTilt �͍폜�Atilt �ɓ����ς݁j
    DirectX::XMMATRIX shieldWorld = scale * modelFix * standUp * tilt * cameraRotation * translation;

    m_shieldModel->Draw(
        m_d3dContext.Get(),
        shieldWorld,
        viewMatrix,
        projectionMatrix,
        DirectX::Colors::White
    );
}

//  �`��S��

// ============================================================
//  DrawWeaponSpawns - �ǂ̕���X�|�[���̕`��
//
//  �y�`����e�z����X�|�[���V�X�e�����Ǘ�����
//  �Ǌ|������̃��f����3D��Ԃɕ`��
// ============================================================
void Game::DrawWeaponSpawns()
{
    if (!m_weaponSpawnSystem)
        return;

    //  �r���[�E�v���W�F�N�V�����s��
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


    //  === �S�Ă̕Ǖ����`��   ===
    for (const auto& spawn : m_weaponSpawnSystem->GetSpawns())
    {
        //  ���[���h�s��
        DirectX::XMMATRIX world = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) *
            DirectX::XMMatrixTranslation(spawn.position.x, spawn.position.y, spawn.position.z);

        //  �F(���R��ɂ���ĕς���)
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
        // ���Е`��
        DrawGibs(viewMatrix, projectionMatrix);
    }

}


// ============================================================
//  DrawHealthPickups - �w���X�s�b�N�A�b�v�̕`��
//
//  �y�`����e�z�G���h���b�v�����񕜃A�C�e����3D��Ԃɕ`��
//  ���V�A�j���[�V�����i�㉺�ɗh���j+ ��]
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

    // �J������Right/Up�x�N�g���擾
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, view);
    DirectX::XMFLOAT4X4 ivF;
    DirectX::XMStoreFloat4x4(&ivF, invView);
    DirectX::XMFLOAT3 camRight(ivF._11, ivF._12, ivF._13);
    DirectX::XMFLOAT3 camUp(ivF._21, ivF._22, ivF._23);

    for (auto& pickup : m_healthPickups)
    {
        if (!pickup.isActive) continue;

        // �{�u���o�i�㉺�ɕ��V�j
        float bobY = sinf(pickup.bobTimer) * 0.2f;

        // �t�F�[�h�i�c��5�b�œ_�Łj
        float alpha = 1.0f;
        if (pickup.lifetime < 5.0f)
            alpha = (sinf(pickup.lifetime * 8.0f) * 0.5f + 0.5f);

        // �ΐF�i����߁j
        DirectX::XMFLOAT4 color(0.1f, 1.0f, 0.2f, alpha);

        DirectX::XMFLOAT3 center = pickup.position;
        center.y += bobY + 0.5f;

        float size = 0.4f;

        // --- �\���̏c�_ ---
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

        // --- �\���̉��_ ---
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