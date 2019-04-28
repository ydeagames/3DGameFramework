#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"

// �R���X�g���N�^
MyGame::MyGame(int width, int height) : m_width(width), m_height(height), Game(width, height)
{
}

// MyGame�I�u�W�F�N�g����������
void MyGame::Initialize(int width, int height) 
{
	// ���N���X��Initialize���Ăяo�� 
	Game::Initialize(width, height);

	// CommonStates�I�u�W�F�N�g�𐶐�����
	m_commonStates = std::make_unique<DirectX::CommonStates>(m_directX.GetDevice().Get());
	// EffectFactory�I�u�W�F�N�g�𐶐�����
	m_effectFactory = std::make_unique<DirectX::EffectFactory>(m_directX.GetDevice().Get());
	// ���f���I�u�W�F�N�g�𐶐�����
	m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"cup.cmo", *m_effectFactory);

	m_world = DirectX::SimpleMath::Matrix::Identity;

	// �f�o�b�O�J�����𐶐�����
	m_debugCamera = std::make_unique<DebugCamera>(width, height);
}

// ���\�[�X�𐶐�����
void MyGame::CreateResources()
{
	// ���N���X��CreateResources���Ăяo��
	Game::CreateResources();

	// �r���[���W�ϊ��s��𐶐�����
	// ���_, �����_, 
	m_view = DirectX::SimpleMath::Matrix::CreateLookAt(DirectX::SimpleMath::Vector3(2.0f, 2.0f, 2.0f),
		DirectX::SimpleMath::Vector3::Zero, DirectX::SimpleMath::Vector3::Up);
	// �ˉe���W�ϊ��s��𐶐�����
	m_projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f,
		float(m_width) / float(m_height), 0.1f, 10.0f);
	// �G�t�F�N�g���X�V����
	m_model->UpdateEffects([](DirectX::IEffect* effect)
		{
			DirectX::IEffectLights* lights = dynamic_cast<DirectX::IEffectLights*>(effect);
			if (lights)
			{
				lights->SetLightingEnabled(true);
				lights->SetPerPixelLighting(true);
				lights->SetLightEnabled(0, true);
				lights->SetLightDiffuseColor(0, DirectX::Colors::AntiqueWhite);
				lights->SetLightEnabled(1, false);
				lights->SetLightEnabled(2, false);
			}
			
			DirectX::IEffectFog* fog = dynamic_cast<DirectX::IEffectFog*>(effect);
			if (fog)
			{
				fog->SetFogEnabled(true);
				fog->SetFogColor(DirectX::Colors::CornflowerBlue);
				fog->SetFogStart(8.f);
				fog->SetFogEnd(10.f);
			}
		});

	// �R�����X�e�[�g�̍쐬
	m_states = std::make_unique<DirectX::CommonStates>(m_directX.GetDevice().Get());

	// �O���b�h�̏��̍쐬
	m_gridFloor = std::make_unique<GridFloor>(m_directX.GetDevice().Get(), m_directX.GetContext().Get(), m_states.get(), 10.0f, 10);
}

// �Q�[�����X�V����
void MyGame::Update(const DX::StepTimer& timer) 
{
	// �o�ߎ��Ԃ��擾����
	float elapsedTime = float(timer.GetTotalSeconds());

	// �f�o�b�O�J�������X�V����
	m_debugCamera->Update();
}

// �Q�[����`�悷��
void MyGame::Render(const DX::StepTimer& timer) 
{
	// �ŏ��̍X�V�̑O�͉����`�悵�Ȃ��悤�ɂ���
	if (timer.GetFrameCount() == 0) 
		return;

	// TODO: �����_�����O�R�[�h��ǉ�����
	float time = float(timer.GetTotalSeconds());

	// Z���ɑ΂��ĉ�]������s��𐶐�����
	m_world = DirectX::SimpleMath::Matrix::CreateRotationZ(cosf(time) * 1.0f);

	// �r���[�s����쐬����
	m_view = m_debugCamera->GetCameraMatrix();

	// �o�b�t�@���N���A����
	Clear();

	// �O���b�h�̏���`�悷��
	m_gridFloor->Render(m_directX.GetContext().Get(), m_view, m_projection);

	// �X�v���C�g�o�b�`���J�n����
	GetSpriteBatch()->Begin(DirectX::SpriteSortMode_Deferred, m_commonStates->NonPremultiplied());
	// FPS��`�悷��
	DrawFPS(timer);
	// ���f����`�悷��
	m_model->Draw(m_directX.GetContext().Get(), *m_commonStates, m_world, m_view, m_projection);

	// �X�v���C�g�o�b�`���I������
	GetSpriteBatch()->End();
	// �o�b�N�o�b�t�@��\������
	Present();
}

// ��n��������
void MyGame::Finalize() 
{
	// ���N���X��Finalize���Ăяo��
	Game::Finalize();
}

// FPS��`�悷��
void MyGame::DrawFPS(const DX::StepTimer& timer)
{
	// FPS������𐶐�����
	wstring fpsString = L"fps = " + std::to_wstring((unsigned int)timer.GetFramesPerSecond());
	// FPS��`�悷��
	GetSpriteFont()->DrawString(GetSpriteBatch(), fpsString.c_str(), DirectX::SimpleMath::Vector2(0, 0), DirectX::Colors::White);
}
