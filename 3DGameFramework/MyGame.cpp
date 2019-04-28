#define _CRT_SECURE_NO_WARNINGS

#include "MyGame.h"

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
		float(m_width) / float(m_height), 0.1f, 10.0f);
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
				fog->SetFogStart(8.f);
				fog->SetFogEnd(10.f);
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
