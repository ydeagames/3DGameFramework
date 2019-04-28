// Game.h

#pragma once
#ifndef GAME_DEFINED
#define GAME_DEFINED

#include "StepTimer.h"
#include "Window.h"
#include "DirectX11.h"

class Window;

// �Q�[�����[�v��񋟂���Q�[���N���X
class Game
{
public:
	
	// �X�v���C�g�o�b�`���擾����
	DirectX::SpriteBatch* GetSpriteBatch() const
	{
		return m_spriteBatch.get();
	}
	// �X�v���C�g�t�H���g���擾����
	DirectX::SpriteFont* GetSpriteFont() const
	{
		return m_spriteFont.get();
	}

	// �Q�[�����[�v�����s����
	MSG Run();

	// �R���X�g���N�^
    Game(int width, int height);
    // �������������Ȃ�
    virtual void Initialize(int width, int height);
	// ���\�[�X�𐶐�����
	virtual void CreateResources();
	// �X�V����
	virtual void Update(const DX::StepTimer& timer);
	// ��ʂ��N���A����
	virtual void Clear();
	// �V�[����`�悷��
	virtual void Render(const DX::StepTimer& timer);
	// �o�b�N�o�b�t�@���X�N���[���ɑ���
    virtual void Present();
	// ��n��������
	virtual void Finalize();

    // ���b�Z�[�W
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // �v���p�e�B 
    void GetDefaultSize(int& width, int& height) const;

private:
	// �o�͕�
	int m_width;
	// �o�͍�
	int m_height;
	// �E�B���h�E�n���h��
	HWND m_hWnd;
	// �E�B���h�E�\���t���O
	int m_nCmdShow;
	// �C���X�^���X�n���h��
	HINSTANCE m_hInstance;

	// �^�C�}�[
    DX::StepTimer m_timer;
	// �@�\���x��
    D3D_FEATURE_LEVEL m_featureLevel;
	// �E�B���h�E
	std::unique_ptr<Window> m_window;
	
	// �X�v���C�g�t�H���g
	std::unique_ptr<DirectX::SpriteFont> m_spriteFont;
	// �X�v���C�g�o�b�`
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
	// DirectX11�N���X�̃C���X�^���X
	DirectX11& m_directX = DirectX11::Get();

	// �L�[�{�[�h
	std::unique_ptr<DirectX::Keyboard> m_keyboard;
	// �}�E�X
	std::unique_ptr<DirectX::Mouse> m_mouse;
};

#endif	// GAME_DEFINED

