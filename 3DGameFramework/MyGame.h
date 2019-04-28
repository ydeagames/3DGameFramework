#pragma once
#ifndef MYGAME_DEFINED
#define MYGAME_DEFINED

#include "Window.h"
#include "Game.h"
#include "DebugCamera.h"
#include "GridFloor.h"

class MyGame : public Game 
{
public:
	// コンストラクタ
	MyGame(int width, int height);
	// ゲームオブジェクトを初期する
	void Initialize(int width, int height) override;
	// リソースを生成する
	void CreateResources() override;
	// ゲームを更新する
	void Update(const DX::StepTimer& timer) override;
	// ゲームを描画する
	void Render(const DX::StepTimer& timer) override;
	// 終了処理をおこなう
	void Finalize() override;

	// FPSを描画する
	void DrawFPS(const DX::StepTimer& timer);

private:
	// 幅
	int m_width;
	// 高さ
	int m_height;
	// ワールド行列
	DirectX::SimpleMath::Matrix m_world;
	// ビュー行列
	DirectX::SimpleMath::Matrix m_view;
	// 射影行列
	DirectX::SimpleMath::Matrix m_projection;

	// スプライトバッチ
	DirectX::SpriteBatch* m_spriteBatch;
	// エフェクトファクトリインターフェース(m_fxFactory)
	std::unique_ptr<DirectX::IEffectFactory> m_effectFactory;
	// コモンステート
	std::unique_ptr <DirectX::CommonStates> m_commonStates;
	// モデル
	std::unique_ptr<DirectX::Model> m_model;

	// DirectX11クラスのインスタンスを取得する
	DirectX11& m_directX = DirectX11::Get();

	// デバッグカメラ
	std::unique_ptr<DebugCamera> m_debugCamera;
	// グリッドフロア
	std::unique_ptr<GridFloor> m_gridFloor;
	// コモンステート
	std::unique_ptr<DirectX::CommonStates> m_states;

};

#endif	// MYGAME_DEFINED