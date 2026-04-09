// UISystem.h - UI�`��V�X�e��
// �y�ړI�z�Q�[����ʂ�UI�iHP�A�|�C���g�A�e��Ȃǁj��`��
#pragma once

#include <DirectXMath.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <memory>

#include "WeaponSpawn.h"

// �O���錾
class Player;
class WeaponSystem;
class WaveManager;
struct WeaponSpawn;

// UI�V�X�e���N���X
class UISystem {
public:
    // �R���X�g���N�^
    // �y�����z��ʃT�C�Y��ݒ�
    UISystem(int screenWidth, int screenHeight);

    // === ���C���`�� ===

    // DrawAll - �S�Ă�UI�v�f��`��
    // �y�����zHP�A�|�C���g�A�e��ȂǑSUI�v�f���ꊇ�`��
    // �y�����zbatch: DirectX�̃v���~�e�B�u�o�b�`�i�`��c�[���j
    //        player: �v���C���[�iHP�E�|�C���g�擾�p�j
    //        weaponSystem: ����V�X�e���i�e��擾�p�j
    //        waveManager: �E�F�[�u�Ǘ��i�E�F�[�u�ԍ��擾�p�j
    void DrawAll(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        const Player* player,
        const WeaponSystem* weaponSystem,
        const WaveManager* waveManager);

    // OnScreenSizeChanged - ��ʃT�C�Y�ύX���ɌĂ�
    void OnScreenSizeChanged(int width, int height);

    void DrawWeaponPrompt(
    DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        WeaponSpawn* weaponSpawn,
        int playerPoints,
        bool alreadyOwned);

private:
    // === �v���C�x�[�g�`��֐� ===

    // �eUI�v�f�̕`��
    void DrawHealthBar(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int health);
    void DrawCrosshair(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch);
    void DrawWaveNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int wave);
    void DrawPoints(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int points);
    void DrawAmmo(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        int currentAmmo, int reserveAmmo, bool isReloading);
    void DrawWeaponNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch, int weaponNum);
    void DrawBox(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        float x, float y, float width, float height, DirectX::XMFLOAT4 color);
    void DrawBoxOutline(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        float x, float y, float width, float height, DirectX::XMFLOAT4 color);

    // �����`��w���p�[
    // �y�����z0-9�̐�������ŕ`��
    void DrawSimpleNumber(DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* batch,
        int digit, float x, float y, DirectX::XMFLOAT4 color);

    // === �����o�ϐ� ===

    int m_screenWidth;   // ��ʂ̕�
    int m_screenHeight;  // ��ʂ̍���
};