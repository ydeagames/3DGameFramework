#include <algorithm>
#include "GridFloor.h"

// コンストラクタ
GridFloor::GridFloor(ID3D11Device* device, ID3D11DeviceContext* context, DirectX::CommonStates* states, float size, int divs) : m_size(size), m_divs(divs), m_states(states)
{
	// ベイシックエフェクトを生成する
	m_basicEffect = std::make_unique<DirectX::BasicEffect>(device);
	// 頂点カラーを有効にする
	m_basicEffect->SetVertexColorEnabled(true);
	// プリミティブバッチを生成する
	m_primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
	
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_basicEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
	// ポリゴン表示用のインプットレイアウトを生成する
	device->CreateInputLayout(DirectX::VertexPositionColor::InputElements,
		DirectX::VertexPositionColor::InputElementCount,
		shaderByteCode, byteCodeLength,
		m_pInputLayout.GetAddressOf());
}

// デストラクタ
GridFloor::~GridFloor()
{
	// ベイシックエフェクトを解放する
	m_basicEffect.reset();
	// 入力レイアウトを解放する
	m_pInputLayout.Reset();
}

// 描画する
void GridFloor::Render(ID3D11DeviceContext* context, DirectX::SimpleMath::Matrix view, DirectX::SimpleMath::Matrix projection, DirectX::GXMVECTOR color)
{
	DirectX::SimpleMath::Matrix world;

	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	// ワールド行列を設定する
	m_basicEffect->SetWorld(world);
	// ビュー行列を設定する
	m_basicEffect->SetView(view);
	// プロジェクション行列を設定する
	m_basicEffect->SetProjection(projection);
	// デバイスコンテキストを適用する
	m_basicEffect->Apply(context);

	context->IASetInputLayout(m_pInputLayout.Get());

	m_primitiveBatch->Begin();

	const DirectX::XMVECTORF32 xAxis = { m_size, 0.0f, 0.0f };
	const DirectX::XMVECTORF32 yAxis = { 0.0f, 0.0f, m_size };

	size_t divs = std::max<size_t>(1, m_divs);
	DirectX::FXMVECTOR origin {};
	for (size_t i = 0; i <= divs; ++i)
	{
		float fPercent = float(i) / float(divs);
		fPercent = (fPercent * 1.0f) - 0.5f;
		// スケールを設定する
		DirectX::XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
		// 原点にスケールを加算する
		vScale = DirectX::XMVectorAdd(vScale, origin);
		// 起点を設定する
		DirectX::VertexPositionColor v1(DirectX::XMVectorSubtract(vScale, yAxis * 0.5f), color);
		// 終点を設定する
		DirectX::VertexPositionColor v2(DirectX::XMVectorAdd(vScale, yAxis * 0.5f), color);
		// 直線を描画する
		m_primitiveBatch->DrawLine(v1, v2);
	}

	for (size_t i = 0; i <= divs; i++)
	{
		FLOAT fPercent = float(i) / float(divs);
		fPercent = (fPercent * 1.0f) - 0.5f;
		// スケールを設定する
		DirectX::XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
		// 原点にスケールを加算する
		vScale = DirectX::XMVectorAdd(vScale, origin);
		// 起点を設定する
		DirectX::VertexPositionColor v1(DirectX::XMVectorSubtract(vScale, xAxis * 0.5f), color);
		// 終点を設定する
		DirectX::VertexPositionColor v2(DirectX::XMVectorAdd(vScale, xAxis * 0.5f), color);
		// 直線を描画する
		m_primitiveBatch->DrawLine(v1, v2);
	}

	m_primitiveBatch->End();
}

