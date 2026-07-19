#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>

// ヘックス遷移エフェクト
//半透明の黒色の六角形が対角線状に広がるエフェクト

class HexTransitionEffect
{
public:
	void Initialize();

	//遷移アニメーションを開始する
	void Start(float durationSeconds = 1.0f);

	void Update(float elapsedTime);

	//描画
	void Render();

	bool IsPlaying() const { return playing; }
	bool IsFinished() const { return finished; }
	float GetProgress() const { return progress; } // 0.0~1.0

	//リセットする
	void Reset();

	//シーンのコピーを取得する
	void CaptureScene();

private:
	struct TransitionConstants
	{
		DirectX::XMFLOAT2 screen_size;
		float progress;
		float hex_size;
		DirectX::XMFLOAT2 direction;   // ワイプの方向（正規化ベクトル）
		float jitter;                  // 出現タイミングのランダム幅（0?1程度）
		float edge_softness;           // セル出現境界のぼかし量
		float wipe_min;
		float wipe_max;
		DirectX::XMFLOAT2 _pad2;
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer>       constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          sceneCopyTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneCopySRV;

	bool playing = false;
	bool finished = false;
	float progress = 0.0f;// 0.0~1.0
	float timer = 0.0f;// 経過時間
	float duration = 1.0f;// 遷移アニメーションの総時間
};