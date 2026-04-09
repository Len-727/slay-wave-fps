// ============================================================
//  GameSlice.cpp
//  ���b�V���ؒf�E���ЁiGib�j�����E�ؒf�s�[�X�`��
//
//  �y�Ӗ��z
//  - �������ɂ�郊�A���^�C�����b�V���ؒf
//  - �ؒf�s�[�X�̕������Z�i�d�́E��]�E���Փˁj
//  - Bullet Physics�x�[�X�̓��ЁiGib�j�����E�X�V�E�`��
//  - �ؒf�p�����_�����O���\�[�X�̏�����
//
//  �y�݌v�Ӑ}�z
//  �S�A�\���̓Q�[���́u��G��v�ɒ�������d�v�ȗv�f�B
//  MeshSlicer�N���X�����w�I�Ȑؒf������S�����A
//  ���̃t�@�C����GPU�o�b�t�@�쐬�ƕ������Z��S������B
//  �ؒf �� �p�[�c���� �� �����V�~�����[�V���� �� �`�� ��
//  ��A�̃p�C�v���C��������1�t�@�C���ɏW�񂳂�Ă���B
//
//  �y�Z�p�I�ȃ|�C���g�z
//  - �ؒf�ʂ̖@���͏��̔�s�������瓮�I�Ɍv�Z
//  - �A�j���[�V�����K�p�ςݒ��_���擾���Ă���ؒf
//  - ���n�������Ђ͕������[���h���珜���iCPU���׌y���j
// ============================================================

#include "Game.h"
#include "MeshSlicer.h"
#include <algorithm>

using namespace DirectX;

// ============================================================
//  �萔��`
// ============================================================
namespace SliceConstants
{
    // �ؒf�s�[�X�̎����ƕ����p�����[�^
    constexpr float PIECE_LIFETIME = 5.0f;          // �ؒf�s�[�X�̐������ԁi�b�j
    constexpr float PIECE_FADE_THRESHOLD = 1.0f;    // �t�F�[�h�J�n����c�����
    constexpr float GRAVITY = -9.8f;                // �d�͉����x (m/s?)
    constexpr float FLOOR_FRICTION = 0.9f;          // ���Ƃ̖��C�i���x�ɏ�Z�j
    constexpr float ANGULAR_DAMPING = 0.95f;        // ��]�̌�����
    constexpr float MAX_ANGULAR_VEL = 5.0f;         // �ő��]���x�irad/s�j

    // ���ЁiGib�j�̃p�����[�^
    constexpr int   GIB_MAX_COUNT = 100;            // �������݂̏��
    constexpr float GIB_RESTITUTION = 0.3f;         // �����W���i�����e�ށj
    constexpr float GIB_FRICTION = 0.8f;            // ���C�W���i�n�ʂŎ~�܂�j
    constexpr float GIB_LAND_HEIGHT = 0.5f;         // ���n����̍���臒l
    constexpr float GIB_LAND_SPEED = 3.0f;          // ���n����̑��x臒l
    constexpr float GIB_FADE_THRESHOLD = 1.0f;      // �t�F�[�h�J�n����c�����

    // �G���f���̃X�P�[���i�`�掞�ƈ�v�����邱�Ɓj
    constexpr float MODEL_SCALE = 0.01f;

    // �ؒf�ʂ̌X���W���i����XZ�������ؒf�ʂɗ^����e���j
    constexpr float SLICE_TILT_FACTOR = 0.3f;

    // �ؒf�ʒu�̃N�����v�͈́i�̂�20%?80%�Ő؂�j
    constexpr float SLICE_MIN_RATIO = 0.2f;
    constexpr float SLICE_MAX_RATIO = 0.8f;

    // AABB�v�Z�p�̃Z���`�l���l
    constexpr float FLOAT_SENTINEL = 99999.0f;

    // �{�[����ԂɈ��k���ꂽ���b�V���̔���臒l
    constexpr float COMPRESSED_MESH_THRESHOLD = 0.1f;
}

// ============================================================
//  InitSliceRendering - �ؒf�`��p�̃G�t�F�N�g�ƃ��C�A�E�g�쐬
//
//  �y�Ăяo���zGame::CreateRenderResources() ����1��
//  �y�쐬������́z
//  - �e�N�X�`������p BasicEffect + InputLayout
//  - �e�N�X�`���Ȃ��p BasicEffect + InputLayout�i�f�ʗp�j
//  - ���ʕ`��p�̃��X�^���C�U�X�e�[�g
// ============================================================
void Game::InitSliceRendering()
{
    // �e�N�X�`������ Effect�i�ؒf�ʈȊO�̌��̃��b�V�������j
    m_sliceEffectTex = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_sliceEffectTex->SetLightingEnabled(true);
    m_sliceEffectTex->SetLightEnabled(0, true);
    m_sliceEffectTex->SetLightDirection(0, XMVectorSet(0, -1, 0.3f, 0));
    m_sliceEffectTex->SetLightDiffuseColor(0, XMVectorSet(1, 0.9f, 0.8f, 1));
    m_sliceEffectTex->SetAmbientLightColor(XMVectorSet(0.4f, 0.4f, 0.4f, 1));
    m_sliceEffectTex->SetTextureEnabled(true);

    // �e�N�X�`���Ȃ� Effect�i�f�ʂ̊W�|���S���p�j
    m_sliceEffectNoTex = std::make_unique<DirectX::BasicEffect>(m_d3dDevice.Get());
    m_sliceEffectNoTex->SetLightingEnabled(true);
    m_sliceEffectNoTex->SetLightEnabled(0, true);
    m_sliceEffectNoTex->SetLightDirection(0, XMVectorSet(0, -1, 0.3f, 0));
    m_sliceEffectNoTex->SetLightDiffuseColor(0, XMVectorSet(1, 0.9f, 0.8f, 1));
    m_sliceEffectNoTex->SetAmbientLightColor(XMVectorSet(0.4f, 0.4f, 0.4f, 1));
    m_sliceEffectNoTex->SetTextureEnabled(false);

    // InputLayout�i�e�N�X�`������j
    void const* shaderByteCode;
    size_t byteCodeLength;
    m_sliceEffectTex->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        VertexPositionNormalTexture::InputElements,
        VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_sliceInputLayoutTex.ReleaseAndGetAddressOf());

    // InputLayout�i�e�N�X�`���Ȃ��j
    m_sliceEffectNoTex->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
    m_d3dDevice->CreateInputLayout(
        VertexPositionNormalTexture::InputElements,
        VertexPositionNormalTexture::InputElementCount,
        shaderByteCode, byteCodeLength,
        m_sliceInputLayoutNoTex.ReleaseAndGetAddressOf());

    // ���ʕ`�惉�X�^���C�U�i�ؒf�ʂ͗����������邽�߁j
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    m_d3dDevice->CreateRasterizerState(&rsDesc, m_sliceNoCullRS.ReleaseAndGetAddressOf());
}

// ============================================================
//  TestMeshSlice - �ؒf�A���S���Y���̓���e�X�g
//
//  �y�p�r�z�f�o�b�O��p�B2x2x2�̗����̂�Y=0�Ő����ؒf���A
//  �ؒf���ʂ�OutputDebugStringA�ŏo�͂���B
//  �Q�[���v���C���͌Ă΂�Ȃ��B
// ============================================================
void Game::TestMeshSlice()
{
    // �e�X�g�p�̔����b�V���i6�ʁ~4���_�j
    std::vector<SliceVertex> vertices = {
        {{ -1, -1,  1 }, { 0, 0, 1 }, { 0, 1 }}, {{  1, -1,  1 }, { 0, 0, 1 }, { 1, 1 }},
        {{  1,  1,  1 }, { 0, 0, 1 }, { 1, 0 }}, {{ -1,  1,  1 }, { 0, 0, 1 }, { 0, 0 }},
        {{  1, -1, -1 }, { 0, 0,-1 }, { 0, 1 }}, {{ -1, -1, -1 }, { 0, 0,-1 }, { 1, 1 }},
        {{ -1,  1, -1 }, { 0, 0,-1 }, { 1, 0 }}, {{  1,  1, -1 }, { 0, 0,-1 }, { 0, 0 }},
        {{ -1,  1,  1 }, { 0, 1, 0 }, { 0, 1 }}, {{  1,  1,  1 }, { 0, 1, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 0, 1, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, { 0, 1, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, { 0,-1, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 0,-1, 0 }, { 1, 1 }},
        {{  1, -1,  1 }, { 0,-1, 0 }, { 1, 0 }}, {{ -1, -1,  1 }, { 0,-1, 0 }, { 0, 0 }},
        {{  1, -1,  1 }, { 1, 0, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 1, 0, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 1, 0, 0 }, { 1, 0 }}, {{  1,  1,  1 }, { 1, 0, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, {-1, 0, 0 }, { 0, 1 }}, {{ -1, -1,  1 }, {-1, 0, 0 }, { 1, 1 }},
        {{ -1,  1,  1 }, {-1, 0, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, {-1, 0, 0 }, { 0, 0 }},
    };
    std::vector<uint32_t> indices = {
         0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,   8, 9,10,  8,10,11,
        12,13,14, 12,14,15,  16,17,18, 16,18,19,  20,21,22, 20,22,23,
    };

    XMFLOAT3 planePoint = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 planeNormal = { 0.0f, 1.0f, 0.0f };

    OutputDebugStringA("[TEST] Slicing cube at Y=0...\n");
    SliceResult result = MeshSlicer::Slice(vertices, indices, planePoint, planeNormal);

    if (result.success)
    {
        char buf[512];
        sprintf_s(buf,
            "[TEST] Slice SUCCESS! Upper: %zu verts/%zu tris, Lower: %zu verts/%zu tris\n",
            result.upperVertices.size(), result.upperIndices.size() / 3,
            result.lowerVertices.size(), result.lowerIndices.size() / 3);
        OutputDebugStringA(buf);
    }
    else
    {
        OutputDebugStringA("[TEST] Slice FAILED - no split occurred\n");
    }
}

// ============================================================
//  CreateSlicedPiece - �ؒf���ʂ���GPU�o�b�t�@���쐬
//
//  �y�����zvertices/indices: MeshSlicer�̏o��
//         position: ���[���h��̏����ʒu
//         velocity: �����x�i������ѕ����j
//  �y�߂�l�zSlicedPiece�\���́iactive=false�Ȃ�쐬���s�j
// ============================================================
Game::SlicedPiece Game::CreateSlicedPiece(
    const std::vector<SliceVertex>& vertices,
    const std::vector<uint32_t>& indices,
    XMFLOAT3 position,
    XMFLOAT3 velocity)
{
    SlicedPiece piece;
    piece.active = false;

    if (vertices.empty() || indices.empty()) return piece;

    // SliceVertex �� DirectXTK��VertexPositionNormalTexture �ɕϊ�
    std::vector<VertexPositionNormalTexture> dxVertices;
    dxVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        VertexPositionNormalTexture dxv;
        dxv.position = v.position;
        dxv.normal = v.normal;
        dxv.textureCoordinate = v.uv;
        dxVertices.push_back(dxv);
    }

    // ���_�o�b�t�@�쐬
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = (UINT)(dxVertices.size() * sizeof(VertexPositionNormalTexture));
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = dxVertices.data();

    HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbData,
        piece.vertexBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return piece;

    // �C���f�b�N�X�o�b�t�@�쐬
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices.data();

    hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibData,
        piece.indexBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr)) return piece;

    piece.indexCount = (UINT)indices.size();
    piece.position = position;
    piece.velocity = velocity;
    piece.rotation = { 0, 0, 0 };

    // �����_���ȉ�]���x�i�ؒf���̃C���p�N�g�����o���j
    piece.angularVel = {
        ((float)rand() / RAND_MAX - 0.5f) * SliceConstants::MAX_ANGULAR_VEL,
        ((float)rand() / RAND_MAX - 0.5f) * SliceConstants::MAX_ANGULAR_VEL,
        ((float)rand() / RAND_MAX - 0.5f) * SliceConstants::MAX_ANGULAR_VEL
    };

    piece.lifetime = SliceConstants::PIECE_LIFETIME;
    piece.active = true;

    return piece;
}

// ============================================================
//  ExecuteSliceTest - �f�o�b�O�p�ؒf�e�X�g���s
//
//  �y�Ăяo���z�L�[���͂Ŕ����i�f�o�b�O�p�j
//  �v���C���[�̑O��3m�ɃL���[�u�𐶐����Đؒf����
// ============================================================
void Game::ExecuteSliceTest()
{
    XMFLOAT3 playerPos = m_player->GetPosition();
    XMFLOAT3 playerRot = m_player->GetRotation();
    float fx = sinf(playerRot.y);
    float fz = cosf(playerRot.y);

    constexpr float TEST_DISTANCE = 3.0f;
    constexpr float TEST_BODY_CENTER_OFFSET = 0.9f;
    float cubeX = playerPos.x + fx * TEST_DISTANCE;
    float cubeY = playerPos.y - TEST_BODY_CENTER_OFFSET;
    float cubeZ = playerPos.z + fz * TEST_DISTANCE;

    // �e�X�g�p�L���[�u�̒��_�f�[�^�iTestMeshSlice�Ɠ���j
    std::vector<SliceVertex> vertices = {
        {{ -1, -1,  1 }, { 0, 0, 1 }, { 0, 1 }}, {{  1, -1,  1 }, { 0, 0, 1 }, { 1, 1 }},
        {{  1,  1,  1 }, { 0, 0, 1 }, { 1, 0 }}, {{ -1,  1,  1 }, { 0, 0, 1 }, { 0, 0 }},
        {{  1, -1, -1 }, { 0, 0,-1 }, { 0, 1 }}, {{ -1, -1, -1 }, { 0, 0,-1 }, { 1, 1 }},
        {{ -1,  1, -1 }, { 0, 0,-1 }, { 1, 0 }}, {{  1,  1, -1 }, { 0, 0,-1 }, { 0, 0 }},
        {{ -1,  1,  1 }, { 0, 1, 0 }, { 0, 1 }}, {{  1,  1,  1 }, { 0, 1, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 0, 1, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, { 0, 1, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, { 0,-1, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 0,-1, 0 }, { 1, 1 }},
        {{  1, -1,  1 }, { 0,-1, 0 }, { 1, 0 }}, {{ -1, -1,  1 }, { 0,-1, 0 }, { 0, 0 }},
        {{  1, -1,  1 }, { 1, 0, 0 }, { 0, 1 }}, {{  1, -1, -1 }, { 1, 0, 0 }, { 1, 1 }},
        {{  1,  1, -1 }, { 1, 0, 0 }, { 1, 0 }}, {{  1,  1,  1 }, { 1, 0, 0 }, { 0, 0 }},
        {{ -1, -1, -1 }, {-1, 0, 0 }, { 0, 1 }}, {{ -1, -1,  1 }, {-1, 0, 0 }, { 1, 1 }},
        {{ -1,  1,  1 }, {-1, 0, 0 }, { 1, 0 }}, {{ -1,  1, -1 }, {-1, 0, 0 }, { 0, 0 }},
    };
    std::vector<uint32_t> indices = {
         0, 1, 2,  0, 2, 3,   4, 5, 6,  4, 6, 7,   8, 9,10,  8,10,11,
        12,13,14, 12,14,15,  16,17,18, 16,18,19,  20,21,22, 20,22,23,
    };

    XMFLOAT3 planePoint = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 planeNormal = { 0.0f, 1.0f, 0.0f };

    SliceResult result = MeshSlicer::Slice(vertices, indices, planePoint, planeNormal);

    if (result.success)
    {
        // �㔼��: ��ɐ������
        SlicedPiece upper = CreateSlicedPiece(
            result.upperVertices, result.upperIndices,
            { cubeX, cubeY + 0.5f, cubeZ },
            { 0.0f, 4.0f, 0.0f });
        if (upper.active) m_slicedPieces.push_back(std::move(upper));

        // ������: �������ɗ�����
        SlicedPiece lower = CreateSlicedPiece(
            result.lowerVertices, result.lowerIndices,
            { cubeX, cubeY - 0.5f, cubeZ },
            { 1.0f, -1.0f, 0.0f });
        if (lower.active) m_slicedPieces.push_back(std::move(lower));

        OutputDebugStringA("[SLICE] Cube sliced! 2 pieces created.\n");
    }
}

// ============================================================
//  SliceEnemyWithShield - �������œG�����A���^�C���ؒf
//
//  �y�����t���[�z
//  1. �G�^�C�v�ɉ�����3D���f����I��
//  2. �S���b�V���̒��_�����W�i�{�[���K�p or �����_�j
//  3. ���̋O������ؒf�ʂ��v�Z
//  4. MeshSlicer::Slice() �Őؒf���s
//  5. �㉺2�p�[�c��GPU�o�b�t�@���쐬
//  6. ���G�t�F�N�g�𔭐�
//
//  �y�Ȃ�GetAnimatedVertices���g���H�z
//  FBX���f���̒��_�̓{�[����ԂɈ��k����Ă���A
//  ���̒��_�f�[�^�ł͐������ؒf�ʒu�������Ȃ��B
//  ���݂̃A�j���[�V�����|�[�Y��K�p�������_���g�����ƂŁA
//  �v���C���[�����Ă���p���ʂ�ɐؒf�����B
// ============================================================
void Game::SliceEnemyWithShield(Enemy& enemy, XMFLOAT3 shieldDir)
{
    // --- �G�^�C�v�ɑΉ����郂�f����I�� ---
    Model* enemyModel = nullptr;
    switch (enemy.type)
    {
    case EnemyType::NORMAL: enemyModel = m_enemyModel.get(); break;
    case EnemyType::RUNNER: enemyModel = m_runnerModel.get(); break;
    case EnemyType::TANK:   enemyModel = m_tankModel.get(); break;
    default: return;  // BOSS/MIDBOSS�͐ؒf���Ȃ��i���o��̗��R�j
    }
    if (!enemyModel || enemyModel->GetMeshes().empty()) return;

    // --- ���f���̑S���b�V�����璸�_�����W ---
    std::vector<SliceVertex> allVertices;
    std::vector<uint32_t> allIndices;

    for (int mi = 0; mi < (int)enemyModel->GetMeshes().size(); mi++)
    {
        const auto& mesh = enemyModel->GetMeshes()[mi];
        uint32_t baseVertex = (uint32_t)allVertices.size();

        // �����_��Y�͈͂��`�F�b�N�i�{�[�����k����Ă��邩����j
        float rawMinY = SliceConstants::FLOAT_SENTINEL;
        float rawMaxY = -SliceConstants::FLOAT_SENTINEL;
        for (const auto& mv : mesh.vertices)
        {
            float y = mv.position.y * SliceConstants::MODEL_SCALE;
            if (y < rawMinY) rawMinY = y;
            if (y > rawMaxY) rawMaxY = y;
        }
        float rawHeight = rawMaxY - rawMinY;

        // Y�͈͂��\���Ȃ琶�̒��_���g�p�A
        // ����������i�{�[����ԂɈ��k�j�Ȃ�o�C���h�|�[�Y���g�p
        bool needBoneTransform = (rawHeight < SliceConstants::COMPRESSED_MESH_THRESHOLD);

        if (needBoneTransform)
        {
            std::vector<ModelVertex> bindVerts = enemyModel->GetBindPoseVertices(mi);
            for (const auto& mv : bindVerts)
            {
                SliceVertex sv;
                sv.position.x = mv.position.x * SliceConstants::MODEL_SCALE;
                sv.position.y = mv.position.y * SliceConstants::MODEL_SCALE;
                sv.position.z = mv.position.z * SliceConstants::MODEL_SCALE;
                sv.normal = mv.normal;
                sv.uv = mv.texCoord;
                allVertices.push_back(sv);
            }
        }
        else
        {
            std::vector<ModelVertex> animVerts =
                enemyModel->GetAnimatedVertices(mi, enemy.currentAnimation, enemy.animationTime);
            for (const auto& mv : animVerts)
            {
                SliceVertex sv;
                sv.position.x = mv.position.x * SliceConstants::MODEL_SCALE;
                sv.position.y = mv.position.y * SliceConstants::MODEL_SCALE;
                sv.position.z = mv.position.z * SliceConstants::MODEL_SCALE;
                sv.normal = mv.normal;
                sv.uv = mv.texCoord;
                allVertices.push_back(sv);
            }
        }

        for (const auto& idx : mesh.indices)
            allIndices.push_back(baseVertex + idx);
    }

    if (allVertices.empty()) return;

    // --- �ؒf�ʂ̌v�Z ---
    EnemyTypeConfig config = GetEnemyConfig(enemy.type);

    // �����G�̃��C�L���X�g�Ő��m�ȏՓ˓_���擾
    float hitRelativeY = config.bodyHeight * 0.4f;  // �f�t�H���g�i��������j

    if (m_dynamicsWorld)
    {
        btVector3 rayFrom(m_thrownShieldPos.x, m_thrownShieldPos.y, m_thrownShieldPos.z);
        btVector3 rayTo(enemy.position.x,
            enemy.position.y + config.bodyHeight * 0.5f,
            enemy.position.z);

        // �Ώۂ̓G�̃J�v�Z�������ɓ��Ă�J�X�^���R�[���o�b�N
        struct EnemyRayCallback : public btCollisionWorld::ClosestRayResultCallback
        {
            int targetID;
            EnemyRayCallback(const btVector3& from, const btVector3& to, int id)
                : ClosestRayResultCallback(from, to), targetID(id) {
            }

            btScalar addSingleResult(btCollisionWorld::LocalRayResult& result,
                bool normalInWorldSpace) override
            {
                void* ptr = result.m_collisionObject->getUserPointer();
                if (!ptr) return 1.0f;
                int id = (int)(intptr_t)ptr - 1;
                if (id != targetID) return 1.0f;
                return ClosestRayResultCallback::addSingleResult(result, normalInWorldSpace);
            }
        };

        EnemyRayCallback callback(rayFrom, rayTo, enemy.id);
        m_dynamicsWorld->rayTest(rayFrom, rayTo, callback);

        if (callback.hasHit())
            hitRelativeY = callback.m_hitPointWorld.getY() - enemy.position.y;
    }

    // --- ���f����Y�͈͂��擾���ăX���C�X���������� ---
    float modelMinY = SliceConstants::FLOAT_SENTINEL;
    float modelMaxY = -SliceConstants::FLOAT_SENTINEL;
    for (const auto& v : allVertices)
    {
        if (v.position.y < modelMinY) modelMinY = v.position.y;
        if (v.position.y > modelMaxY) modelMaxY = v.position.y;
    }
    float modelHeight = modelMaxY - modelMinY;

    // hitRelativeY�����f����Y�͈͂Ƀ}�b�s���O
    float hitRatio = hitRelativeY / config.bodyHeight;
    hitRatio = max(SliceConstants::SLICE_MIN_RATIO, min(SliceConstants::SLICE_MAX_RATIO, hitRatio));
    float sliceHeight = modelMinY + modelHeight * hitRatio;

    // --- �ؒf�ʂ̖@���i���̔�s�����ŏ����X����j ---
    XMFLOAT3 sliceNormal;
    sliceNormal.x = -shieldDir.z * SliceConstants::SLICE_TILT_FACTOR;
    sliceNormal.y = 1.0f;  // ��{�͐����ؒf
    sliceNormal.z = shieldDir.x * SliceConstants::SLICE_TILT_FACTOR;

    // �@���𐳋K��
    float nLen = sqrtf(sliceNormal.x * sliceNormal.x +
        sliceNormal.y * sliceNormal.y +
        sliceNormal.z * sliceNormal.z);
    sliceNormal.x /= nLen;
    sliceNormal.y /= nLen;
    sliceNormal.z /= nLen;

    XMFLOAT3 planePoint = { 0.0f, sliceHeight, 0.0f };

    // --- �ؒf���s ---
    SliceResult result = MeshSlicer::Slice(allVertices, allIndices, planePoint, sliceNormal);
    if (!result.success) return;

    // --- 2�p�[�c�𐶐� ---
    float centerY = enemy.position.y + config.bodyHeight * 0.5f;
    ID3D11ShaderResourceView* tex = enemyModel->GetTexture();

    // ��p�[�c�i�@������ + �������ɐ�����ԁj
    XMFLOAT3 upperVel = {
        sliceNormal.x * 3.0f + shieldDir.x * 2.0f,
        3.0f + sliceNormal.y * 2.0f,
        sliceNormal.z * 3.0f + shieldDir.z * 2.0f
    };
    SlicedPiece upper = CreateSlicedPiece(
        result.upperVertices, result.upperIndices,
        { enemy.position.x, centerY, enemy.position.z }, upperVel);
    if (upper.active)
    {
        upper.texture = tex;
        m_slicedPieces.push_back(std::move(upper));
    }

    // ���p�[�c�i�@���̔��Ε����ɂ������|���j
    XMFLOAT3 lowerVel = {
        -sliceNormal.x * 2.0f,
        -1.0f,
        -sliceNormal.z * 2.0f
    };
    SlicedPiece lower = CreateSlicedPiece(
        result.lowerVertices, result.lowerIndices,
        { enemy.position.x, centerY, enemy.position.z }, lowerVel);
    if (lower.active)
    {
        lower.texture = tex;
        m_slicedPieces.push_back(std::move(lower));
    }

    // --- ���̓G���\���ɂ��Č��G�t�F�N�g ---
    enemy.isExploded = true;

    XMFLOAT3 hitPos = { enemy.position.x, centerY, enemy.position.z };
    m_gpuParticles->EmitSplash(hitPos, shieldDir, 60, 8.0f);
    m_gpuParticles->EmitMist(hitPos, 200, 4.0f);
    m_gpuParticles->Emit(hitPos, 100, 6.0f);
    m_bloodSystem->OnEnemyKilled(enemy.position, m_player->GetPosition());
}

// ============================================================
//  UpdateSlicedPieces - �ؒf�s�[�X�̕����X�V
//
//  �y���t���[���z�d�́E�ʒu�E��]���X�V���A���Ŏ~�߂�
//  �y�����Ǘ��zlifetime�؂�̃s�[�X��z�񂩂�폜
// ============================================================
void Game::UpdateSlicedPieces(float deltaTime)
{
    for (auto& piece : m_slicedPieces)
    {
        if (!piece.active) continue;

        // �d��
        piece.velocity.y += SliceConstants::GRAVITY * deltaTime;

        // �ʒu�X�V
        piece.position.x += piece.velocity.x * deltaTime;
        piece.position.y += piece.velocity.y * deltaTime;
        piece.position.z += piece.velocity.z * deltaTime;

        // ��]�X�V
        piece.rotation.x += piece.angularVel.x * deltaTime;
        piece.rotation.y += piece.angularVel.y * deltaTime;
        piece.rotation.z += piece.angularVel.z * deltaTime;

        // ���Ŏ~�܂�iGetMeshFloorHeight�Ŏ��ۂ̒n�ʍ������擾�j
        float floorY = GetMeshFloorHeight(piece.position.x, piece.position.z, piece.position.y);
        if (piece.position.y < floorY)
        {
            piece.position.y = floorY;
            piece.velocity.y = 0.0f;
            piece.velocity.x *= SliceConstants::FLOOR_FRICTION;
            piece.velocity.z *= SliceConstants::FLOOR_FRICTION;
            piece.angularVel.x *= SliceConstants::ANGULAR_DAMPING;
            piece.angularVel.y *= SliceConstants::ANGULAR_DAMPING;
            piece.angularVel.z *= SliceConstants::ANGULAR_DAMPING;
        }

        // ����
        piece.lifetime -= deltaTime;
        if (piece.lifetime <= 0.0f)
            piece.active = false;
    }

    // ���񂾃s�[�X��z�񂩂�폜�ierase-remove �C�f�B�I���j
    m_slicedPieces.erase(
        std::remove_if(m_slicedPieces.begin(), m_slicedPieces.end(),
            [](const SlicedPiece& p) { return !p.active; }),
        m_slicedPieces.end());
}

// ============================================================
//  DrawSlicedPieces - �ؒf�s�[�X�̕`��
//
//  �y�`������z�e�N�X�`������ƂȂ���Effect��؂�ւ�
//  �y�t�F�[�h�z�c�����1�b�ȉ��ŃA���t�@��������
// ============================================================
void Game::DrawSlicedPieces(XMMATRIX view, XMMATRIX proj)
{
    if (m_slicedPieces.empty()) return;
    if (!m_sliceEffectTex) return;

    // ���ʕ`��ON�i�ؒf�ʂ͗�����������j
    m_d3dContext->RSSetState(m_sliceNoCullRS.Get());

    UINT stride = sizeof(VertexPositionNormalTexture);
    UINT offset = 0;

    for (const auto& piece : m_slicedPieces)
    {
        if (!piece.active) continue;

        XMMATRIX world =
            XMMatrixRotationX(piece.rotation.x) *
            XMMatrixRotationY(piece.rotation.y) *
            XMMatrixRotationZ(piece.rotation.z) *
            XMMatrixTranslation(piece.position.x, piece.position.y, piece.position.z);

        // ������1�b�ȉ��Ńt�F�[�h�A�E�g
        float alpha = (piece.lifetime < SliceConstants::PIECE_FADE_THRESHOLD)
            ? piece.lifetime : 1.0f;

        if (piece.texture)
        {
            m_sliceEffectTex->SetTexture(piece.texture);
            m_sliceEffectTex->SetDiffuseColor(XMVectorSet(1, 1, 1, alpha));
            m_sliceEffectTex->SetWorld(world);
            m_sliceEffectTex->SetView(view);
            m_sliceEffectTex->SetProjection(proj);
            m_sliceEffectTex->Apply(m_d3dContext.Get());
            m_d3dContext->IASetInputLayout(m_sliceInputLayoutTex.Get());
        }
        else
        {
            m_sliceEffectNoTex->SetDiffuseColor(XMVectorSet(0.8f, 0.2f, 0.2f, alpha));
            m_sliceEffectNoTex->SetWorld(world);
            m_sliceEffectNoTex->SetView(view);
            m_sliceEffectNoTex->SetProjection(proj);
            m_sliceEffectNoTex->Apply(m_d3dContext.Get());
            m_d3dContext->IASetInputLayout(m_sliceInputLayoutNoTex.Get());
        }

        ID3D11Buffer* vb = piece.vertexBuffer.Get();
        m_d3dContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        m_d3dContext->IASetIndexBuffer(piece.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_d3dContext->DrawIndexed(piece.indexCount, 0, 0);
    }

    m_d3dContext->RSSetState(nullptr);
}

// ============================================================
//  SpawnGibs - Bullet Physics�œ��Ђ𐶐�
//
//  �y�d�g�݁z
//  �_�C�i�~�b�N���́imass>0�j��Box�𕡐��������A
//  �����_���ȕ����ɃC���p���X��^����B
//  Bullet���d�́E�ՓˁE��]�������v�Z���Ă����B
//
//  �y�T�C�Y���z�z
//  �ŏ���3�� = ��p�[�c�i���̕Ёj
//  ����4��   = ���p�[�c�i�葫�j
//  �c��      = ���p�[�c�i���Ёj
// ============================================================
void Game::SpawnGibs(XMFLOAT3 position, int count, float power)
{
    // ����𒴂�����Â��̂��폜�i�������Ǘ��j
    while (m_gibs.size() > SliceConstants::GIB_MAX_COUNT)
    {
        auto& old = m_gibs.front();
        if (old.body)
        {
            m_dynamicsWorld->removeRigidBody(old.body);
            delete old.body->getMotionState();
            delete old.body;
            delete old.shape;
        }
        m_gibs.erase(m_gibs.begin());
    }

    // ���Ђ̐F�p�^�[���i���̐�?�Â����F�j
    constexpr XMFLOAT4 GIB_COLORS[] = {
        { 0.85f, 0.10f, 0.05f, 1.0f },  // �Â���
        { 0.95f, 0.15f, 0.05f, 1.0f },  // ���̐�
        { 0.75f, 0.05f, 0.05f, 1.0f },  // �[����
        { 0.90f, 0.25f, 0.10f, 1.0f },  // ���F
        { 0.70f, 0.08f, 0.08f, 1.0f },  // �قڍ���
        { 1.00f, 0.20f, 0.10f, 1.0f },  // ���邢��
    };
    constexpr int GIB_COLOR_COUNT = sizeof(GIB_COLORS) / sizeof(GIB_COLORS[0]);

    // �T�C�Y�敪�̒萔
    constexpr int LARGE_PARTS = 3;
    constexpr int MEDIUM_PARTS = 7;

    for (int i = 0; i < count; i++)
    {
        // �T�C�Y�i�偨�������̏��Ő����j
        float size;
        if (i < LARGE_PARTS)
            size = 0.25f + (float)rand() / RAND_MAX * 0.2f;
        else if (i < MEDIUM_PARTS)
            size = 0.14f + (float)rand() / RAND_MAX * 0.12f;
        else
            size = 0.07f + (float)rand() / RAND_MAX * 0.08f;

        btCollisionShape* shape = new btBoxShape(btVector3(size, size, size));

        // �����ʒu�i�G�̈ʒu���烉���_���I�t�Z�b�g�j
        float offsetX = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
        float offsetY = 0.5f + (float)rand() / RAND_MAX * 1.0f;
        float offsetZ = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(
            position.x + offsetX,
            position.y + offsetY,
            position.z + offsetZ));

        // �����_����]
        btQuaternion rotation(
            (float)rand() / RAND_MAX * 3.14f,
            (float)rand() / RAND_MAX * 3.14f,
            (float)rand() / RAND_MAX * 3.14f);
        transform.setRotation(rotation);

        // �_�C�i�~�b�N���́imass > 0 �� �d�͂ŗ�����j
        btScalar mass = 0.5f + (float)rand() / RAND_MAX * 1.5f;
        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
        rbInfo.m_restitution = SliceConstants::GIB_RESTITUTION;
        rbInfo.m_friction = SliceConstants::GIB_FRICTION;

        btRigidBody* body = new btRigidBody(rbInfo);

        // �����I�ȗ͂�������i���ˏ� + ������j
        float angleXZ = ((float)rand() / RAND_MAX) * 6.28f;
        float upForce = power * (0.6f + (float)rand() / RAND_MAX * 0.8f);
        btVector3 impulse(
            cosf(angleXZ) * power * (0.5f + (float)rand() / RAND_MAX),
            upForce,
            sinf(angleXZ) * power * (0.5f + (float)rand() / RAND_MAX));
        body->applyCentralImpulse(impulse);

        // �����_����]�́i�O���O����鉉�o�j
        btVector3 torque(
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f,
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f,
            ((float)rand() / RAND_MAX - 0.5f) * power * 2.0f);
        body->applyTorqueImpulse(torque);

        m_dynamicsWorld->addRigidBody(body);

        // Gib�\���̂ɕۑ�
        Gib gib;
        gib.body = body;
        gib.shape = shape;
        gib.lifetime = 3.0f + (float)rand() / RAND_MAX * 2.0f;
        gib.size = size;
        gib.color = GIB_COLORS[i % GIB_COLOR_COUNT];
        gib.hasLanded = false;
        m_gibs.push_back(gib);
    }
}

// ============================================================
//  UpdateGibs - ���Ђ̍X�V�i�����Ǘ� + ���n�œK���j
//
//  �y���n�œK���z���x��臒l�ȉ��ɂȂ����畨�����[���h���珜���B
//  �ŏI�ʒu��ۑ����ĐÓI�ɕ`�悷��iCPU���׌y���j�B
//  100�̓��Ђ��S�ĕ������Z������FPS�ɉe�����邽�߁B
// ============================================================
void Game::UpdateGibs(float deltaTime)
{
    for (auto it = m_gibs.begin(); it != m_gibs.end();)
    {
        it->lifetime -= deltaTime;

        if (it->lifetime <= 0.0f)
        {
            // �����؂� �� �����I�u�W�F�N�g�����
            if (it->body)
            {
                m_dynamicsWorld->removeRigidBody(it->body);
                delete it->body->getMotionState();
                delete it->body;
                delete it->shape;
            }
            it = m_gibs.erase(it);
        }
        else
        {
            // ���n����F�����Ƒ��x��臒l�ȉ��Ȃ畨�����~
            if (!it->hasLanded && it->body)
            {
                btTransform t;
                it->body->getMotionState()->getWorldTransform(t);
                float y = t.getOrigin().getY();
                float speed = it->body->getLinearVelocity().length();

                if (y < SliceConstants::GIB_LAND_HEIGHT &&
                    speed < SliceConstants::GIB_LAND_SPEED)
                {
                    it->hasLanded = true;

                    // �ŏI�ʒu��ۑ�
                    it->finalPos = { t.getOrigin().getX(), t.getOrigin().getY(), t.getOrigin().getZ() };
                    btQuaternion rot = t.getRotation();
                    it->finalRot = { rot.x(), rot.y(), rot.z(), rot.w() };

                    // �������[���h���珜���iCPU���׌y���j
                    m_dynamicsWorld->removeRigidBody(it->body);
                    delete it->body->getMotionState();
                    delete it->body;
                    delete it->shape;
                    it->body = nullptr;
                    it->shape = nullptr;
                }
            }
            ++it;
        }
    }
}

// ============================================================
//  DrawGibs - ���ЃL���[�u�̕`��
//
//  �y�`������zGeometricPrimitive::CreateBox ���g����
//  �y�ʒu�擾�z��s��=Bullet����A���n�ς�=�ۑ������ʒu����
//  �y�t�F�[�h�z�c��1�b�ŃA���t�@��������
// ============================================================
void Game::DrawGibs(XMMATRIX view, XMMATRIX proj)
{
    if (m_gibs.empty()) return;

    for (const auto& gib : m_gibs)
    {
        XMMATRIX scale = XMMatrixScaling(gib.size, gib.size, gib.size);
        XMMATRIX rotation;
        XMMATRIX translation;

        if (gib.body != nullptr)
        {
            // �܂����ł� �� Bullet����ʒu�E��]���擾
            btTransform transform;
            gib.body->getMotionState()->getWorldTransform(transform);
            btVector3 pos = transform.getOrigin();
            btQuaternion rot = transform.getRotation();
            rotation = XMMatrixRotationQuaternion(
                XMVectorSet(rot.x(), rot.y(), rot.z(), rot.w()));
            translation = XMMatrixTranslation(pos.x(), pos.y(), pos.z());
        }
        else
        {
            // ���n�ς� �� �ۑ������ʒu���g�p
            rotation = XMMatrixRotationQuaternion(
                XMVectorSet(gib.finalRot.x, gib.finalRot.y, gib.finalRot.z, gib.finalRot.w));
            translation = XMMatrixTranslation(gib.finalPos.x, gib.finalPos.y, gib.finalPos.z);
        }

        XMMATRIX world = scale * rotation * translation;

        XMFLOAT4 color = gib.color;
        if (gib.lifetime < SliceConstants::GIB_FADE_THRESHOLD)
            color.w = gib.lifetime;

        m_cube->Draw(world, view, proj,
            XMVECTORF32{ color.x, color.y, color.z, color.w });
    }
}