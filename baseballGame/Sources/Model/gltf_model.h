#pragma once
#define NOMINMAX
#include <d3d11.h>
#include <wrl.h>
#include <DirectXMath.h>
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

class gltf_model 
{
	std::string filename;
public:
	gltf_model(ID3D11Device* device, const std::string& filename);
	virtual ~gltf_model() = default;

	

	struct scene
	{
		std::string name;
		std::vector<int> nodes;// 「ルート」ノードの配列
	};
	std::vector<scene> scenes;

	struct node 
	{
		std::string name;
		int skin{ -1 };// このノードが参照するスキンのインデックス
		int mesh{ -1 };// このノードが参照するメッシュのインデックス
		//-1はスキンやメッシュを参照していない

		std::vector<int> children;// このノードの子ノードのインデックスの配列
		int parent{ -1 }; // 親ノードのインデックスを追加
		//Local Transforms
		DirectX::XMFLOAT4 rotation{ 0,0,0,1 };//回転
		DirectX::XMFLOAT3 scale{ 1,1,1 };//スケール
		DirectX::XMFLOAT3 translation{ 0,0,0 };//位置

		DirectX::XMFLOAT4X4 global_transform{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
		DirectX::XMFLOAT4X4 local_transform{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	};
	std::vector<node> nodes;

	struct buffer_view
	{
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN; // バッファ内データのDXGIフォーマット
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer; // Direct3D 11 バッファへの参照
		size_t stride_in_bytes{ 0 }; // 1要素あたりのバイト数（ストライド）
		size_t size_in_bytes{ 0 };   // バッファ全体のバイト数
		size_t count() const
		{
			return size_in_bytes / stride_in_bytes; // バッファ内の要素数を返す
		}
	};

	struct mesh
	{
		std::string name; // メッシュ名
		struct primitive
		{
			int material; // 使用するマテリアルのインデックス
			std::map<std::string, buffer_view> vertex_buffer_views; // 頂点属性名とバッファの対応表
			buffer_view index_buffer_view; // インデックスバッファ

			//バッチング用のCPU側の頂点データ
			std::vector<DirectX::XMFLOAT3> cpu_positions;
			std::vector<DirectX::XMFLOAT3> cpu_normals;
			std::vector<DirectX::XMFLOAT4> cpu_tangents;
			std::vector<DirectX::XMFLOAT2> cpu_texcoords;
			std::vector<uint32_t>          cpu_indices;
		};
		std::vector<primitive> primitives; // メッシュを構成するプリミティブの配列
	};
	std::vector<mesh> meshes; // モデル内の全メッシュ

	void fetch_nodes(const tinygltf::Model& gltf_model);
	void cumulate_transforms(std::vector<node>& nodes);
	buffer_view make_buffer_view(const tinygltf::Accessor& accessor);
	void fetch_meshes(ID3D11Device* device, const tinygltf::Model& gltf_model);

	void render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world, const std::vector<node>& animated_nodes);
	// UNIT.35
	void fetch_materials(ID3D11Device* device, const tinygltf::Model& gltf_model);

	//UNIT.36
	void fetch_textures(ID3D11Device* device, const tinygltf::Model& gltf_model);

	//UNIT.37
	void fetch_animations(const tinygltf::Model& gltf_model);
	void animate(size_t animation_index, float time, std::vector<node>& animated_nodes);


	// ノードインデックス取得
	int GetNodeIndex(const char* name) const;

	const std::vector<mesh>& GetMeshes() const { return meshes; }


	// UNIT.34
public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;

	struct primitive_constants
	{
		DirectX::XMFLOAT4X4 world;
		int material{ -1 };
		int has_tangent{ 0 };
		int skin{ -1 };
		int pad;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;

	// UNIT.35
	//マテリアルの取得
	struct texture_info 
	{
		int index = -1;
		int texcoord = 0;
	};
	struct normal_texture_info 
	{
		int index = -1;
		int texcoord = 0;
		float scale = 1;
	};
	struct occlusion_texture_info 
	{
		int index = -1;
		int texcoord = 0;
		float strength = 1;
	};
	struct pbr_metallic_roughness 
	{
		float basecolor_factor[4] = { 1,1,1,1 };
		texture_info basecolor_texture;
		float metallic_factor = 1;
		float roughness_factor = 1;
		texture_info metallic_roughness_texture;
	};
	struct material 
	{
		std::string name;
		struct cbuffer 
		{
			float emissive_factor[3] = { 0,0,0 };
			int alpha_mode = 0; // "OPAQUE" : 0, "MASK" : 1, "BLEND" : 2
			float alpha_cutoff = 0.5f;
			bool double_sided = false;

			pbr_metallic_roughness pbr_metallic_roughness;

			normal_texture_info normal_texture;
			occlusion_texture_info occlusion_texture;
			texture_info emissive_texture;
		};
		cbuffer data;
	};
	std::vector<material> materials;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> material_resource_view;

	// UNIT.36
	struct texture
	{
	  std::string name;
	  int source{ -1 };
	};
	std::vector<texture> textures;
	struct image
	{
	  std::string name;
	   int width{ -1 };
	   int height{ -1 };
	   int component{ -1 };
	   int bits{ -1 };
	   int pixel_type{ -1 };
	   int buffer_view;
	   std::string mime_type;
	   std::string uri;
	   bool as_is{ false };
	};
	 std::vector<image> images;
	 std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> texture_resource_views;

	 //UNIT.37
	 struct skin
	 {
	   std::vector<DirectX::XMFLOAT4X4> inverse_bind_matrices;
	   std::vector<int> joints;
	 };
	 std::vector<skin> skins;
	 
	 struct animation
	 {
	    std::string name;
	    float duration{ 0.0f };

	    struct channel
		{
	     int sampler{ -1 };
	     int target_node{ -1 };
		 std::string target_path;// "translation", "rotation", "scale" など
		};
	    std::vector<channel> channels;
	
		struct sampler
		{
		     int input{ -1 };
		     int output{ -1 };
		     std::string interpolation;
		};
	    std::vector<sampler> samplers;
	    
	    std::unordered_map<int/*sampler.input*/, std::vector<float>> timelines;
	    std::unordered_map<int/*sampler.output*/, std::vector<DirectX::XMFLOAT3>> scales;
	    std::unordered_map<int/*sampler.output*/, std::vector<DirectX::XMFLOAT4>> rotations;
	    std::unordered_map<int/*sampler.output*/, std::vector<DirectX::XMFLOAT3>> translations;
	 };
	 std::vector<animation> animations;

	 //UNIT.37
	 //ボーン行列の構造体と定数バッファ
	 static const size_t PRIMITIVE_MAX_JOINTS = 512;
	 struct primitive_joint_constants 
	 {
		 DirectX::XMFLOAT4X4 matrices[PRIMITIVE_MAX_JOINTS];
	 };
	 Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_joint_cbuffer;

	 //STATIC_BATCHING
	 struct batched_primitive
	 {
		 int material{ -1 };
		 Microsoft::WRL::ComPtr<ID3D11Buffer> position_buffer;
		 Microsoft::WRL::ComPtr<ID3D11Buffer> normal_buffer;
		 Microsoft::WRL::ComPtr<ID3D11Buffer> tangent_buffer;
		 Microsoft::WRL::ComPtr<ID3D11Buffer> texcoord_buffer;
		 Microsoft::WRL::ComPtr<ID3D11Buffer> joint_buffer;//ダミー
		 Microsoft::WRL::ComPtr<ID3D11Buffer> weight_buffer;//ダミー
		 Microsoft::WRL::ComPtr<ID3D11Buffer> index_buffer;
		 UINT index_count{ 0 };
	 };

	 std::vector<batched_primitive> batched_primitives;

	 // ロード後に1回呼ぶ
	 void build_static_batches(ID3D11Device* device);

	 // バッチを使った描画（スキンなしモデル用）
	 void render_batched(ID3D11DeviceContext* immediate_context,
		 const DirectX::XMFLOAT4X4& world, const std::vector<node>& animated_nodes);

	
public:
	//フラスタムカリング用のバウンディングボックス
	struct BoundingBox
	{
		DirectX::XMFLOAT3 box_min;
		DirectX::XMFLOAT3 box_max;

		BoundingBox() : box_min(FLT_MAX, FLT_MAX, FLT_MAX), box_max(-FLT_MAX, -FLT_MAX, -FLT_MAX) {}

		void Merge(const DirectX::XMFLOAT3& point);// 点を含むようにバウンディングボックスを拡張
		void Merge(const BoundingBox& other);// 他のバウンディングボックスを含むように拡張
		DirectX::XMFLOAT3 GetCenter() const; // バウンディングボックスの中心を取得
		DirectX::XMFLOAT3 GetExtents() const; // バウンディングボックスの半分のサイズ（extents）を取得
		float GetRadius() const; // バウンディングボックスの半径を取得
	};

	//バウンディングスフィア
	struct BoundingSphere
	{
		DirectX::XMFLOAT3 center;
		float radius;
		BoundingSphere() : center(0.0f, 0.0f, 0.0f), radius(0.0f) {}
	};

	//モデル全体のバウンディングボックスとバウンディングスフィアを取得
	const BoundingBox& GetBoundingBox() const { return boundingBox; }
	const BoundingSphere& GetBoundingSphere() const { return boundingSphere; }

	//モデル全体のバウンディングボックスとバウンディングスフィアを計算
	void CalculateBounds();

private:
	BoundingBox boundingBox;
	BoundingSphere boundingSphere;

public:
	// インスタンスデータ用構造体
	struct instance_data
	{
		DirectX::XMFLOAT4X4 world;
	};

	// インスタンス描画用のバッファと容量
	Microsoft::WRL::ComPtr<ID3D11Buffer> instance_buffer;
	UINT instance_buffer_capacity = 0;

	// インスタンス専用の Vertex Shader と Input Layout (既存のものと2本立てにする)
	Microsoft::WRL::ComPtr<ID3D11VertexShader> instanced_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  instanced_input_layout;

	// 静的バッチング（batched_primitives）を使ったインスタンシング描画関数
	void render_batched_instanced(
		ID3D11DeviceContext* immediate_context,
		const std::vector<DirectX::XMFLOAT4X4>& worlds);
};