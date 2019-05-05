#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"
#include "SoftModel.h"
#include "CSG.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// コンストラクタ
MyGame::MyGame(int width, int height) : m_width(width), m_height(height), Game(width, height)
{
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

	m_world = DirectX::SimpleMath::Matrix::Identity;

	// デバッグカメラを生成する
	m_debugCamera = std::make_unique<DebugCamera>(width, height);



	// モデルオブジェクトを生成する
	std::cout << "Loading CMO" << std::endl;
	//auto model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"cup.cmo", *m_effectFactory);
	//auto model0 = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron.cmo", *m_effectFactory);
	//auto model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron2.cmo", *m_effectFactory);
	//auto model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron3.cmo", *m_effectFactory);
	auto model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"Tetrahedron4.cmo", *m_effectFactory);

	std::cout << "Converting to SoftModel" << std::endl;
	//m_model = std::move(model);
	auto smodel = SoftModelConverter::FromModel(m_directX.GetDevice().Get(), m_directX.GetContext().Get(), model);
	SoftModelConverter::RemoveUnreferencedVertices(smodel);
	SoftModelConverter::ConvertPolygonFaces(smodel, false);
	//SoftModelConverter::MergeMesh(smodel);
	//auto smodel0 = SoftModelConverter::FromModel(m_directX.GetDevice().Get(), m_directX.GetContext().Get(), model0);
	//SoftModelConverter::RemoveUnreferencedVertices(smodel0);
	//SoftModelConverter::ConvertPolygonFaces(smodel0, false);
	//SoftModelConverter::MergeMesh(smodel0);

	auto& m1 = smodel->meshes[0]->meshParts[0];
	auto& m2 = smodel->meshes[1]->meshParts[0];
	auto c1 = CSG::Difference(CSG::CSGModel{ m2->vertices, m2->indices }, CSG::CSGModel{ m1->vertices, m1->indices });
	m1->vertices = c1.vertices;
	m1->indices = c1.indices;
	smodel->meshes.pop_back();

	//auto& m1 = smodel->meshes[0]->meshParts[0];
	//auto& m2 = smodel0->meshes[0]->meshParts[0];
	//auto c1 = CSG::Intersection(CSG::CSGModel{ m1->vertices, m1->indices }, CSG::CSGModel{ m2->vertices, m2->indices });
	//m1->vertices = c1.vertices;
	//m1->indices = c1.indices;

	/*
	{
		auto& m1 = smodel0->meshes[0]->meshParts[0];
		auto& m2 = smodel0->meshes[0]->meshParts[1];
		auto& m3 = smodel0->meshes[0]->meshParts[2];
		auto& m = smodel->meshes[0]->meshParts[0];
		std::cout << "Generating CSG Mesh 1/3" << std::endl;
		auto c1 = CSG::Difference(CSG::CSGModel{ m1->vertices, m1->indices }, CSG::CSGModel{ m->vertices, m->indices });
		std::cout << "Generating CSG Mesh 2/3" << std::endl;
		auto c2 = CSG::Difference(CSG::CSGModel{ m2->vertices, m2->indices }, CSG::CSGModel{ m->vertices, m->indices });
		std::cout << "Generating CSG Mesh 3/3" << std::endl;
		auto c3 = CSG::Difference(CSG::CSGModel{ m3->vertices, m3->indices }, CSG::CSGModel{ m->vertices, m->indices });
		m1->vertices = c1.vertices;
		m1->indices = c1.indices;
		m2->vertices = c2.vertices;
		m2->indices = c2.indices;
		m3->vertices = c3.vertices;
		m3->indices = c3.indices;
	}
	/**/

	m_model = SoftModelConverter::ToModel(m_directX.GetDevice().Get(), smodel);
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
