#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"
#include "SoftModel.h"

using namespace Microsoft::WRL;
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
	auto model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"cup.cmo", *m_effectFactory);
	//m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron.cmo", *m_effectFactory);
	//m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron2.cmo", *m_effectFactory);
	//m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron3.cmo", *m_effectFactory);

	m_world = DirectX::SimpleMath::Matrix::Identity;

	// �f�o�b�O�J�����𐶐�����
	m_debugCamera = std::make_unique<DebugCamera>(width, height);

	auto smodel = SoftModelConverter::FromModel(m_directX.GetDevice().Get(), m_directX.GetContext().Get(), model);
	smodel->meshes[0]->meshParts[0]->vertices[0].position.y += 4;
	smodel->meshes[0]->meshParts[0]->vertices[3].position.y += 4;
	smodel->meshes[0]->meshParts[0]->vertices[6].position.y += 4;
	smodel->meshes[0]->meshParts[0]->vertices[9].position.y += 4;
	smodel->meshes[0]->meshParts[0]->vertices[12].position.y += 4;
	smodel->meshes[0]->meshParts[0]->vertices[15].position.y += 4;
	smodel->meshes[0]->meshParts[0]->vertices[18].position.y += 4;
	m_model = SoftModelConverter::ToModel(m_directX.GetDevice().Get(), smodel);

	/*
	{
		auto device = m_directX.GetDevice().Get();
		auto deviceContext = m_directX.GetContext().Get();
		auto& meshes = m_model->meshes;

		// Draw opaque parts
		for (auto it = meshes.cbegin(); it != meshes.cend(); ++it)
		{
			auto mesh = it->get();
			assert(mesh != nullptr);

			{
				auto& meshParts = mesh->meshParts;
				bool alpha = false;

				for (auto it = meshParts.cbegin(); it != meshParts.cend(); ++it)
				{
					auto part = (*it).get();
					assert(part != nullptr);

					{
						auto iinputLayout = part->inputLayout.Get();
						auto& vertexBuffer = part->vertexBuffer;
						auto& vertexStride = part->vertexStride;
						auto& indexBuffer = part->indexBuffer;
						auto& indexFormat = part->indexFormat;
						auto ieffect = part->effect.get();
						auto& primitiveType = part->primitiveType;
						auto& indexCount = part->indexCount;
						auto& startIndex = part->startIndex;
						auto& vertexOffset = part->vertexOffset;
						auto vb = vertexBuffer.Get();

						// --------------
						{
							D3D11_BUFFER_DESC ibDesc;
							indexBuffer->GetDesc(&ibDesc);
							//(2)[�AGPU���CPU�ǂݏ����\�o�b�t�@]���쐬���邽�߂̐ݒ�
							D3D11_BUFFER_DESC vbDesc;
							vb->GetDesc(&vbDesc);
							vbDesc.Usage = D3D11_USAGE_STAGING;
							vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
							vbDesc.BindFlags = 0;

							//(3)[�AGPU���CPU�ǂݏ����\�o�b�t�@]���쐬
							ID3D11Buffer* buf;
							ThrowIfFailed(device->CreateBuffer(&vbDesc, nullptr, &buf));

							//(4)[�@GPU��̏������ݐ�p�o�b�t�@]��[�AGPU���CPU�ǂݏ����\�o�b�t�@]�փR�s�[//
							deviceContext->CopyResource(buf, vb);

							//(5)[�AGPU���CPU�ǂݏ����\�o�b�t�@]�̃������A�h���X�̃}�b�v���J��//
							D3D11_MAPPED_SUBRESOURCE mappedResource;
							deviceContext->Map(buf, 0, D3D11_MAP_READ_WRITE, 0, &mappedResource);

							//(6)[�BCPU��̃o�b�t�@]���m��//
							std::vector<VertexPositionNormalTangentColorTexture> verts(vbDesc.ByteWidth / sizeof(VertexPositionNormalTangentColorTexture));

							//(7)[�AGPU���CPU�ǂݏ����\�o�b�t�@]����[�BCPU��̃o�b�t�@]�֓]��//
							CopyMemory(&verts[0], mappedResource.pData, vbDesc.ByteWidth);

							{
								//for (auto& vert : verts)
								//{
								//	vert.position.x *= 2.5f;
								//}
								verts[1].position.x *= 2.5f;
								//verts[6].position.x *= 2.5f;
								//verts[10].position.x *= 2.5f;
							}

							//(8)[�BCPU��̃o�b�t�@]����[�AGPU���CPU�ǂݏ����\�o�b�t�@]�֓]��//
							CopyMemory(mappedResource.pData, &verts[0], vbDesc.ByteWidth);

							//(9)[�AGPU���CPU�ǂݏ����\�o�b�t�@]�̃������A�h���X�̃}�b�v�����//
							deviceContext->Unmap(buf, 0);

							//(10)[�AGPU���CPU�ǂݏ����\�o�b�t�@]��[�@GPU��̏������ݐ�p�o�b�t�@]�փR�s�[//
							deviceContext->CopyResource(vb, buf);

							//(11)[�AGPU���CPU�ǂݏ����\�o�b�t�@]�����//
							buf->Release();
						}
					}
				}
			}
		}
	}
	*/
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

	/*
	{
		auto device = m_directX.GetDevice().Get();
		auto deviceContext = m_directX.GetContext().Get();
		auto& meshes = m_model->meshes;
		auto& states = *m_commonStates;
		bool wireframe = false;
		auto& world = m_world;
		auto& view = m_view;
		auto& projection = m_projection;
		std::function<void __cdecl()> setCustomState = nullptr;

		// Draw opaque parts
		for (auto it = meshes.cbegin(); it != meshes.cend(); ++it)
		{
			auto mesh = it->get();
			assert(mesh != nullptr);

			mesh->PrepareForRendering(deviceContext, states, false, wireframe);

			//mesh->Draw(deviceContext, world, view, projection, false, setCustomState);
			{
				auto& meshParts = mesh->meshParts;
				bool alpha = false;

				for (auto it = meshParts.cbegin(); it != meshParts.cend(); ++it)
				{
					auto part = (*it).get();
					assert(part != nullptr);

					if (part->isAlpha != alpha)
					{
						// Skip alpha parts when drawing opaque or skip opaque parts if drawing alpha
						continue;
					}

					auto imatrices = dynamic_cast<IEffectMatrices*>(part->effect.get());
					if (imatrices)
					{
						imatrices->SetMatrices(world, view, projection);
					}

					//part->Draw(deviceContext, part->effect.get(), part->inputLayout.Get(), setCustomState);
					{
						auto iinputLayout = part->inputLayout.Get();
						auto& vertexBuffer = part->vertexBuffer;
						auto& vertexStride = part->vertexStride;
						auto& indexBuffer = part->indexBuffer;
						auto& indexFormat = part->indexFormat;
						auto ieffect = part->effect.get();
						auto& primitiveType = part->primitiveType;
						auto& indexCount = part->indexCount;
						auto& startIndex = part->startIndex;
						auto& vertexOffset = part->vertexOffset;

						deviceContext->IASetInputLayout(iinputLayout);

						auto vb = vertexBuffer.Get();
						UINT vbStride = vertexStride;
						UINT vbOffset = 0;

						// �擾
						//D3D11_MAPPED_SUBRESOURCE msr;
						//deviceContext->Map(vb, 0, D3D11_MAP_READ_WRITE, 0, &msr);
						//deviceContext->Unmap(vb, 0);

						// ----------

						D3D11_BUFFER_DESC vbDesc;
						vb->GetDesc(&vbDesc);
						vbDesc.Usage = D3D11_USAGE_STAGING;
						vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
						vbDesc.BindFlags = 0;

						ID3D11Buffer* buf;
						ThrowIfFailed(device->CreateBuffer(&vbDesc, nullptr, &buf));

						deviceContext->CopyResource(buf, vb);

						//(5)GPU��̓ǂݍ��݉\�o�b�t�@�̃������A�h���X�̃}�b�v���J��//
						D3D11_MAPPED_SUBRESOURCE mappedResource;
						deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

						//(6)CPU��̃������Ƀo�b�t�@���m��//
						std::vector<VertexPositionNormalTangentColorTexture> verts(vbDesc.ByteWidth / sizeof(VertexPositionNormalTangentColorTexture));

						//(7)GPU��̓ǂݍ��݉\�o�b�t�@����CPU��̃o�b�t�@�֓]��//
						CopyMemory(&verts[0], mappedResource.pData, vbDesc.ByteWidth);
						deviceContext->Unmap(buf, 0);
						buf->Release();

						// -----------

						deviceContext->IASetVertexBuffers(0, 1, &vb, &vbStride, &vbOffset);

						// Note that if indexFormat is DXGI_FORMAT_R32_UINT, this model mesh part requires a Feature Level 9.2 or greater device
						deviceContext->IASetIndexBuffer(indexBuffer.Get(), indexFormat, 0);

						assert(ieffect != nullptr);
						ieffect->Apply(deviceContext);

						// Hook lets the caller replace our shaders or state settings with whatever else they see fit.
						if (setCustomState)
						{
							setCustomState();
						}

						// Draw the primitive.
						deviceContext->IASetPrimitiveTopology(primitiveType);

						deviceContext->DrawIndexed(indexCount, startIndex, vertexOffset);
					}
				}
			}
		}

		// Draw alpha parts
		for (auto it = meshes.cbegin(); it != meshes.cend(); ++it)
		{
			auto mesh = it->get();
			assert(mesh != nullptr);

			mesh->PrepareForRendering(deviceContext, states, true, wireframe);

			mesh->Draw(deviceContext, world, view, projection, true, setCustomState);
		}
	}
	*/

	//ComPtr<ID3D11Buffer> indexBuffer = m_model->meshes[0]->meshParts[0]->indexBuffer;
	//ComPtr<ID3D11Buffer> vertexBuffer = m_model->meshes[0]->meshParts[0]->vertexBuffer;

	//D3D11_BUFFER_DESC vdesc;
	//indexBuffer->GetDesc(&vdesc);

	//ZeroMemory(&vdesc, sizeof(vdesc));
	//vdesc.ByteWidth = vsize;
	//vdesc.Usage = D3D11_USAGE_DEFAULT;
	//vdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	//vdesc.CPUAccessFlags = 0;

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
