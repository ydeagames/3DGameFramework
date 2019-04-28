#ifndef DEBUG_CAMERA
#define DEBUG_CAMERA

class DebugCamera
{
private:
	// モーション
	void Motion(int x, int y);

public:
	// コンストラクタ
	DebugCamera(int width, int height);
	// デバッグカメラを更新する
	void Update();
	// デバッグカメラのビュー行列を取得する
	DirectX::SimpleMath::Matrix GetCameraMatrix() const;
	// デバッグカメラの位置を取得する
	DirectX::SimpleMath::Vector3 GetEyePosition() const;
	// 注視点の位置を返す
	DirectX::SimpleMath::Vector3 GetTargetPosition() const;
	// ウィンドウスケールを調整する
	void AdjustWindowScale(int width, int height);

private:
	// カメラの距離
	static const float CAMERA_DISTANCE;
	// 横回転
	float m_yAngle, m_yTmp;
	// 縦回転
	float m_xAngle, m_xTmp;
	// ドラッグされた座標
	int m_x, m_y;
	// スケール
	float m_xScale, m_yScale;

	// ビュー行列
	DirectX::SimpleMath::Matrix m_view;
	// スクロールホイール値
	int m_scrollWheelValue;
	// 視点
	DirectX::SimpleMath::Vector3 m_eye;
	// 注視点
	DirectX::SimpleMath::Vector3 m_target;
	// マウストラッカー
	DirectX::Mouse::ButtonStateTracker m_tracker;
};

#endif	// DEBUG_CAMERA

