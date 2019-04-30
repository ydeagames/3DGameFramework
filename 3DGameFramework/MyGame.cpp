#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"

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
	// モデルオブジェクトを生成する
	m_model = DirectX::Model::CreateFromCMO(m_directX.GetDevice().Get(), L"cup.cmo", *m_effectFactory);

	m_world = DirectX::SimpleMath::Matrix::Identity;

	//FbxManagerとFbxSceneオブジェクトを作成
	FbxManager* manager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
	manager->SetIOSettings(ios);
	FbxScene* scene = FbxScene::Create(manager, "");

	//データをインポート
	const char* filename = "star2.FBX";
	FbxImporter* importer = FbxImporter::Create(manager, "");
	importer->Initialize(filename, -1, manager->GetIOSettings());
	importer->Import(scene);
	importer->Destroy();

	//三角ポリゴン化
	FbxGeometryConverter geometryConverter(manager);
	geometryConverter.Triangulate(scene, true);

	m_primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(m_directX.GetContext().Get());

	m_fbxmodel = scene;

	// デバッグカメラを生成する
	m_debugCamera = std::make_unique<DebugCamera>(width, height);
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

void DisplayPosition(FbxMesh* mesh)
{
	int positionNum = mesh->GetControlPointsCount();	// 頂点数
	FbxVector4* position = mesh->GetControlPoints();	// 頂点座標配列

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
	//総ポリゴン数
	int polygonNum = mesh->GetPolygonCount();

	//p個目のポリゴンへの処理
	for (int p = 0; p < polygonNum; ++p)
	{
		//p個目のポリゴンのn個目の頂点への処理
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

	// スプライトバッチを終了する
	GetSpriteBatch()->End();

	m_primitiveBatch->Begin();
	DisplayContent(m_fbxmodel);
	m_primitiveBatch->End();

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
