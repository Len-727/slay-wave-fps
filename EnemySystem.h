//	EnemySystem.h

#pragma once
#include "Entities.h"
#include <vector>
#include <DirectXMath.h>
#include <unordered_map>
#include <algorithm>

class EnemySystem {
public:
	//	�G�̔z�����ɂ��āA�X�|�[���^�C�}�[�Ȃǂ�������
	//	�R���X�g���N�^
	EnemySystem();

	// �y�����z�S�Ă̓G�̈ړ��A�v���C���[�ւ̐ڋ߂Ȃǂ�����
	// �y�����zdeltaTime: �O�t���[������̌o�ߎ��ԁi�b�j
	//        playerPos: �v���C���[�̌��݈ʒu�i�G���ǂ������邽�߁j
	void Update(float deltaTime, DirectX::XMFLOAT3 playerPos);

	// SpawnEnemy - �V�����G��1�̐���
	// �y�����z�����_���Ȉʒu�ɓG���o��������
	// �y�����zplayerPos: �v���C���[�ʒu�i�v���C���[���痣�ꂽ�ꏊ�ɃX�|�[���j
	// �y�Ȃ�public���zGame�N���X����u���G���o���I�v�Ɩ��߂��邽��
	void SpawnEnemy(DirectX::XMFLOAT3 playerPos);

    void SpawnMidBoss(DirectX::XMFLOAT3 playerPos);
    void SpawnBoss(DirectX::XMFLOAT3 playerPos);

	// ClearDeadEnemies - ���񂾓G��z�񂩂�폜
	// �y�����zisAlive == false�̓G��m_enemies�������
	// �y���R�z���񂾓G���c���ƃ������̖��ʁ��������d���Ȃ�
	void ClearDeadEnemies();

	//	�Q�b�^�[ (����ǂݎ��֐��j

	// GetEnemies (const��) - �G�̔z���ǂݎ���p�Ŏ擾
	// �y�p�r�z�`�掞�Ȃǁu���邾���v�̏����Ŏg��
	// �y�߂�l�zconst std::vector<Enemy>&
	//   - const: �ύX�s�i���S�j
	//   - &: �Q�Ɓi�R�s�[���Ȃ��̂ō����j
	const std::vector<Enemy>& GetEnemies() const { return m_enemies; }

    std::vector<Enemy>& GetEnemiesMutable() { return m_enemies; }

	// GetEnemies (��const��) - �G�̔z���ύX�\�Ȍ`�Ŏ擾
    // �y�p�r�z�ˌ����Ƀ_���[�W��^����Ȃǁu�ύX����v�����Ŏg��
    // �y�߂�l�zstd::vector<Enemy>&
    //   - &: �Q�Ɓi�{����G��j
    //   - const�Ȃ�: �ύX�\
    // �y�Ȃ�2�K�v���z�p�r�ɉ����Ďg�������邱�ƂŁA�o�O��h��
	std::vector<Enemy>& GetEnemies() { return m_enemies; }	//	��const�Łi�_���[�W�����p�j

	//	�ő�I�����擾�@�u����ȏ�G�������Ȃ��v�Ƃ���������`�F�b�N
	int GetMaxEnemies() const { return m_maxEnemies; }

	//	�E�F�[�u�Ǘ��p	�E�F�[�u���i�ނƓG�̏���𑝂₷�ȂǂŎg��
	void SeMaxEnemies(int max) { m_maxEnemies = max; }

    // �A�^�b�N�^�C�~���O�@IMGui
    float m_normalAttackHitTime = 1.22f;
    float m_normalAttackDuration = 3.83f;
    float m_runnerAttackHitTime = 1.2f;
    float m_runnerAttackDuration = 2.20f;
    float m_tankAttackHitTime = 1.2f;
    float m_tankAttackDuration = 3.17f;

    // �f�o�b�O�p�F�w��^�C�v�̓G��1�̃X�|�[��
    void SpawnEnemyOfType(EnemyType type, DirectX::XMFLOAT3 playerPos);

    // �A�j���[�V�������x�i�^�C�v�~�A�j���ʁj
    // [0]=NORMAL [1]=RUNNER [2]=TANK [3]=MIDBOSS [4]=BOSS
    float m_animSpeed_Idle[5] = { 1.0f, 1.20f, 1.0f, 1.0f, 1.0f };
    float m_animSpeed_Walk[5] = { 1.0f, 1.20f, 1.0f, 1.0f, 1.0f };
    float m_animSpeed_Run[5] = { 1.0f, 1.20f, 1.0f, 1.0f, 1.0f };
    float m_animSpeed_Attack[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.00f };
    float m_animSpeed_Death[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    // === �{�X�U���^�C�~���O�iImGui�����p�j ===
    float m_bossJumpWindupTime = 0.5f;       // �W�����v����
    float m_bossJumpAirTime = 0.6f;          // �W�����v�؋�
    float m_bossSlamRecoveryTime = 1.5f;     // ���n�d��
    float m_bossSlashWindupTime = 0.8f;      // �a������
    float m_bossSlashFireTime = 0.2f;        // �a������
    float m_bossSlashRecoveryTime = 1.0f;    // �a����d��
    float m_bossRoarWindupTime = 1.0f;       // �r�[������
    float m_bossRoarFireTime = 5.0f;         // �r�[������
    float m_bossRoarRecoveryTime = 2.0f;     // �r�[����d��
    float m_bossAttackCooldown = 3.0f;       // �U���ԃN�[���_�E��
    float m_bossMeleeHitTime = 0.5f;         // �ߐڂ̉���u��
    float m_bossMeleeDuration = 3.17f;        // �ߐڃ��[�V�����S��
    float m_bossJumpHeight = 8.0f;           // �W�����v�̍���


    //  �E�F�[�u��Փx�X�P�[�����O
    float m_waveHpMult = 1.0f;  // HP�{��
    float m_waveSpeedMult = 1.0f;  // �ړ����x�{��
    float m_waveDamageMult = 1.0f; // �U���͔{��

    void SetWaveScaling(int wave)
    {
        float w = (float)(wave - 1);
        m_waveHpMult = (std::min)(1.0f + w * 0.08f, 3.0f);  // �ő�3�{�iWave26�œ��B�j
        m_waveSpeedMult = (std::min)(1.0f + w * 0.03f, 1.8f);   // �ő�1.8�{�iWave27�œ��B�j
        m_waveDamageMult = (std::min)(1.0f + w * 0.06f, 2.5f);  // �ő�2.5�{�iWave26�œ��B�j
    }
private:
    // �O���b�h�Z���̃L�[�iX, Z���W�̃y�A�j
    struct GridKey
    {
        int x;  // �Z����X���W
        int z;  // �Z����Z���W

        // ��r���Z�q�iunordered_map�ŕK�v�j
        bool operator==(const GridKey& other) const
        {
            return x == other.x && z == other.z;
        }
    };

    // �n�b�V���֐��iGridKey�𐔒l�ɕϊ��j
    struct GridKeyHash
    {
        size_t operator()(const GridKey& key) const
        {
            // 2�̐�����1�̃n�b�V���l�ɕϊ�
            // �傫�ȑf�����g���ďՓ˂����炷
            return ((size_t)key.x * 73856093) ^ ((size_t)key.z * 19349663);
        }
    };

    // === ��ԕ����p�̊֐��錾 ===

    // ���W����O���b�h�L�[���擾
    GridKey GetGridKey(const DirectX::XMFLOAT3& position) const;

    // ��ԃO���b�h���\�z�i���t���[���Ăԁj
    void BuildSpatialGrid();

    // ��ԕ������g�����Փˉ���
    void ResolveCollisionsSpatial();

    // �X�|�[���ʒu�����̓G�Əd�Ȃ�Ȃ����`�F�b�N
    bool IsPositionValid(const DirectX::XMFLOAT3& position, float minDistance) const;

    // === ��ԕ����p�̃����o�ϐ� ===

    // �Z���T�C�Y�i5m �~ 5m �̃O���b�h�j
    const float CELL_SIZE = 5.0f;

    // ��ԃn�b�V���e�[�u��
    // �L�[: GridKey�i�Z���̍��W�j
    // �l: vector<size_t>�i���̃Z�����̓G�̃C���f�b�N�X�j
    std::unordered_map<GridKey, std::vector<size_t>, GridKeyHash> m_spatialGrid;


	// UpdateEnemyMovement - 1�̂̓G�̈ړ����X�V
	// �y�����z�G���v���C���[�Ɍ������ē�����
	// �y�����zenemy: �������G�i�Q�Ƃœn���̂Ŗ{����ύX�j
	//        playerPos: �v���C���[�ʒu
	//        deltaTime: �o�ߎ���
	// �y�Ȃ�private���z�O�����璼�ڌĂԕK�v���Ȃ���������
	void UpdateEnemyMovement(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime);

    void UpdateBossAI(Enemy& enemy, DirectX::XMFLOAT3 playerPos, float deltaTime);

	std::vector<Enemy> m_enemies;	//	�}�b�v�ɂ��邷�ׂĂ̓G���i�[
	int m_maxEnemies;				//	�����ɑ��݂ł���G�̍ő吔
	float m_enemySpawnTimer;		//	�G�̎����X�|�[���p�^�C�}�[
	int m_nextEnemyID;				//	���Ɋ��蓖�Ă�ID
};