#pragma once

#include <DirectXMath.h>	//	XMFLOAT3
#include "EnemySystem.h"

//	�E�F�[�u�Ǘ��N���X
class WaveManager {
public:
	//	�R���X�g���N�^
	//	�y�����z�����l��ݒ�i�E�F�[�u1�A�G10�̂Ȃǁj
	WaveManager();

	//	���C������

	//	Update - �E�F�[�u�̐i�s�Ǘ�
	//	�y�����z�������ԁE�X�|�[���E�E�F�[�u�N���A����
	//	�y�����zdeltaTime: �o�ߎ���
	//		   playerPos: �v���C���[�ʒu
	//		   enemySystem: �G�V�X�e���i�X�|�[���g�p�j
	void Update(float deltaTime, DirectX::XMFLOAT3 playerPos, EnemySystem* enemySystem);

	//	===	�G���j�ʒm	===

	//	OnEnemyKilled - �G��|�����Ƃ��ɌĂ�
	//	�y�����z���j�����J�E���g�A�E�F�[�u�N���A����
	//	�y�߂�l�z�E�F�[�u�N���A�{�[�i�X�|�C���g�i0 or 100�j
	int OnEnemyKilled();

	//	===	�Q�b�^�[	===

	//	GetCurrentWave	-	���݂̃E�F�[�u�ԍ�
	int GetCurrentWave() const { return m_currentWave; }

	//	GetEnemyCountForWave - �w��Wave�̓G�����v�Z
	//	�y�����zWave�ԍ�����G���������v�Z
	//	�y�����zwave: Wave�ԍ��i1, 2, 3...�j
	//	�y�߂�l�z����Wave�̓G��
	//	�y�v�Z���zbaseEnemyCount * (difficultyScale ^ (wave - 1))
	//	�y��zWave1=10��, Wave2=12��, Wave3=14��...
	int GetEnemyCountForWave(int wave) const;

	//	GetDifficultyMultiplier - ���݂�Wave�̓�Փx�{��
	//	�y�����z�G��HP�E�_���[�W�{�����擾
	//	�y�߂�l�z��Փx�{���i1.0 �` 3.0�j
	//	�y�v�Z���z1.0 + (currentWave - 1) * 0.1
	//	�y��zWave1=1.0�{, Wave5=1.4�{, Wave10=1.9�{
	float GetDifficultyMultiplier() const;

	//	IsVictoryWave - ���݂��{�XWave������
	//	�y�����z10Wave���ƂɃ{�X�킩�ǂ���
	//	�y�߂�l�ztrue=�{�XWave, false=�ʏ�Wave
	//	�y��zWave10, Wave20, Wave30... �� true
	bool IsVictoryWave() const;

	void Reset();


	void SetPaused(bool paused) { m_paused = paused; }
	bool IsPaused() const { return m_paused; }

	// game.cpp ����m�F�����Z�b�g�p
	bool DidMidBossSpawn() { bool r = m_midBossJustSpawned; m_midBossJustSpawned = false; return r; }
	bool DidBossSpawn() { bool r = m_bossJustSpawned;    m_bossJustSpawned = false;    return r; }

private:
	bool m_paused = false;

	//	===	�E�F�[�u���	===
	int	m_currentWave;				//	���݂̃E�F�[�u�ԍ�
	int m_enemiesKilledThisWave;	//	���̃E�F�[�u�œ|������
	int m_totalEnemiesThisWave;		//	�E�F�[�u�̑��G��
	bool m_betweenWaves;			//	�E�F�[�u�Ԃ��ǂ���
	float m_waveStartTimer;			//	���E�F�[�u�܂ł̎���
	float m_enemySpawnTimer;		//	�G�X�|�[���p�^�C�}�[

	int m_baseEnemyCount;			//	��{�G���iWave1�̓G���j

	// �{�X�X�|�[���ʒm�t���O
	bool m_midBossJustSpawned = false;
	bool m_bossJustSpawned = false;

	float m_difficultyScale;		//	��Փx�X�P�[���iWave�������j
};