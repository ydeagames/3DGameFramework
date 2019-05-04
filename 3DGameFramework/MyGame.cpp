#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// コンストラクタ
MyGame::MyGame(int width, int height) : m_width(width), m_height(height), Game(width, height)
{
}

// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
	com_exception(HRESULT hr) noexcept : result(hr) {}

	virtual const char* what() const override
	{
		static char s_str[64] = {};
		sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
		return s_str;
	}

private:
	HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw com_exception(hr);
	}
}

class SoftModelMeshPart
{
public:
	uint32_t														vertexStride;
	D3D_PRIMITIVE_TOPOLOGY											primitiveType;
	DXGI_FORMAT														indexFormat;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>						inputLayout;
	std::vector<VertexPositionNormalTangentColorTextureSkinning>	vertices;
	std::vector<uint16_t>											indices;
	std::shared_ptr<IEffect>										effect;
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>			vbDecl;
	bool															isAlpha;

	typedef std::vector<std::unique_ptr<SoftModelMeshPart>>	Collection;
};

class SoftModelMesh
{
public:
	BoundingSphere													boundingSphere;
	BoundingBox														boundingBox;
	SoftModelMeshPart::Collection									meshParts;
	std::wstring													name;
	bool															ccw;
	bool															pmalpha;

	typedef std::vector<std::shared_ptr<SoftModelMesh>>		Collection;
};

class SoftModel
{
public:
	SoftModelMesh::Collection										meshes;
	std::wstring													name;
};

class SoftModelConverter
{
public:
	static std::unique_ptr<SoftModel> FromModel(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::unique_ptr<Model>& model);
	static std::unique_ptr<Model> ToModel(ID3D11Device* device, std::unique_ptr<SoftModel>& model);
};

std::unique_ptr<SoftModel> SoftModelConverter::FromModel(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::unique_ptr<Model>& model)
{
	if (!device || !deviceContext || !model)
		throw std::exception("Device, deviceContext and model cannot be null");

	auto smodel = std::make_unique<SoftModel>();

	smodel->name = model->name;

	// Draw opaque parts
	for (auto it = model->meshes.cbegin(); it != model->meshes.cend(); ++it)
	{
		auto mesh = it->get();
		assert(mesh != nullptr);
		auto smesh = std::make_shared<SoftModelMesh>();

		smesh->boundingSphere = mesh->boundingSphere;
		smesh->boundingBox = mesh->boundingBox;
		smesh->name = mesh->name;
		smesh->ccw = mesh->ccw;
		smesh->pmalpha = mesh->pmalpha;

		for (auto it = mesh->meshParts.cbegin(); it != mesh->meshParts.cend(); ++it)
		{
			auto& part = *it;
			assert(part != nullptr);
			auto spart = std::make_unique<SoftModelMeshPart>();

			spart->vertexStride = part->vertexStride;
			spart->primitiveType = part->primitiveType;
			spart->indexFormat = part->indexFormat;
			spart->inputLayout = part->inputLayout;
			spart->effect = part->effect;
			spart->vbDecl = part->vbDecl;
			spart->isAlpha = part->isAlpha;

			{
				auto nIndexes = part->indexCount;

				//(2)[②GPU上のCPU読み書き可能バッファ]を作成するための設定
				D3D11_BUFFER_DESC desc;
				part->indexBuffer->GetDesc(&desc);
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.BindFlags = 0;

				//(3)[②GPU上のCPU読み書き可能バッファ]を作成
				ID3D11Buffer* buf;
				ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &buf));

				//(4)[①GPU上の書き込み専用バッファ]を[②GPU上のCPU読み書き可能バッファ]へコピー//
				deviceContext->CopyResource(buf, part->indexBuffer.Get());

				//(5)[②GPU上のCPU読み書き可能バッファ]のメモリアドレスのマップを開く//
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

				//(6)[③CPU上のバッファ]を確保//
				std::vector<uint16_t> inds(nIndexes);

				//(7)[②GPU上のCPU読み書き可能バッファ]から[③CPU上のバッファ]へ転送//
				auto* pDstInds = &inds[0];
				auto* pSrcInds = static_cast<uint16_t*>(mappedResource.pData);
				for (uint32_t i = 0; i < nIndexes; i++)
					pDstInds[i] = pSrcInds[i + part->startIndex] - part->vertexOffset;

				//(9)[②GPU上のCPU読み書き可能バッファ]のメモリアドレスのマップを閉じる//
				deviceContext->Unmap(buf, 0);

				//(11)[②GPU上のCPU読み書き可能バッファ]を解放//
				buf->Release();

				spart->indices = std::move(inds);
			}

			{
				//(2)[②GPU上のCPU読み書き可能バッファ]を作成するための設定
				D3D11_BUFFER_DESC desc;
				part->vertexBuffer->GetDesc(&desc);
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.BindFlags = 0;

				//頂点数
				auto nVerts = desc.ByteWidth / part->vertexStride;

				//(3)[②GPU上のCPU読み書き可能バッファ]を作成
				ID3D11Buffer* buf;
				ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &buf));

				//(4)[①GPU上の書き込み専用バッファ]を[②GPU上のCPU読み書き可能バッファ]へコピー//
				deviceContext->CopyResource(buf, part->vertexBuffer.Get());

				//(5)[②GPU上のCPU読み書き可能バッファ]のメモリアドレスのマップを開く//
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

				//(6)[③CPU上のバッファ]を確保//
				decltype(spart->vertices) verts(nVerts);

				//(7)[②GPU上のCPU読み書き可能バッファ]から[③CPU上のバッファ]へ転送//
				auto* pDstVerts = &verts[0];
				auto* pSrcVerts = static_cast<char*>(mappedResource.pData);
				for (size_t i = 0; i < nVerts; i++)
					CopyMemory(&pDstVerts[i], &pSrcVerts[part->vertexOffset + part->vertexStride * i], part->vertexStride);

				//(9)[②GPU上のCPU読み書き可能バッファ]のメモリアドレスのマップを閉じる//
				deviceContext->Unmap(buf, 0);

				//(11)[②GPU上のCPU読み書き可能バッファ]を解放//
				buf->Release();

				spart->vertices = std::move(verts);
			}

			smesh->meshParts.emplace_back(std::move(spart));
		}

		smodel->meshes.emplace_back(std::move(smesh));
	}

	return smodel;
}

std::unique_ptr<Model> SoftModelConverter::ToModel(ID3D11Device* device, std::unique_ptr<SoftModel>& smodel)
{
	if (!device || !smodel)
		throw std::exception("Device and softModel cannot be null");

	auto model = std::make_unique<Model>();

	model->name = smodel->name;

	// Draw opaque parts
	for (auto it = smodel->meshes.cbegin(); it != smodel->meshes.cend(); ++it)
	{
		auto smesh = it->get();
		assert(smesh != nullptr);
		auto mesh = std::make_shared<ModelMesh>();

		mesh->boundingSphere = smesh->boundingSphere;
		mesh->boundingBox = smesh->boundingBox;
		mesh->name = smesh->name;
		mesh->ccw = smesh->ccw;
		mesh->pmalpha = smesh->pmalpha;

		for (auto it = smesh->meshParts.cbegin(); it != smesh->meshParts.cend(); ++it)
		{
			auto& spart = *it;
			assert(spart != nullptr);
			auto part = std::make_unique<ModelMeshPart>();

			part->vertexStride = spart->vertexStride;
			part->primitiveType = spart->primitiveType;
			part->indexFormat = spart->indexFormat;
			part->inputLayout = spart->inputLayout;
			part->effect = spart->effect;
			part->vbDecl = spart->vbDecl;
			part->isAlpha = spart->isAlpha;

			{
				auto nIndexes = spart->indices.size();
				auto sizeInBytes = uint32_t(sizeof(uint16_t) * nIndexes);

				//(2)[②GPU上のCPU読み書き可能バッファ]を作成するための設定
				D3D11_BUFFER_DESC desc = {};
				desc.ByteWidth = sizeInBytes;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				//
				auto* pDstInds = &spart->indices[0];

				//
				D3D11_SUBRESOURCE_DATA initData = {};
				initData.pSysMem = pDstInds;

				//(3)[②GPU上のCPU読み書き可能バッファ]を作成
				ComPtr<ID3D11Buffer> buf;
				ThrowIfFailed(device->CreateBuffer(&desc, &initData, &buf));

				part->startIndex = 0;
				part->indexCount = static_cast<uint32_t>(nIndexes);
				part->indexBuffer = std::move(buf);
			}

			{
				auto nVerts = spart->vertices.size();
				auto sizeInBytes = uint32_t(spart->vertexStride * nVerts);

				//(2)[②GPU上のCPU読み書き可能バッファ]を作成するための設定
				D3D11_BUFFER_DESC desc = {};
				desc.ByteWidth = sizeInBytes;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				//
				std::vector<char> verts(sizeInBytes);

				//(7)[②GPU上のCPU読み書き可能バッファ]から[③CPU上のバッファ]へ転送//
				auto* pDstVerts = &verts[0];
				auto* pSrcVerts = &spart->vertices[0];
				for (size_t i = 0; i < nVerts; i++)
					CopyMemory(&pDstVerts[spart->vertexStride * i], &pSrcVerts[i], spart->vertexStride);

				//
				D3D11_SUBRESOURCE_DATA initData = {};
				initData.pSysMem = pDstVerts;

				//(3)[②GPU上のCPU読み書き可能バッファ]を作成
				ComPtr<ID3D11Buffer> buf;
				ThrowIfFailed(device->CreateBuffer(&desc, &initData, &buf));

				part->vertexOffset = 0;
				part->vertexBuffer = std::move(buf);
			}

			mesh->meshParts.emplace_back(std::move(part));
		}

		model->meshes.emplace_back(std::move(mesh));
	}

	return model;
}

// MyGameオブジェクトを初期する
void MyGame::Initialize(int width, int height)
{
	// 基底クラスのInitializeを呼び出す 
	Game::Initialize(width, height);

	// CommonStatesオブジェクトを生成する
	m_commonStates = std::make_unique<DirectX::CommonStates>(m_directX.GetDevice().Get());
	// EffectFactoryオブジェクトを生成する
	m_effectFactory = std::make_unique<DirectX::EffectFactory>(m_directX.GetDevice().Get());
	// モデルオブジェクトを生成する
	auto model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"cup.cmo", *m_effectFactory);
	//m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron.cmo", *m_effectFactory);
	//m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron2.cmo", *m_effectFactory);
	//m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron3.cmo", *m_effectFactory);

	m_world = DirectX::SimpleMath::Matrix::Identity;

	// デバッグカメラを生成する
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
							//(2)[②GPU上のCPU読み書き可能バッファ]を作成するための設定
							D3D11_BUFFER_DESC vbDesc;
							vb->GetDesc(&vbDesc);
							vbDesc.Usage = D3D11_USAGE_STAGING;
							vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
							vbDesc.BindFlags = 0;

							//(3)[②GPU上のCPU読み書き可能バッファ]を作成
							ID3D11Buffer* buf;
							ThrowIfFailed(device->CreateBuffer(&vbDesc, nullptr, &buf));

							//(4)[①GPU上の書き込み専用バッファ]を[②GPU上のCPU読み書き可能バッファ]へコピー//
							deviceContext->CopyResource(buf, vb);

							//(5)[②GPU上のCPU読み書き可能バッファ]のメモリアドレスのマップを開く//
							D3D11_MAPPED_SUBRESOURCE mappedResource;
							deviceContext->Map(buf, 0, D3D11_MAP_READ_WRITE, 0, &mappedResource);

							//(6)[③CPU上のバッファ]を確保//
							std::vector<VertexPositionNormalTangentColorTexture> verts(vbDesc.ByteWidth / sizeof(VertexPositionNormalTangentColorTexture));

							//(7)[②GPU上のCPU読み書き可能バッファ]から[③CPU上のバッファ]へ転送//
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

							//(8)[③CPU上のバッファ]から[②GPU上のCPU読み書き可能バッファ]へ転送//
							CopyMemory(mappedResource.pData, &verts[0], vbDesc.ByteWidth);

							//(9)[②GPU上のCPU読み書き可能バッファ]のメモリアドレスのマップを閉じる//
							deviceContext->Unmap(buf, 0);

							//(10)[②GPU上のCPU読み書き可能バッファ]を[①GPU上の書き込み専用バッファ]へコピー//
							deviceContext->CopyResource(vb, buf);

							//(11)[②GPU上のCPU読み書き可能バッファ]を解放//
							buf->Release();
						}
					}
				}
			}
		}
	}
	*/
}

// リソースを生成する
void MyGame::CreateResources()
{
	// 基底クラスのCreateResourcesを呼び出す
	Game::CreateResources();

	// ビュー座標変換行列を生成する
	// 視点, 注視点, 
	m_view = DirectX::SimpleMath::Matrix::CreateLookAt(DirectX::SimpleMath::Vector3(2.0f, 2.0f, 2.0f),
		DirectX::SimpleMath::Vector3::Zero, DirectX::SimpleMath::Vector3::Up);
	// 射影座標変換行列を生成する
	m_projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f,
		float(m_width) / float(m_height), 0.1f, 100.0f);
	// エフェクトを更新する
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

	// コモンステートの作成
	m_states = std::make_unique<DirectX::CommonStates>(m_directX.GetDevice().Get());

	// グリッドの床の作成
	m_gridFloor = std::make_unique<GridFloor>(m_directX.GetDevice().Get(), m_directX.GetContext().Get(), m_states.get(), 10.0f, 10);
}

// ゲームを更新する
void MyGame::Update(const DX::StepTimer& timer)
{
	// 経過時間を取得する
	float elapsedTime = float(timer.GetTotalSeconds());

	// デバッグカメラを更新する
	m_debugCamera->Update();
}

// ゲームを描画する
void MyGame::Render(const DX::StepTimer& timer)
{
	// 最初の更新の前は何も描画しないようにする
	if (timer.GetFrameCount() == 0)
		return;

	// TODO: レンダリングコードを追加する
	float time = float(timer.GetTotalSeconds());

	// Z軸に対して回転させる行列を生成する
	m_world = DirectX::SimpleMath::Matrix::CreateRotationZ(cosf(time) * 1.0f);

	// ビュー行列を作成する
	m_view = m_debugCamera->GetCameraMatrix();

	// バッファをクリアする
	Clear();

	// グリッドの床を描画する
	m_gridFloor->Render(m_directX.GetContext().Get(), m_view, m_projection);

	// スプライトバッチを開始する
	GetSpriteBatch()->Begin(DirectX::SpriteSortMode_Deferred, m_commonStates->NonPremultiplied());
	// FPSを描画する
	DrawFPS(timer);
	// モデルを描画する
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

						// 取得
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

						//(5)GPU上の読み込み可能バッファのメモリアドレスのマップを開く//
						D3D11_MAPPED_SUBRESOURCE mappedResource;
						deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

						//(6)CPU上のメモリにバッファを確保//
						std::vector<VertexPositionNormalTangentColorTexture> verts(vbDesc.ByteWidth / sizeof(VertexPositionNormalTangentColorTexture));

						//(7)GPU上の読み込み可能バッファからCPU上のバッファへ転送//
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

	// スプライトバッチを終了する
	GetSpriteBatch()->End();

	// バックバッファを表示する
	Present();
}

// 後始末をする
void MyGame::Finalize()
{
	// 基底クラスのFinalizeを呼び出す
	Game::Finalize();
}

// FPSを描画する
void MyGame::DrawFPS(const DX::StepTimer& timer)
{
	// FPS文字列を生成する
	wstring fpsString = L"fps = " + std::to_wstring((unsigned int)timer.GetFramesPerSecond());
	// FPSを描画する
	GetSpriteFont()->DrawString(GetSpriteBatch(), fpsString.c_str(), DirectX::SimpleMath::Vector2(0, 0), DirectX::Colors::White);
}
