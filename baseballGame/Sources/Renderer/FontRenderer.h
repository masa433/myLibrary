#pragma once


// stb_truetype.h はヘッダオンリーライブラリ。
// 実装は FontRenderer.cpp 側で1回だけ STB_TRUETYPE_IMPLEMENTATION を定義してincludeする。
#include "imstb_truetype.h"

class FontRenderer
{
public:
	FontRenderer() = default;
	~FontRenderer() { Uninitialize(); }

	// UTF-8文字列からUnicodeコードポイント列を作るヘルパー。
	// ベイクしたい文字をまとめて指定する際に使う(重複していてもOK、Initialize側でユニーク化する)。
	static std::vector<int> Utf8ToCodepoints(const char* utf8Text);

	// fontPath  : .ttf / .otf へのパス(ttcフォントコレクションはfontIndexで面を指定)
	// pixelHeight: ベイクするフォントの基準サイズ(px)。大きいほどテクスチャが綺麗だが重い。
	// screenWidth/screenHeight: 描画先のスクリーンサイズ(ピクセル変換に使用)
	// codepoints: ベイクしたいUnicodeコードポイントの一覧。
	//             nullptrの場合はASCII 32(' ')?126('~')のみをベイクする(従来動作)。
	//             日本語を使う場合は Utf8ToCodepoints() で作った配列を渡すこと。
	// fontIndex : .ttc(フォントコレクション)内の面番号。通常の.ttf/.otfは0でよい。
	bool Initialize(
		ID3D11Device* device,
		const wchar_t* fontPath,
		float pixelHeight,
		int screenWidth,
		int screenHeight,
		int atlasWidth = 512,
		int atlasHeight = 512,
		const std::vector<int>* codepoints = nullptr,
		int fontIndex = 0);

	void Uninitialize();

	// スクリーンサイズが変わった場合に呼ぶ(リサイズ対応)
	void SetScreenSize(int screenWidth, int screenHeight) { screenWidth_ = screenWidth; screenHeight_ = screenHeight; }

	// text(UTF-8)を (x, y) を左上基準として描画する。
	// ベイクしていない文字(コードポイント)は無視され、半角スペース分だけ進む。
	void DrawText(
		ID3D11DeviceContext* dc,
		const char* utf8Text,
		float x, float y,
		float scale,
		float r, float g, float b, float a);

	// 文字列を描画した場合の幅・高さ(px)を計算する(改行非対応の1行想定)
	void MeasureText(const char* utf8Text, float scale, float& outWidth, float& outHeight) const;

	bool IsValid() const { return valid_; }

private:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texcoord;
	};

	bool CreateShaders(ID3D11Device* device);
	bool CreateAtlasTexture(ID3D11Device* device, const unsigned char* bitmap, int width, int height);
	bool EnsureVertexBuffer(ID3D11Device* device, size_t requiredVertexCount);

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>   vertexShader_;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>    pixelShader_;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>    inputLayout_;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>      atlasTexture_;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> atlasSRV_;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>   samplerState_;
	Microsoft::WRL::ComPtr<ID3D11BlendState>     blendState_;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
	Microsoft::WRL::ComPtr<ID3D11Buffer>         vertexBuffer_;
	size_t                                       vertexBufferCapacity_ = 0;

	// コードポイント(Unicode) -> ベイクされたグリフ情報
	std::unordered_map<int, stbtt_packedchar> glyphs_;

	int   atlasWidth_ = 2048;
	int   atlasHeight_ = 2048;
	float pixelHeight_ = 64.0f;

	int   screenWidth_ = 1280;
	int   screenHeight_ = 720;

	bool  valid_ = false;
};