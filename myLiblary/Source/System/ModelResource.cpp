#include "../pch.h"
#include <stdlib.h>
#include <functional>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include "Misc.h"
#include "GpuResourceUtils.h"
#include "ModelResource.h"
#include "../tinygltf-release/tiny_gltf.h"



const std::vector<D3D11_INPUT_ELEMENT_DESC> ModelResource::InputElementDescs =
{
	{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// CEREALバージョン定義
CEREAL_CLASS_VERSION(ModelResource::Node, 1)
CEREAL_CLASS_VERSION(ModelResource::Material, 1)
CEREAL_CLASS_VERSION(ModelResource::Subset, 1)
CEREAL_CLASS_VERSION(ModelResource::Vertex, 1)
CEREAL_CLASS_VERSION(ModelResource::Mesh, 1)
CEREAL_CLASS_VERSION(ModelResource::NodeKeyData, 1)
CEREAL_CLASS_VERSION(ModelResource::Keyframe, 1)
CEREAL_CLASS_VERSION(ModelResource::Animation, 1)
CEREAL_CLASS_VERSION(ModelResource, 1)

// シリアライズ
namespace DirectX
{
	template<class Archive>
	void serialize(Archive& archive, XMUINT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT2& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT3& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}

	template<class Archive>
	void serialize(Archive& archive, XMFLOAT4X4& m)
	{
		archive(
			cereal::make_nvp("_11", m._11), cereal::make_nvp("_12", m._12), cereal::make_nvp("_13", m._13), cereal::make_nvp("_14", m._14),
			cereal::make_nvp("_21", m._21), cereal::make_nvp("_22", m._22), cereal::make_nvp("_23", m._23), cereal::make_nvp("_24", m._24),
			cereal::make_nvp("_31", m._31), cereal::make_nvp("_32", m._32), cereal::make_nvp("_33", m._33), cereal::make_nvp("_34", m._34),
			cereal::make_nvp("_41", m._41), cereal::make_nvp("_42", m._42), cereal::make_nvp("_43", m._43), cereal::make_nvp("_44", m._44)
		);
	}
}

template<class Archive>
void ModelResource::Node::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(id),
		CEREAL_NVP(name),
		CEREAL_NVP(path),
		CEREAL_NVP(parentIndex),
		CEREAL_NVP(scale),
		CEREAL_NVP(rotate),
		CEREAL_NVP(translate)
	);
}

template<class Archive>
void ModelResource::Material::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(name),
		CEREAL_NVP(textureFilename),
		CEREAL_NVP(color)
	);
}

template<class Archive>
void ModelResource::Subset::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(startIndex),
		CEREAL_NVP(indexCount),
		CEREAL_NVP(materialIndex)
	);
}

template<class Archive>
void ModelResource::Vertex::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(position),
		CEREAL_NVP(normal),
		CEREAL_NVP(tangent),
		CEREAL_NVP(texcoord),
		CEREAL_NVP(color),
		CEREAL_NVP(boneWeight),
		CEREAL_NVP(boneIndex)
	);
}

template<class Archive>
void ModelResource::Mesh::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(vertices),
		CEREAL_NVP(indices),
		CEREAL_NVP(subsets),
		CEREAL_NVP(nodeIndex),
		CEREAL_NVP(nodeIndices),
		CEREAL_NVP(offsetTransforms),
		CEREAL_NVP(boundsMin),
		CEREAL_NVP(boundsMax)
	);
}

template<class Archive>
void ModelResource::NodeKeyData::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(scale),
		CEREAL_NVP(rotate),
		CEREAL_NVP(translate)
	);
}

template<class Archive>
void ModelResource::Keyframe::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(seconds),
		CEREAL_NVP(nodeKeys)
	);
}

template<class Archive>
void ModelResource::Animation::serialize(Archive& archive, int version)
{
	archive(
		CEREAL_NVP(name),
		CEREAL_NVP(secondsLength),
		CEREAL_NVP(keyframes)
	);
}

// 読み込み
void ModelResource::Load(ID3D11Device* device, const char* filename)
{
	// ディレクトリパス取得
	char drive[32], dir[256], dirname[256];
	::_splitpath_s(filename, drive, sizeof(drive), dir, sizeof(dir), nullptr, 0, nullptr, 0);
	::_makepath_s(dirname, sizeof(dirname), drive, dir, nullptr, nullptr);

	// モデル構築
	BuildModel(device, dirname);

	// 拡張子取得
	std::string fname(filename);
	std::string ext;
	size_t dot = fname.find_last_of('.');
	if (dot != std::string::npos) {
		ext = fname.substr(dot + 1);
		for (auto& c : ext) c = static_cast<char>(tolower(c));
	}

	if (ext == "mdl") {
		Deserialize(filename);
		BuildModel(device, dirname);
	}
	else if (ext == "gltf" || ext == "glb") {
		LoadFromGLTF(device, filename, dirname);
	}
	else {
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Unsupported model format: %s", filename);
		_ASSERT_EXPR_A(false, buffer);
	}
}

// tinygltfからModelResourceへ詰め替え
void ModelResource::LoadFromGLTF(ID3D11Device* device, const char* filename, const char* dirname)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool ret = false;

	std::string fname(filename);
	std::string ext = fname.substr(fname.find_last_of('.') + 1);
	if (ext == "glb")
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename);
	else
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);

	if (!ret) {
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "glTF load failed: %s\n%s", filename, err.c_str());
		_ASSERT_EXPR_A(false, buffer);
		return;
	}

	// 1. マテリアル
	materials.clear();
	for (const auto& mat : gltfModel.materials) {
		Material m;
		m.name = mat.name;
		// テクスチャ名取得（ベースカラーのみ例示）
		if (mat.values.count("baseColorTexture")) {
			int texIndex = mat.values.at("baseColorTexture").TextureIndex();
			if (texIndex >= 0 && texIndex < gltfModel.textures.size()) {
				int imgIndex = gltfModel.textures[texIndex].source;
				if (imgIndex >= 0 && imgIndex < gltfModel.images.size()) {
					m.textureFilename = gltfModel.images[imgIndex].uri;
				}
			}
		}
		// 色（ベースカラーのみ例示）
		if (mat.values.count("baseColorFactor")) {
			const auto& col = mat.values.at("baseColorFactor").ColorFactor();
			m.color = DirectX::XMFLOAT4((float)col[0], (float)col[1], (float)col[2], (float)col[3]);
		}
		materials.push_back(m);
	}

	// 2. ノード
	nodes.clear();
	for (const auto& n : gltfModel.nodes) {
		Node node;
		node.name = n.name;
		node.parentIndex = -1; // 後で親子関係を構築
		// スケール
		if (!n.scale.empty())
			node.scale = DirectX::XMFLOAT3((float)n.scale[0], (float)n.scale[1], (float)n.scale[2]);
		else
			node.scale = DirectX::XMFLOAT3(1, 1, 1);
		// 回転
		if (!n.rotation.empty())
			node.rotate = DirectX::XMFLOAT4((float)n.rotation[0], (float)n.rotation[1], (float)n.rotation[2], (float)n.rotation[3]);
		else
			node.rotate = DirectX::XMFLOAT4(0, 0, 0, 1);
		// 平行移動
		if (!n.translation.empty())
			node.translate = DirectX::XMFLOAT3((float)n.translation[0], (float)n.translation[1], (float)n.translation[2]);
		else
			node.translate = DirectX::XMFLOAT3(0, 0, 0);
		nodes.push_back(node);
	}
	// 親子関係
	for (size_t i = 0; i < gltfModel.nodes.size(); ++i) {
		for (int child : gltfModel.nodes[i].children) {
			if (child >= 0 && child < (int)nodes.size()) {
				nodes[child].parentIndex = (int)i;
			}
		}
	}

	// 3. メッシュ
	meshes.clear();
	for (const auto& mesh : gltfModel.meshes) {
		for (const auto& prim : mesh.primitives) {
			Mesh m;
			// 頂点
			if (prim.attributes.count("POSITION")) {
				const auto& accessor = gltfModel.accessors[prim.attributes.at("POSITION")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];
				const float* pos = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				for (size_t i = 0; i < accessor.count; ++i) {
					Vertex v;
					v.position = DirectX::XMFLOAT3(pos[i * 3 + 0], pos[i * 3 + 1], pos[i * 3 + 2]);
					m.vertices.push_back(v);
				}
			}
			// 法線
			if (prim.attributes.count("NORMAL")) {
				const auto& accessor = gltfModel.accessors[prim.attributes.at("NORMAL")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];
				const float* nor = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				for (size_t i = 0; i < accessor.count && i < m.vertices.size(); ++i) {
					m.vertices[i].normal = DirectX::XMFLOAT3(nor[i * 3 + 0], nor[i * 3 + 1], nor[i * 3 + 2]);
				}
			}
			// UV
			if (prim.attributes.count("TEXCOORD_0")) {
				const auto& accessor = gltfModel.accessors[prim.attributes.at("TEXCOORD_0")];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];
				const float* uv = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				for (size_t i = 0; i < accessor.count && i < m.vertices.size(); ++i) {
					m.vertices[i].texcoord = DirectX::XMFLOAT2(uv[i * 2 + 0], uv[i * 2 + 1]);
				}
			}
			// インデックス
			if (prim.indices >= 0) {
				const auto& accessor = gltfModel.accessors[prim.indices];
				const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const auto& buffer = gltfModel.buffers[bufferView.buffer];
				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* idx = reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
					for (size_t i = 0; i < accessor.count; ++i) {
						m.indices.push_back(idx[i]);
					}
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					const uint32_t* idx = reinterpret_cast<const uint32_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
					for (size_t i = 0; i < accessor.count; ++i) {
						m.indices.push_back(idx[i]);
					}
				}
			}
			// サブセット
			Subset subset;
			subset.startIndex = 0;
			subset.indexCount = (uint32_t)m.indices.size();
			subset.materialIndex = prim.material;
			m.subsets.push_back(subset);

			// ノード割り当て（最初のノードに割り当てる例）
			m.nodeIndex = 0;
			meshes.push_back(m);
		}
	}

	// アニメーション（省略。必要に応じて追加）

	// GPUリソース生成
	BuildModel(device, dirname);
}

// モデル構築
void ModelResource::BuildModel(ID3D11Device* device, const char* dirname)
{
	for (Material& material : materials)
	{
		// 相対パスの解決
		char filename[256];
		::_makepath_s(filename, 256, nullptr, dirname, material.textureFilename.c_str(), nullptr);

		// マルチバイト文字からワイド文字へ変換
		wchar_t wfilename[256];
		::MultiByteToWideChar(CP_ACP, 0, filename, -1, wfilename, 256);

		// テクスチャ読み込み
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;
		HRESULT hr = GpuResourceUtils::LoadTexture(device, filename, material.shaderResourceView.GetAddressOf());
		if (FAILED(hr))
		{
			hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, material.shaderResourceView.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
	}

	for (Mesh& mesh : meshes)
	{
		// サブセット
		for (Subset& subset : mesh.subsets)
		{
			subset.material = &materials.at(subset.materialIndex);
		}

		// 頂点バッファ
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			D3D11_SUBRESOURCE_DATA subresourceData = {};

			bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.vertices.size());
			//bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			subresourceData.pSysMem = mesh.vertices.data();
			subresourceData.SysMemPitch = 0;
			subresourceData.SysMemSlicePitch = 0;

			HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.vertexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}

		// インデックスバッファ
		{
			D3D11_BUFFER_DESC bufferDesc = {};
			D3D11_SUBRESOURCE_DATA subresourceData = {};

			bufferDesc.ByteWidth = static_cast<UINT>(sizeof(u_int) * mesh.indices.size());
			//bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;
			subresourceData.pSysMem = mesh.indices.data();
			subresourceData.SysMemPitch = 0; //Not use for index buffers.
			subresourceData.SysMemSlicePitch = 0; //Not use for index buffers.
			HRESULT hr = device->CreateBuffer(&bufferDesc, &subresourceData, mesh.indexBuffer.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
	}
}

// シリアライズ
void ModelResource::Serialize(const char* filename)
{
	std::ofstream ostream(filename, std::ios::binary);
	if (ostream.is_open())
	{
		cereal::BinaryOutputArchive archive(ostream);

		try
		{
			archive(
				CEREAL_NVP(nodes),
				CEREAL_NVP(materials),
				CEREAL_NVP(meshes),
				CEREAL_NVP(animations)
			);
		}
		catch (...)
		{
			char buffer[256];
			sprintf_s(buffer, sizeof(buffer), "model serialize failed.\n%s\n", filename);
			_ASSERT_EXPR_A(false, buffer);
			return;
		}
	}
}

// デシリアライズ
void ModelResource::Deserialize(const char* filename)
{
	std::ifstream istream(filename, std::ios::binary);
	if (istream.is_open())
	{
		cereal::BinaryInputArchive archive(istream);

		try
		{
			archive(
				CEREAL_NVP(nodes),
				CEREAL_NVP(materials),
				CEREAL_NVP(meshes),
				CEREAL_NVP(animations)
			);
		}
		catch (...)
		{
			char buffer[256];
			sprintf_s(buffer, sizeof(buffer), "model deserialize failed.\n%s\n", filename);
			_ASSERT_EXPR_A(false, buffer);
			return;
		}
	}
	else
	{
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "File not found > %s", filename);
		_ASSERT_EXPR_A(false, buffer);
	}
}
