#pragma once
#include <d3d11.h>
#include <directXmath.h>
#include <string>
#include <wrl.h>
#include <vector>

class sprite 
{
public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertex_buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;
	D3D11_TEXTURE2D_DESC texture2d_desc;

	//スプライトの状態を表す構造体
	struct SpriteState
	{
		DirectX::XMFLOAT2 position = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 size = { 100.0f, 100.0f };
		float             rotation = 0.0f;
		DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
	SpriteState state;

	//Undo・Redo用の関数
	void SaveState();
	void Undo();
	void Redo();
	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }

	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,//矩形の左上の座標
		float dw, float dh,//矩形のサイズ
		float r,float g,float b,float a,
		float angle/*degree*/);

	void render(ID3D11DeviceContext* immediate_context,
		float dx, float dy,//矩形の左上の座標
		float dw, float dh,//矩形のサイズ
		float r, float g, float b, float a,
		float angle/*degree*/,
		float sx,float sy,float sw, float sh);

	void render(ID3D11DeviceContext* immediate_context, float dx, float dy, float dw, float dh);

	//stateを使って描画するシンプルなrender
	void render(ID3D11DeviceContext* immediate_context);

	sprite(ID3D11Device* device, const wchar_t* filename);
	~sprite();

	void textout(ID3D11DeviceContext* immediate_context, std::string s,
		float x, float y, float w, float h, float r, float g, float b, float a);

	//頂点フォーマット
	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 texcoord;
	};

private:

	static const int MAX_UNDO = 50;//最大Undo回数
	std::vector<SpriteState> undoStack;//Undoの状態を保存するスタック
	std::vector<SpriteState> redoStack;//Redoの状態を保存するスタック
};