#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

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

	//FbxManager��FbxScene�I�u�W�F�N�g���쐬
	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ios);
	FbxScene* scene = FbxScene::Create(manager, "");

	//�f�[�^���C���|�[�g
	const char* filename = "star2.FBX";
	FbxImporter* importer = FbxImporter::Create(manager, "");
	importer->Initialize(filename, -1, manager->GetIOSettings());
	importer->Import(scene);
	importer->Destroy();

	//�O�p�|���S����
	FbxGeometryConverter geometryConverter(manager);
	geometryConverter.Triangulate(scene, true);

	m_primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(m_directX.GetContext().Get());

	m_fbxmodel = scene;

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
		float(m_width) / float(m_height), 0.1f, 100.0f);
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
				fog->SetFogStart(50.f);
				fog->SetFogEnd(60.f);
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

void DisplayPosition(FbxMesh* mesh)
{
	int positionNum = mesh->GetControlPointsCount();	// ���_��
	FbxVector4* position = mesh->GetControlPoints();	// ���_���W�z��

	for (int i = 0; i < positionNum; ++i)
	{
		std::cout << "position[" << i << "] : ("
			<< position[i][0] << ","
			<< position[i][1] << ","
			<< position[i][2] << ","
			<< position[i][3] << ")" << std::endl;
	}
}

void DisplayIndex(FbxMesh* mesh)
{
	//���|���S����
	int polygonNum = mesh->GetPolygonCount();

	//p�ڂ̃|���S���ւ̏���
	for (int p = 0; p < polygonNum; ++p)
	{
		//p�ڂ̃|���S����n�ڂ̒��_�ւ̏���
		for (int n = 0; n < 3; ++n)
		{
			int index = mesh->GetPolygonVertex(p, n);
			std::cout << "index[" << p + n << "] : " << index << std::endl;
		}
	}
}

void MyGame::DisplayMesh(FbxNode* node)
{
	FbxMesh* mesh = (FbxMesh*)node->GetNodeAttribute();

	//std::cout << "\n\nMesh Name: " << (char*)node->GetName() << std::endl;

	//DisplayIndex(mesh);
	//DisplayPosition(mesh);

	m_spriteBatch;
}

void MyGame::DisplayContent(FbxScene* scene, FbxNode* node)
{
	FbxNodeAttribute::EType lAttributeType;

	if (node->GetNodeAttribute() == NULL)
	{
		//std::cout << "NULL Node Attribute\n\n";
	}
	else
	{
		lAttributeType = (node->GetNodeAttribute()->GetAttributeType());

		switch (lAttributeType)
		{
		default:
			break;

		case FbxNodeAttribute::eMesh:

			FbxMesh* mesh = (FbxMesh*)node->GetNodeAttribute();

			//std::cout << "\n\nMesh Name: " << (char*)node->GetName() << std::endl;

			//DisplayIndex(mesh);
			//DisplayPosition(mesh);

			//m_spriteBatch->Draw(scene->GetTexture(,), mesh.getTex);
			int vertexCount = mesh->GetControlPointsCount();
			auto vertices = new VertexPositionColor[vertexCount];
			for (int i = 0; i < mesh->GetControlPointsCount(); i++) {
				auto point = mesh->GetControlPointAt(i);
				//auto color = mesh->coloGetElementVertexColor(i);
				vertices[i] = VertexPositionColor(Vector3((float)point[0], (float)point[1], (float)point[2]), Colors::White);
			}

			m_primitiveBatch->DrawIndexed(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, (uint16_t*) mesh->GetPolygonVertices(), mesh->GetPolygonCount(), vertices, vertexCount);
			delete vertices;

			break;
		}
	}


	for (int i = 0; i < node->GetChildCount(); i++)
	{
		DisplayContent(scene, node->GetChild(i));
	}
}

void MyGame::DisplayContent(FbxScene* scene)
{
	FbxNode* node = scene->GetRootNode();

	if (node)
	{
		for (int i = 0; i < node->GetChildCount(); i++)
		{
			DisplayContent(scene, node->GetChild(i));
		}
	}
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

	//for (auto& mesh : m_model->meshes)
	//{
	//	int i = 0;
	//	//mesh->Draw(m_directX.GetContext().Get(), m_world, m_view, m_projection);
	//	for (auto& part : mesh->meshParts)
	//	{
	//		if (true)
	//		{
	//			auto effect = std::dynamic_pointer_cast<DirectX::BasicEffect>(part->effect);
	//			effect->SetWorld(m_world);
	//			effect->SetView(m_view);
	//			effect->SetProjection(m_projection);
	//			part->Draw(m_directX.GetContext().Get(), part->effect.get(), part->inputLayout.Get());
	//		}
	//		i++;
	//	}
	//}

	// �X�v���C�g�o�b�`���I������
	GetSpriteBatch()->End();

	m_primitiveBatch->Begin();
	DisplayContent(m_fbxmodel);
	m_primitiveBatch->End();

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