// ============================================================
//  GamePhysics.cpp
//  Bullet Physics�����E���C�L���X�g�E�i�r�O���b�h�\�z
//
//  �y�Ӗ��z
//  - Bullet Physics���[���h�̏�����/�j��
//  - �G�̃L�l�}�e�B�b�N���̂�///�X�V/�폜
//  - ���C�L���X�g�ɂ��ˌ�����i�G�̂�/�}�b�v�̂݁j
//  - ���b�V���R���C�_�[�ɂ��ǏՓ˔���
//  - A*�p�X�t�@�C���f�B���O�p�i�r�O���b�h�̎����\�z
//
//  �y�݌v�Ӑ}�z
//  game.cpp���畨�����Z�n�𕪗��B
//  �����֘A�̕ύX�����̕`��/UI�R�[�h�ɉe�����Ȃ��悤�ɂ���B
//  Bullet Physics�ւ̈ˑ������̃t�@�C���ɏW��B
// ============================================================

#include "Game.h"
#include "Player.h"

using namespace DirectX;

// ============================================================
//  InitPhysics - Bullet Physics���[���h�̏�����
//
//  �y�Ăяo�����zGame::Initialize()
//  �y���������z�����_�����O���\�[�X�쐬��ɌĂԂ���
//    �i�������������ԈႦ��ƃN���b�V������j
// ============================================================
void Game::InitPhysics()
{
    // �Փˌ��o�̐ݒ�i�f�t�H���g�ŏ\���j
    m_collisionConfiguration =
        std::make_unique<btDefaultCollisionConfiguration>();

    // �Փ˃y�A�̊Ǘ��i�u���[�h�t�F�[�Y�ōi������̐�������j
    m_dispatcher =
        std::make_unique<btCollisionDispatcher>(m_collisionConfiguration.get());

    // ��ԕ����iAABB�d���̍������o�j
    m_broadphase = std::make_unique<btDbvtBroadphase>();

    // �S���\���o�[�i����̓L�l�}�e�B�b�N�݂̂Ȃ̂Ōy�ʁj
    m_solver =
        std::make_unique<btSequentialImpulseConstraintSolver>();

    // ���[���h�쐬�i�S�R���|�[�l���g��ڑ��j
    m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(),
        m_broadphase.get(),
        m_solver.get(),
        m_collisionConfiguration.get()
    );
}

// ============================================================
//  CleanupPhysics - �S�������\�[�X�̈��S�ȉ��
//
//  �y����������d�v�z
//  1. �}�b�v���b�V������ �� 2. �G�̍��� �� 3. ���[���h�{��
//  �t���ɂ���ƃ_���O�����O�|�C���^�ŃN���b�V������
// ============================================================
void Game::CleanupPhysics()
{
    // --- �}�b�v���b�V���R���C�_�[�̉�� ---
    if (m_mapMeshBody)
    {
        if (m_dynamicsWorld) m_dynamicsWorld->removeRigidBody(m_mapMeshBody);
        delete m_mapMeshBody->getMotionState();
        delete m_mapMeshBody;
        m_mapMeshBody = nullptr;
    }
    if (m_mapMeshShape) { delete m_mapMeshShape; m_mapMeshShape = nullptr; }
    if (m_mapTriMesh) { delete m_mapTriMesh; m_mapTriMesh = nullptr; }

    // --- �S�G�̕����{�f�B���폜 ---
    for (auto& pair : m_enemyPhysicsBodies)
    {
        btRigidBody* body = pair.second;

        if (m_dynamicsWorld)
        {
            m_dynamicsWorld->removeRigidBody(body);
        }

        if (body->getMotionState())
        {
            delete body->getMotionState();
        }
        if (body->getCollisionShape())
        {
            delete body->getCollisionShape();
        }
        delete body;
    }
    m_enemyPhysicsBodies.clear();

    // --- Bullet Physics���[���h�{�̂���� ---
    // �쐬�̋t���Ŕj���i�ˑ��֌W���󂳂Ȃ��j
    m_dynamicsWorld.reset();
    m_solver.reset();
    m_broadphase.reset();
    m_dispatcher.reset();
    m_collisionConfiguration.reset();
}

// ============================================================
//  RaycastPhysics - ���C�L���X�g�œG�Ƃ̏Փ˔���
//
//  �y�p�r�z�ˌ�����i�e�e�̋O����ɓG�����邩�j
//  �y�d�g�݁z
//    - EnemyOnlyRayCallback �ŕ�(userPtr==null)�𖳎�
//    - userPointer��ID+1���i�[�i0��nullptr�Ƌ�ʂł���悤�j
//  �y�߂�l�zRaycastResult�i�q�b�g�ʒu�E�@���E�G�|�C���^�j
// ============================================================
Game::RaycastResult Game::RaycastPhysics(
    DirectX::XMFLOAT3 start,
    DirectX::XMFLOAT3 direction,
    float maxDistance)
{
    RaycastResult result;
    result.hit = false;
    result.hitEnemy = nullptr;
    result.hitPoint = { 0, 0, 0 };
    result.hitNormal = { 0, 0, 0 };

    // �����x�N�g���𐳋K���i����1�ɂ���j
    float length = sqrtf(
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z
    );

    // �[���x�N�g���`�F�b�N�i0���Z�h�~�j
    constexpr float DIRECTION_EPSILON = 0.0001f;
    if (length < DIRECTION_EPSILON)
        return result;

    direction.x /= length;
    direction.y /= length;
    direction.z /= length;

    // ���C�̎n�_�ƏI�_��Bullet�`���ɕϊ�
    btVector3 rayStart(start.x, start.y, start.z);
    btVector3 rayEnd(
        start.x + direction.x * maxDistance,
        start.y + direction.y * maxDistance,
        start.z + direction.z * maxDistance
    );

    // --- �J�X�^���R�[���o�b�N: �G�̃J�v�Z�������ɓ�����悤�ɂ��� ---
    // �}�b�v���b�V��(userPtr==nullptr)�̓X�L�b�v
    struct EnemyOnlyRayCallback : public btCollisionWorld::ClosestRayResultCallback
    {
        EnemyOnlyRayCallback(const btVector3& from, const btVector3& to)
            : ClosestRayResultCallback(from, to) {
        }

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
            bool normalInWorldSpace) override
        {
            if (rayResult.m_collisionObject->getUserPointer() == nullptr)
                return 1.0f; // �}�b�v���b�V�� �� �������đ��s

            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    EnemyOnlyRayCallback rayCallback(rayStart, rayEnd);
    m_dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);

    if (rayCallback.hasHit())
    {
        result.hit = true;

        // �q�b�g�ʒu�Ɩ@�����擾
        result.hitPoint.x = rayCallback.m_hitPointWorld.getX();
        result.hitPoint.y = rayCallback.m_hitPointWorld.getY();
        result.hitPoint.z = rayCallback.m_hitPointWorld.getZ();

        result.hitNormal.x = rayCallback.m_hitNormalWorld.getX();
        result.hitNormal.y = rayCallback.m_hitNormalWorld.getY();
        result.hitNormal.z = rayCallback.m_hitNormalWorld.getZ();

        // --- UserPointer����GID�𕜌� ---
        // �i�[��: (void*)(intptr_t)(id + 1) �Ȃ̂ŁA�������� -1
        const btCollisionObject* hitObj = rayCallback.m_collisionObject;
        void* userPtr = hitObj->getUserPointer();

        if (userPtr != nullptr)
        {
            int enemyID = (int)(intptr_t)userPtr - 1;

            // ID����G�������i���`�T�� �� �G���������ꍇ��map���������j
            const auto& enemies = m_enemySystem->GetEnemies();
            for (const auto& enemy : enemies)
            {
                if (enemy.id == enemyID && enemy.isAlive)
                {
                    // const_cast: GetEnemies()��const�Q�Ƃ�Ԃ�����
                    // �y���P�āzGetEnemiesMutable()���g���ׂ������A
                    //   �ˌ�����̌Ăяo������mutable�łɐ؂�ւ���K�v������
                    result.hitEnemy = const_cast<Enemy*>(&enemy);
                    break;
                }
            }
        }
    }

    return result;
}

// ============================================================
//  AddEnemyPhysicsBody - �G�̃L�l�}�e�B�b�N���̂�//
//
//  �y�L�l�}�e�B�b�N���̂Ƃ́z
//  mass=0�Ŏ����ł͓����Ȃ����A���C�L���X�g�ɓ�����B
//  ���t���[��UpdateEnemyPhysicsBody�ňʒu���蓮�X�V����B
//  �i�_�C�i�~�b�N���̂��ƓG���m���e�������Đ���s�\�ɂȂ邽�߁j
// ============================================================
void Game::AddEnemyPhysicsBody(Enemy& enemy)
{
    // �f�o�b�O�p�q�b�g�{�b�N�X�ƃf�t�H���g�l��؂�ւ�
    EnemyTypeConfig config;
    if (m_useDebugHitboxes)
    {
        switch (enemy.type)
        {
        case EnemyType::NORMAL:  config = m_normalConfigDebug;  break;
        case EnemyType::RUNNER:  config = m_runnerConfigDebug;  break;
        case EnemyType::TANK:    config = m_tankConfigDebug;    break;
        case EnemyType::MIDBOSS: config = m_midbossConfigDebug; break;
        case EnemyType::BOSS:    config = m_bossConfigDebug;    break;
        default: config = GetEnemyConfig(enemy.type); break;
        }
    }
    else
    {
        config = GetEnemyConfig(enemy.type);
    }

    // �J�v�Z���`����쐬�i���a + �~�������j
    float radius = config.bodyWidth / 2.0f;
    float totalHeight = config.bodyHeight;
    float cylinderHeight = totalHeight - 2.0f * radius;

    if (cylinderHeight < 0.0f)
        cylinderHeight = 0.0f;

    btCollisionShape* shape = new btCapsuleShape(radius, cylinderHeight);

    // �����ʒu�i�����ł͂Ȃ��̂̒��S�j
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        enemy.position.x,
        enemy.position.y + totalHeight / 2.0f,
        enemy.position.z
    ));

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);

    // mass=0 �� �ÓI/�L�l�}�e�B�b�N�I�u�W�F�N�g
    constexpr btScalar KINEMATIC_MASS = 0.0f;
    btVector3 localInertia(0, 0, 0);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        KINEMATIC_MASS,
        motionState,
        shape,
        localInertia
    );

    btRigidBody* body = new btRigidBody(rbInfo);

    // �L�l�}�e�B�b�N�t���O�ݒ�i�蓮�ňʒu�X�V����I�u�W�F�N�g�j
    body->setCollisionFlags(
        body->getCollisionFlags() |
        btCollisionObject::CF_KINEMATIC_OBJECT
    );

    // �X���[�v�����Ȃ��i���t���[���ʒu���ς�邽�߁j
    body->setActivationState(DISABLE_DEACTIVATION);

    // ID+1��UserPointer�Ɋi�[�i0=nullptr�Ƌ�ʂ��邽��+1�j
    body->setUserPointer((void*)(intptr_t)(enemy.id + 1));

    m_dynamicsWorld->addRigidBody(body);
    m_enemyPhysicsBodies[enemy.id] = body;
}

// ============================================================
//  CheckMeshCollision - ��vs���b�V���̏Փ˔���
//
//  �y�p�r�z�v���C���[�̕ǂ߂荞�ݖh�~
//  �y�d�g�݁z�e�X�g�p�̋�������āA�}�b�v���b�V���Ƃ�
//    �ڐG�y�A�e�X�g(contactPairTest)�Ŕ���
// ============================================================
bool Game::CheckMeshCollision(DirectX::XMFLOAT3 position, float radius)
{
    if (!m_dynamicsWorld || !m_mapMeshBody) return false;

    // �e�X�g�p�̋������i�v���C���[�̑̂�͋[�j
    btSphereShape sphere(radius);

    btCollisionObject testObj;
    testObj.setCollisionShape(&sphere);

    // �ڂ̍���(position.y)����̂̒��S�ɉ�����
    constexpr float EYE_TO_CENTER_OFFSET = 0.9f;
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(
        position.x,
        position.y - EYE_TO_CENTER_OFFSET,
        position.z));
    testObj.setWorldTransform(t);

    // Bullet�Ɂu���̋��ƃ}�b�v���b�V���A�d�Ȃ��Ă�H�v�ƕ���
    struct ContactCallback : public btCollisionWorld::ContactResultCallback
    {
        bool hit = false;

        btScalar addSingleResult(
            btManifoldPoint& cp,
            const btCollisionObjectWrapper* colObj0Wrap,
            int partId0, int index0,
            const btCollisionObjectWrapper* colObj1Wrap,
            int partId1, int index1) override
        {
            hit = true;
            return 0; // 0��Ԃ��ƒT���𑁊��I���i1������Ώ\���j
        }
    };

    ContactCallback callback;
    m_dynamicsWorld->contactPairTest(&testObj, m_mapMeshBody, callback);

    return callback.hit;
}

// ============================================================
//  GetMeshFloorHeight - ���b�V���̏��̍��������C�L���X�g�Ŏ擾
//
//  �y�p�r�z�v���C���[/�G/�p�[�e�B�N����Y���W��n�ʂɍ��킹��
//  �y�d�v�z�S�Ă�Y���W���W�b�N�͂��̊֐����o�R���邱�ƁI
//    Y=0�̃n�[�h�R�[�h�͋֎~�iFBX�}�b�v�̏��͕��R�ł͂Ȃ��j
//  �y�d�g�݁z�^��(Y=100)����^��(Y=-100)�Ƀ��C���΂��A
//    �}�b�v���b�V�������ɓ��������_��Y��Ԃ�
// ============================================================
float Game::GetMeshFloorHeight(float x, float z, float defaultY)
{
    if (!m_dynamicsWorld || !m_mapMeshBody) return defaultY;

    constexpr float RAY_START_Y = 100.0f;
    constexpr float RAY_END_Y = -100.0f;

    btVector3 rayFrom(x, RAY_START_Y, z);
    btVector3 rayTo(x, RAY_END_Y, z);

    // �}�b�v���b�V�������ɓ�����J�X�^���R�[���o�b�N
    // �i�G�̃J�v�Z�����𖳎�����j
    struct MapOnlyRayCallback : public btCollisionWorld::ClosestRayResultCallback
    {
        btRigidBody* mapBody;

        MapOnlyRayCallback(const btVector3& from, const btVector3& to, btRigidBody* map)
            : ClosestRayResultCallback(from, to), mapBody(map) {
        }

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
            bool normalInWorldSpace) override
        {
            // �}�b�v���b�V���ȊO�͖���
            if (rayResult.m_collisionObject != mapBody)
                return 1.0f;

            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    MapOnlyRayCallback rayCallback(rayFrom, rayTo, m_mapMeshBody);
    m_dynamicsWorld->rayTest(rayFrom, rayTo, rayCallback);

    if (rayCallback.hasHit())
    {
        return rayCallback.m_hitPointWorld.getY();
    }

    return defaultY;
}

// ============================================================
// GetFloorHeightBelow - �w��Y���W��艺�̏������o
//
// �y�Ⴂ�zGetMeshFloorHeight �� Y=100 ���猂���ߓV��Ƀq�b�g����B
//   ������� startY ���牺������T���̂ŁA����̓V��𖳎��ł���B
// �y�p�r�z�W�����v���̃v���C���[�̒��n����
// ============================================================
float Game::GetFloorHeightBelow(float x, float startY, float z)
{
    if (!m_dynamicsWorld || !m_mapMeshBody) return -9999.0f;

    // ���C�̎n�_�F�����̏�����i�n�ʂɖ��܂��Ă鎞���E����悤�Ɂj
    constexpr float RAY_MARGIN = 0.5f;
    btVector3 rayFrom(x, startY + RAY_MARGIN, z);
    btVector3 rayTo(x, -100.0f, z);

    // �}�b�v���b�V�������ɓ�����R�[���o�b�N�iGetMeshFloorHeight�Ɠ����d�g�݁j
    struct MapOnlyRayCallback : public btCollisionWorld::ClosestRayResultCallback
    {
        btRigidBody* mapBody;

        MapOnlyRayCallback(const btVector3& from, const btVector3& to, btRigidBody* map)
            : ClosestRayResultCallback(from, to), mapBody(map) {
        }

        btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,
            bool normalInWorldSpace) override
        {
            if (rayResult.m_collisionObject != mapBody)
                return 1.0f;   // �}�b�v�ȊO�͖���
            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
    };

    MapOnlyRayCallback rayCallback(rayFrom, rayTo, m_mapMeshBody);
    m_dynamicsWorld->rayTest(rayFrom, rayTo, rayCallback);

    if (rayCallback.hasHit())
    {
        return rayCallback.m_hitPointWorld.getY();
    }

    return -9999.0f;   // ����������Ȃ�
}


// ============================================================
//  BuildNavGrid - �i�r�Q�[�V�����O���b�h�̎����\�z
//
//  �y�d�g�݁z
//  1. FBX���b�V����AABB����O���b�h�͈͂������v�Z
//  2. �S�}�X�ɂ��� CheckMeshCollision �ŕǔ���
//  3. �i���� maxStepHeight �𒴂���}�X���ǈ���
//
//  �y�p�t�H�[�}���X���Ӂz
//  �}�X���~CheckMeshCollision �Ȃ̂ŁA�N������1�񂾂��ĂԁB
//  cellSize=1.0m �ŊT�� 50�~60=3000�}�X �� ��0.5�b
// ============================================================
void Game::BuildNavGrid()
{
    // ���b�V���̎O�p�`�f�[�^����AABB���v�Z
    const auto& tris = m_mapSystem->GetCollisionTriangles();
    if (tris.empty()) {
        OutputDebugStringA("[NavGrid] No triangles, skipping.\n");
        return;
    }

    constexpr float FLOAT_MAX = 999999.0f;
    float meshMinX = FLOAT_MAX, meshMaxX = -FLOAT_MAX;
    float meshMinY = FLOAT_MAX, meshMaxY = -FLOAT_MAX;
    float meshMinZ = FLOAT_MAX, meshMaxZ = -FLOAT_MAX;

    for (const auto& tri : tris)
    {
        auto check = [&](const DirectX::XMFLOAT3& v) {
            if (v.x < meshMinX) meshMinX = v.x;
            if (v.x > meshMaxX) meshMaxX = v.x;
            if (v.y < meshMinY) meshMinY = v.y;
            if (v.y > meshMaxY) meshMaxY = v.y;
            if (v.z < meshMinZ) meshMinZ = v.z;
            if (v.z > meshMaxZ) meshMaxZ = v.z;
            };
        check(tri.v0); check(tri.v1); check(tri.v2);
    }

    // �}�[�W����//�i�[�̕��s���肪�r�؂�Ȃ��悤�Ɂj
    constexpr float GRID_MARGIN = 2.0f;
    constexpr float CELL_SIZE = 1.0f;
    constexpr float WALK_TEST_RADIUS = 0.4f;

    float mapMinX = meshMinX - GRID_MARGIN;
    float mapMaxX = meshMaxX + GRID_MARGIN;
    float mapMinZ = meshMinZ - GRID_MARGIN;
    float mapMaxZ = meshMaxZ + GRID_MARGIN;

    // �n�ʂ̍��� = AABB�̉��[���班����i�ǂ̓��̂�����j
    constexpr float TEST_HEIGHT_OFFSET = 1.0f;
    float testY = meshMinY + TEST_HEIGHT_OFFSET;

    {
        char buf[512];
        sprintf_s(buf, "[NavGrid] Using mesh AABB: X(%.1f~%.1f) Z(%.1f~%.1f) testY=%.1f\n",
            mapMinX, mapMaxX, mapMinZ, mapMaxZ, testY);
        OutputDebugStringA(buf);
    }

    m_navGrid.Initialize(mapMinX, mapMaxX, mapMinZ, mapMaxZ,
        CELL_SIZE, WALK_TEST_RADIUS);

    if (!m_mapMeshBody)
    {
        OutputDebugStringA("[NavGrid] WARNING: No mesh collider! Grid is all walkable.\n");
        return;
    }

    // --- �S�}�X��1���`�F�b�N ---
    int blockedCount = 0;
    int width = m_navGrid.GetWidth();
    int height = m_navGrid.GetHeight();

    float baseGroundY = GetMeshFloorHeight(0.0f, 0.0f, meshMinY);
    constexpr float MAX_STEP_HEIGHT = 1.5f; // ����ȏ�̒i���͓o��Ȃ�

    {
        char buf[256];
        sprintf_s(buf, "[NavGrid] Base ground Y: %.2f, max step: %.1f\n",
            baseGroundY, MAX_STEP_HEIGHT);
        OutputDebugStringA(buf);
    }

    for (int gz = 0; gz < height; gz++)
    {
        for (int gx = 0; gx < width; gx++)
        {
            float wx = m_navGrid.GridToWorldX(gx);
            float wz = m_navGrid.GridToWorldZ(gz);

            bool blocked = false;

            // �ǂ����邩�i��vs���b�V���Փ˔���j
            DirectX::XMFLOAT3 testPos = { wx, testY, wz };
            if (CheckMeshCollision(testPos, WALK_TEST_RADIUS))
            {
                blocked = true;
            }

            // �n�ʂ��������Ȃ����i���፷����j
            if (!blocked)
            {
                float cellGroundY = GetMeshFloorHeight(wx, wz, baseGroundY);
                if (cellGroundY > baseGroundY + MAX_STEP_HEIGHT)
                {
                    blocked = true;
                }
            }

            if (blocked)
            {
                m_navGrid.SetBlocked(gx, gz, true);
                blockedCount++;
            }
        }
    }

    char buf[256];
    sprintf_s(buf, "[NavGrid] Built: %d/%d cells blocked (%.1f%%)\n",
        blockedCount, width * height,
        100.0f * blockedCount / (width * height));
    OutputDebugStringA(buf);
}

// ============================================================
//  UpdateEnemyPhysicsBody - �G��Bullet���̈ʒu�𓯊�
//
//  �y���t���[���ĂԁzEnemySystem���G�𓮂�������A
//  Bullet���̍��̈ʒu�����킹��i�t�ł͂Ȃ��I�j
//  �Q�[�����W�b�N �� Bullet���� �̈���ʍs�B
// ============================================================
void Game::UpdateEnemyPhysicsBody(Enemy& enemy)
{
    auto it = m_enemyPhysicsBodies.find(enemy.id);
    if (it == m_enemyPhysicsBodies.end())
        return;

    btRigidBody* body = it->second;

    // �f�o�b�O�p�q�b�g�{�b�N�X�ƃf�t�H���g�l��؂�ւ�
    EnemyTypeConfig config;
    if (m_useDebugHitboxes)
    {
        switch (enemy.type)
        {
        case EnemyType::NORMAL:  config = m_normalConfigDebug;  break;
        case EnemyType::RUNNER:  config = m_runnerConfigDebug;  break;
        case EnemyType::TANK:    config = m_tankConfigDebug;    break;
        case EnemyType::MIDBOSS: config = m_midbossConfigDebug; break;
        case EnemyType::BOSS:    config = m_bossConfigDebug;    break;
        default: config = GetEnemyConfig(enemy.type); break;
        }
    }
    else
    {
        config = GetEnemyConfig(enemy.type);
    }

    float totalHeight = config.bodyHeight;

    // �����ʒu + �����̔��� = �̂̒��S
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        enemy.position.x,
        enemy.position.y + totalHeight / 2.0f,
        enemy.position.z
    ));

    body->setWorldTransform(transform);

    if (body->getMotionState())
    {
        body->getMotionState()->setWorldTransform(transform);
    }
}

// ============================================================
//  RemoveEnemyPhysicsBody - �G�̍��̂����[���h����폜
//
//  �y�Ăяo���^�C�~���O�z�G�����S����ClearDeadEnemies�����O
//  �y�������Ǘ��zshape, motionState, body��3��delete
// ============================================================
void Game::RemoveEnemyPhysicsBody(int enemyID)
{
    auto it = m_enemyPhysicsBodies.find(enemyID);
    if (it == m_enemyPhysicsBodies.end())
        return;

    btRigidBody* body = it->second;
    m_dynamicsWorld->removeRigidBody(body);

    delete body->getMotionState();
    delete body->getCollisionShape();
    delete body;

    m_enemyPhysicsBodies.erase(it);
}

// ============================================================
//  UpdatePhysics - �����V�~�����[�V������1�X�e�b�v���s
//
//  �y���Ӂz�ߐڃ`���[�W�̎����񕜂������Ɋ܂܂�Ă���
//  �i�{����UpdatePlaying�Ɉڂ��ׂ������A�����X�V�Ɠ��^�C�~���O��
//   ���s����K�v�����邽�ߎb��I�ɂ����ɔz�u�j
// ============================================================
void Game::UpdatePhysics(float deltaTime)
{
    if (m_dynamicsWorld)
    {
        // �����V�~�����[�V������1�X�e�b�v�i�߂�
        // maxSubSteps=10: �t���[���������Ă��ő�10�񕪂܂Œǂ���
        constexpr int MAX_SUB_STEPS = 10;
        m_dynamicsWorld->stepSimulation(deltaTime, MAX_SUB_STEPS);
        UpdateGibs(deltaTime);

        // �ߐڃ`���[�W������
        if (m_meleeCharges < m_meleeMaxCharges)
        {
            m_meleeRechargeTimer += deltaTime;
            if (m_meleeRechargeTimer >= m_meleeRechargeTime)
            {
                m_meleeCharges++;
                m_meleeRechargeTimer = 0.0f;
            }
        }
    }
}

// ============================================================
//  DrawNavGridDebug - �i�r�O���b�h�̕ǃ}�X���f�o�b�O�`��
//
//  �y�\�������zm_debugDrawNavGrid == true �̂Ƃ�
//  �y�œK���z�v���C���[����25m�ȓ��̃}�X�����`��
//    �i2���r�� sqrt ������j
// ============================================================
void Game::DrawNavGridDebug(DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
{
    if (!m_debugDrawNavGrid || !m_navGrid.IsReady()) return;

    int width = m_navGrid.GetWidth();
    int height = m_navGrid.GetHeight();

    DirectX::XMFLOAT3 pPos = m_player->GetPosition();

    // �`��͈͂̒萔���isqrt�������2���r�j
    constexpr float DRAW_RANGE = 25.0f;
    constexpr float DRAW_RANGE_SQ = DRAW_RANGE * DRAW_RANGE;

    for (int gz = 0; gz < height; gz++)
    {
        for (int gx = 0; gx < width; gx++)
        {
            if (!m_navGrid.IsBlocked(gx, gz)) continue;

            float wx = m_navGrid.GridToWorldX(gx);
            float wz = m_navGrid.GridToWorldZ(gz);

            // �����`�F�b�N�i2���r��sqrt����j
            float dx = wx - pPos.x;
            float dz = wz - pPos.z;
            if (dx * dx + dz * dz > DRAW_RANGE_SQ) continue;

            // ���̃}�X�̒n�ʂ̍������擾�i�ǂ̏���W�ɂ���j
            float cellY = GetMeshFloorHeight(wx, wz, pPos.y - Player::EYE_HEIGHT);

            float cellSize = m_navGrid.GetCellSize();

            // �Ԃ�����n�ʂ���3m��܂ŗ��Ă�
            constexpr float DEBUG_PILLAR_HEIGHT = 3.0f;
            constexpr float DEBUG_CELL_SCALE = 0.8f;

            DirectX::XMMATRIX world =
                DirectX::XMMatrixScaling(cellSize * DEBUG_CELL_SCALE, DEBUG_PILLAR_HEIGHT, cellSize * DEBUG_CELL_SCALE) *
                DirectX::XMMatrixTranslation(wx, cellY + DEBUG_PILLAR_HEIGHT / 2.0f, wz);

            constexpr float DEBUG_WALL_ALPHA = 0.5f;
            m_cube->Draw(world, view, proj,
                DirectX::XMVECTORF32{ 1.0f, 0.0f, 0.0f, DEBUG_WALL_ALPHA });
        }
    }
}