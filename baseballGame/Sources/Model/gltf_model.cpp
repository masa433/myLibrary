#include "gltf_model.h"
#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
#include "misc.h"
#include <stack>
#include "shader.h"
#include "texture.h"



// 画像データの読み込みを無効化するダミー関数
bool null_load_image_data(tinygltf::Image*, const int, std::string*, std::string*,
	int, int, const unsigned char*, int, void*) 
{
	return true;
}

gltf_model::gltf_model(ID3D11Device* device, const std::string& filename) : filename(filename)
{
	tinygltf::TinyGLTF tiny_gltf;
	tiny_gltf.SetImageLoader(null_load_image_data, nullptr);

	tinygltf::Model gltf_model;
	std::string error, warning;
	bool succeeded{ false };
	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadBinaryFromFile(&gltf_model, &error, &warning, filename.c_str());
	}
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
	}

	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, L"Failed to load glTF file");

	for (std::vector<tinygltf::Scene>::const_reference gltf_scene : gltf_model.scenes)
	{
		scene& scene{ scenes.emplace_back() };
		scene.name = gltf_scene.name;
		scene.nodes = gltf_scene.nodes;
	}
	

	fetch_nodes(gltf_model);
	fetch_meshes(device, gltf_model);

	// UNIT.35
	fetch_materials(device, gltf_model);
	// UNIT.36
	fetch_textures(device, gltf_model);
	//UNIT.37
	fetch_animations(gltf_model);

	// UNIT.34
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "JOINTS", 0, DXGI_FORMAT_R16G16B16A16_UINT, 4, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0,DXGI_FORMAT_R32G32B32A32_FLOAT, 5, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	create_vs_from_cso(device, "gltf_model_vs.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	create_ps_from_cso(device, "gltf_model_ps.cso", pixel_shader.ReleaseAndGetAddressOf());
	//UNIT.37
	//ボーン行列の定数バッファを生成する
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(primitive_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HRESULT hr;
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = sizeof(primitive_joint_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(&buffer_desc, NULL, primitive_joint_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

}
//find関数の戻り値が「検索した文字列が見つからなかった場合」だから
//!=を使って「filename の中に .glb という文字列が含まれていれば true、含まれていなければ false」になる

void gltf_model::fetch_nodes(const tinygltf::Model& gltf_model)
{
	for (std::vector<tinygltf::Node>::const_reference gltf_node : gltf_model.nodes)
	{
		node& node{ nodes.emplace_back() };
		node.name = gltf_node.name;
		node.skin = gltf_node.skin;
		node.mesh = gltf_node.mesh;
		node.children = gltf_node.children;

		if (!gltf_node.matrix.empty())
		{
			DirectX::XMFLOAT4X4 matrix;
			for (size_t row = 0; row < 4; row++)
			{
				for (size_t column = 0; column < 4; column++)
				{
					matrix(row, column) = static_cast<float>(gltf_node.matrix.at(4 * row + column));
				}
			}

			DirectX::XMVECTOR S, T, R;
			bool succeed = DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&matrix));
			_ASSERT_EXPR(succeed, L"Failed to decompose matrix.");

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.translation, T);
		}
		else
		{
			if (gltf_node.scale.size() > 0)
			{
				node.scale.x = static_cast<float>(gltf_node.scale.at(0));
				node.scale.y = static_cast<float>(gltf_node.scale.at(1));
				node.scale.z = static_cast<float>(gltf_node.scale.at(2));
			}
			if (gltf_node.translation.size() > 0)
			{
				node.translation.x = static_cast<float>(gltf_node.translation.at(0));
				node.translation.y = static_cast<float>(gltf_node.translation.at(1));
				node.translation.z = static_cast<float>(gltf_node.translation.at(2));
			}
			if (gltf_node.rotation.size() > 0)
			{
				node.rotation.x = static_cast<float>(gltf_node.rotation.at(0));
				node.rotation.y = static_cast<float>(gltf_node.rotation.at(1));
				node.rotation.z = static_cast<float>(gltf_node.rotation.at(2));
				node.rotation.w = static_cast<float>(gltf_node.rotation.at(3));
			}
		}
	}

	// 親子関係を設定
	for (size_t node_index = 0; node_index < nodes.size(); ++node_index)
	{
		for (int child_index : nodes[node_index].children)
		{
			if (child_index >= 0 && child_index < static_cast<int>(nodes.size()))
			{
				nodes[child_index].parent = static_cast<int>(node_index);
			}
		}
	}

	cumulate_transforms(nodes);
}

void gltf_model::cumulate_transforms(std::vector<node>&nodes)
{
  using namespace DirectX;

std::stack<XMFLOAT4X4> parent_global_transforms;
	std::function<void(int)> traverse{ [&] (int node_index)->void
	{
	  node & node{nodes.at(node_index)};
	  XMMATRIX S{ XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z) };
	  XMMATRIX R{ XMMatrixRotationQuaternion(
	    XMVectorSet(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w)) };
	  XMMATRIX T{ XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z) };
	  XMStoreFloat4x4(&node.global_transform, S * R * T * XMLoadFloat4x4(&parent_global_transforms.top()));
	  for (int child_index : node.children)
	  {
	    parent_global_transforms.push(node.global_transform);
	    traverse(child_index);

		  parent_global_transforms.pop();
	  }
	}

    };
  for (std::vector<int>::value_type node_index : scenes.at(0).nodes)
  {
    parent_global_transforms.push({ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 });
    traverse(node_index);
    parent_global_transforms.pop();
  }
}

gltf_model::buffer_view gltf_model::make_buffer_view(const tinygltf::Accessor& accessor)
{
	buffer_view buffer_view;
	switch (accessor.type) 
	{
	case TINYGLTF_TYPE_SCALAR:
		switch (accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			buffer_view.format = DXGI_FORMAT_R16_UINT;
			buffer_view.stride_in_bytes = sizeof(USHORT);
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			buffer_view.format = DXGI_FORMAT_R32_UINT;
			buffer_view.stride_in_bytes = sizeof(UINT);
			break;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			break;
		}
		break;
	case TINYGLTF_TYPE_VEC2:
		switch (accessor.componentType) 
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			buffer_view.format = DXGI_FORMAT_R32G32_FLOAT;
			buffer_view.stride_in_bytes = sizeof(FLOAT) * 2;
			break;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			break;
		}
		break;
	case TINYGLTF_TYPE_VEC3:
		switch (accessor.componentType) 
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			buffer_view.format = DXGI_FORMAT_R32G32B32_FLOAT;
			buffer_view.stride_in_bytes = sizeof(FLOAT) * 3;
			break;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			break;
		}
		break;
	case TINYGLTF_TYPE_VEC4:
		switch (accessor.componentType)
		{
			// LOOKAT
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			buffer_view.format = DXGI_FORMAT_R8G8B8A8_UINT;
			buffer_view.stride_in_bytes = sizeof(BYTE) * 4;
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			buffer_view.format = DXGI_FORMAT_R16G16B16A16_UINT;
			buffer_view.stride_in_bytes = sizeof(USHORT) * 4;
			break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			buffer_view.format = DXGI_FORMAT_R32G32B32A32_UINT;
			buffer_view.stride_in_bytes = sizeof(UINT) * 4;
			break;
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			buffer_view.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			buffer_view.stride_in_bytes = sizeof(FLOAT) * 4;
			break;
		default:
			_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
			break;
		}
		break;
	default:
		_ASSERT_EXPR(FALSE, L"This accessor component type is not supported.");
		break;
	}
	buffer_view.size_in_bytes = static_cast<UINT>(accessor.count * buffer_view.stride_in_bytes);
	return buffer_view;
}

void gltf_model::fetch_meshes(ID3D11Device* device, const tinygltf::Model& gltf_model) 
{
	HRESULT hr;
	for (std::vector<tinygltf::Mesh>::const_reference gltf_mesh : gltf_model.meshes)
	{
		mesh& mesh{ meshes.emplace_back() };
		mesh.name = gltf_mesh.name;
		for (std::vector<tinygltf::Primitive>::const_reference gltf_primitive : gltf_mesh.primitives) 
		{
			mesh::primitive& primitive{ mesh.primitives.emplace_back() };
			primitive.material = gltf_primitive.material;

			//Create index buffer(インデックスバッファ)
			const tinygltf::Accessor & gltf_accessor{ gltf_model.accessors.at(gltf_primitive.indices) };
			const tinygltf::BufferView & gltf_buffer_view{ gltf_model.bufferViews.at(gltf_accessor.bufferView) };

			primitive.index_buffer_view = make_buffer_view(gltf_accessor);

			D3D11_BUFFER_DESC buffer_desc{};
			buffer_desc.ByteWidth = static_cast<UINT>(primitive.index_buffer_view.size_in_bytes);
			buffer_desc.Usage = D3D11_USAGE_DEFAULT;
			buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			D3D11_SUBRESOURCE_DATA subresource_data{};
			subresource_data.pSysMem = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
				+ gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;

			hr = device->CreateBuffer(&buffer_desc, &subresource_data,
			      primitive.index_buffer_view.buffer.ReleaseAndGetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

			// インデックスのCPUコピー
			{
				const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
					+ gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
				primitive.cpu_indices.resize(gltf_accessor.count);
				if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					const uint16_t* src16 = reinterpret_cast<const uint16_t*>(src);
					for (size_t i = 0; i < gltf_accessor.count; ++i)
						primitive.cpu_indices[i] = src16[i];
				}
				else // UNSIGNED_INT
				{
					memcpy(primitive.cpu_indices.data(), src,
						gltf_accessor.count * sizeof(uint32_t));
				}
			}

			//Create vertex buffer(頂点バッファ)
			for (std::map<std::string, int>::const_reference gltf_attribute : gltf_primitive.attributes)
			{
				const tinygltf::Accessor & gltf_accessor{ gltf_model.accessors.at(gltf_attribute.second) };
				const tinygltf::BufferView & gltf_buffer_view{ gltf_model.bufferViews.at(gltf_accessor.bufferView) };
				
				buffer_view vertex_buffer_view{ make_buffer_view(gltf_accessor) };
				
				D3D11_BUFFER_DESC buffer_desc{};
				buffer_desc.ByteWidth = static_cast<UINT>(vertex_buffer_view.size_in_bytes);
				buffer_desc.Usage = D3D11_USAGE_DEFAULT;
				buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				D3D11_SUBRESOURCE_DATA subresource_data{};
				subresource_data.pSysMem = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
					+gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
				hr = device->CreateBuffer(&buffer_desc, &subresource_data,
				      vertex_buffer_view.buffer.ReleaseAndGetAddressOf());
				_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
				
				primitive.vertex_buffer_views.emplace(std::make_pair(gltf_attribute.first, vertex_buffer_view));

				// CPU上の頂点属性データもコピーする（スキニング計算のため）
				if (gltf_attribute.first == "POSITION")
				{
					const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
						+ gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					primitive.cpu_positions.resize(gltf_accessor.count);
					memcpy(primitive.cpu_positions.data(), src, primitive.cpu_positions.size() * sizeof(DirectX::XMFLOAT3));
				}
				else if (gltf_attribute.first == "NORMAL")
				{
					const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
						+ gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					primitive.cpu_normals.resize(gltf_accessor.count);
					memcpy(primitive.cpu_normals.data(), src,
						gltf_accessor.count * sizeof(DirectX::XMFLOAT3));
				}
				else if (gltf_attribute.first == "TANGENT")
				{
					const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
						+ gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					primitive.cpu_tangents.resize(gltf_accessor.count);
					memcpy(primitive.cpu_tangents.data(), src,
						gltf_accessor.count * sizeof(DirectX::XMFLOAT4));
				}
				else if (gltf_attribute.first == "TEXCOORD_0")
				{
					const uint8_t* src = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data()
						+ gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
					primitive.cpu_texcoords.resize(gltf_accessor.count);
					memcpy(primitive.cpu_texcoords.data(), src,
						gltf_accessor.count * sizeof(DirectX::XMFLOAT2));
				}
				
			}

			      // Add dummy attributes if any are missing. 
			const std::unordered_map<std::string, buffer_view> attributes
			{
				{ "TANGENT", { DXGI_FORMAT_R32G32B32A32_FLOAT } },
				{ "TEXCOORD_0", { DXGI_FORMAT_R32G32_FLOAT } },
				{ "JOINTS_0", { DXGI_FORMAT_R16G16B16A16_UINT } },
				{ "WEIGHTS_0", { DXGI_FORMAT_R32G32B32A32_FLOAT } },
			};
			for (std::unordered_map<std::string, buffer_view>::const_reference attribute : attributes)
			{
			    if (primitive.vertex_buffer_views.find(attribute.first) == primitive.vertex_buffer_views.end())
				{
			      primitive.vertex_buffer_views.insert(std::make_pair(attribute.first, attribute.second));
			    }
			}
		}
	}
}

//UNIT.34
void gltf_model::render(ID3D11DeviceContext* immediate_context, const DirectX::XMFLOAT4X4& world, const std::vector<node>& animated_nodes)
{
	using namespace DirectX;

	const std::vector<node>& nodes{ animated_nodes.size() > 0 ? animated_nodes : gltf_model::nodes };

	// UNIT.35
	//material_resource_viewオブジェクトをバインドする 
	immediate_context->PSSetShaderResources(0, 1, material_resource_view.GetAddressOf());

	// 頂点シェーダーとピクセルシェーダーをセット
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);
	// 入力レイアウトをセット
	immediate_context->IASetInputLayout(input_layout.Get());
	// プリミティブタイプ（三角形リスト）をセット
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ノードを再帰的に巡回するラムダ関数
	std::function<void(int)> traverse{ [&](int node_index)->void {
		const node& node{nodes.at(node_index)};

		if (node.skin > -1) 
		{
			const skin& skin{ skins.at(node.skin) };
			primitive_joint_constants primitive_joint_data{};
			for (size_t joint_index = 0; joint_index < skin.joints.size(); ++joint_index)
			{
				XMStoreFloat4x4(&primitive_joint_data.matrices[joint_index],
					XMLoadFloat4x4(&skin.inverse_bind_matrices.at(joint_index)) *
					XMLoadFloat4x4(&nodes.at(skin.joints.at(joint_index)).global_transform) *
					XMMatrixInverse(NULL, XMLoadFloat4x4(&node.global_transform))
				);
			}
			immediate_context->UpdateSubresource(primitive_joint_cbuffer.Get(), 0, 0, &primitive_joint_data, 0, 0);
			immediate_context->VSSetConstantBuffers(2, 1, primitive_joint_cbuffer.GetAddressOf());
		}

		// このノードがメッシュを持っていれば描画処理
		if (node.mesh > -1)
		{
			const mesh& mesh{ meshes.at(node.mesh) };
			// メッシュ内の全プリミティブを描画
			for (std::vector<mesh::primitive>::const_reference primitive : mesh.primitives)
			{
				

				// 頂点バッファをセット
				ID3D11Buffer* vertex_buffers[]
				{
				  primitive.vertex_buffer_views.at("POSITION").buffer.Get(),
				  primitive.vertex_buffer_views.at("NORMAL").buffer.Get(),
				  primitive.vertex_buffer_views.at("TANGENT").buffer.Get(),
				  primitive.vertex_buffer_views.at("TEXCOORD_0").buffer.Get(),
				  primitive.vertex_buffer_views.at("JOINTS_0").buffer.Get(),
				  primitive.vertex_buffer_views.at("WEIGHTS_0").buffer.Get(),
				};

				// 各頂点バッファのストライド（1頂点あたりのバイト数）
				UINT strides[]
				{
				  static_cast<UINT>(primitive.vertex_buffer_views.at("POSITION").stride_in_bytes),
				  static_cast<UINT>(primitive.vertex_buffer_views.at("NORMAL").stride_in_bytes),
				  static_cast<UINT>(primitive.vertex_buffer_views.at("TANGENT").stride_in_bytes),
				  static_cast<UINT>(primitive.vertex_buffer_views.at("TEXCOORD_0").stride_in_bytes),
				  static_cast<UINT>(primitive.vertex_buffer_views.at("JOINTS_0").stride_in_bytes),
				  static_cast<UINT>(primitive.vertex_buffer_views.at("WEIGHTS_0").stride_in_bytes),
				};

				// 各頂点バッファのオフセット（全て0）
				UINT offsets[_countof(vertex_buffers)]{ 0 };
				immediate_context->IASetVertexBuffers(0, _countof(vertex_buffers), vertex_buffers, strides, offsets);
				// インデックスバッファをセット
				immediate_context->IASetIndexBuffer(primitive.index_buffer_view.buffer.Get(),
				  primitive.index_buffer_view.format, 0);

				// プリミティブごとの定数バッファを更新
				primitive_constants primitive_data{};
				primitive_data.material = primitive.material; // マテリアルインデックス
				primitive_data.has_tangent = primitive.vertex_buffer_views.at("TANGENT").buffer != NULL; // タンジェント有無
				primitive_data.skin = node.skin; // スキンインデックス
				// ワールド行列（ノードのグローバル変換 × 外部から渡されたワールド行列）
				XMStoreFloat4x4(&primitive_data.world,
				  XMLoadFloat4x4(&node.global_transform) * XMLoadFloat4x4(&world));
				immediate_context->UpdateSubresource(primitive_cbuffer.Get(), 0, 0, &primitive_data, 0, 0);
				// 定数バッファをシェーダーにセット
				immediate_context->VSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());
				immediate_context->PSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());

				// UNIT.36
				const material& material{ materials.at(primitive.material) };
				const int texture_indices[]
				{
					 material.data.pbr_metallic_roughness.basecolor_texture.index,
					 material.data.pbr_metallic_roughness.metallic_roughness_texture.index,
					 material.data.normal_texture.index,
					 material.data.emissive_texture.index,
					 material.data.occlusion_texture.index,
				};
				ID3D11ShaderResourceView* null_shader_resource_view{};
				std::vector<ID3D11ShaderResourceView*> shader_resource_views(_countof(texture_indices));
				for (int texture_index = 0; texture_index < shader_resource_views.size(); ++texture_index)
				{
					shader_resource_views.at(texture_index) = texture_indices[texture_index] > -1 ?
						texture_resource_views.at(textures.at(texture_indices[texture_index]).source).Get() :
						null_shader_resource_view;
				}
				immediate_context->PSSetShaderResources(1, static_cast<UINT>(shader_resource_views.size()),
					shader_resource_views.data());

				// インデックス描画
				immediate_context->DrawIndexed(static_cast<UINT>(primitive.index_buffer_view.count()), 0, 0);
		    }
	    }
		// 子ノードを再帰的に描画
		for (std::vector<int>::value_type child_index : node.children)
		{
		  traverse(child_index);
		}
	}
	};

	// シーンの「ルート」ノードから描画を開始
	for (std::vector<int>::value_type node_index : scenes.at(0).nodes)
	{
		traverse(node_index);
	}
}

void gltf_model::fetch_materials(ID3D11Device* device, const tinygltf::Model& gltf_model)
{
	// GLTFファイルに含まれる全マテリアルを走査
	for (std::vector<tinygltf::Material>::const_reference gltf_material : gltf_model.materials)
	{
		// 内部のmaterial配列に新しい要素を追加し、参照を取得
		std::vector<material>::reference material = materials.emplace_back();

		// マテリアル名をコピー
		material.name = gltf_material.name;

		// 放射（エミッシブ）カラーの取得
		material.data.emissive_factor[0] = static_cast<float>(gltf_material.emissiveFactor.at(0));
		material.data.emissive_factor[1] = static_cast<float>(gltf_material.emissiveFactor.at(1));
		material.data.emissive_factor[2] = static_cast<float>(gltf_material.emissiveFactor.at(2));

		// アルファモードの設定（文字列を数値に変換）
		material.data.alpha_mode = gltf_material.alphaMode == "OPAQUE" ?
			0 : gltf_material.alphaMode == "MASK" ? 1 : gltf_material.alphaMode == "BLEND" ? 2 : 0;

		// アルファカットオフの設定
		material.data.alpha_cutoff = static_cast<float>(gltf_material.alphaCutoff);

		// 両面描画の設定（true:1, false:0）
		material.data.double_sided = gltf_material.doubleSided ? 1 : 0;

		// PBR（Metallic-Roughness）のベースカラー（RGBA）を取得
		material.data.pbr_metallic_roughness.basecolor_factor[0] =
			static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(0));
		material.data.pbr_metallic_roughness.basecolor_factor[1] =
			static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(1));
		material.data.pbr_metallic_roughness.basecolor_factor[2] =
			static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(2));
		material.data.pbr_metallic_roughness.basecolor_factor[3] =
			static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(3));

		// ベースカラーテクスチャのインデックスとUVセット番号を取得
		material.data.pbr_metallic_roughness.basecolor_texture.index =
			gltf_material.pbrMetallicRoughness.baseColorTexture.index;
		material.data.pbr_metallic_roughness.basecolor_texture.texcoord =
			gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord;

		// メタリックとラフネスの係数を取得
		material.data.pbr_metallic_roughness.metallic_factor =
			static_cast<float>(gltf_material.pbrMetallicRoughness.metallicFactor);
		material.data.pbr_metallic_roughness.roughness_factor =
			static_cast<float>(gltf_material.pbrMetallicRoughness.roughnessFactor);

		// メタリック・ラフネステクスチャの設定
		material.data.pbr_metallic_roughness.metallic_roughness_texture.index =
			gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
		material.data.pbr_metallic_roughness.metallic_roughness_texture.texcoord =
			gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

		// 法線マップの設定
		material.data.normal_texture.index = gltf_material.normalTexture.index;
		material.data.normal_texture.texcoord = gltf_material.normalTexture.texCoord;
		material.data.normal_texture.scale = static_cast<float>(gltf_material.normalTexture.scale);

		// オクルージョン（遮蔽）テクスチャの設定
		material.data.occlusion_texture.index = gltf_material.occlusionTexture.index;
		material.data.occlusion_texture.texcoord = gltf_material.occlusionTexture.texCoord;
		material.data.occlusion_texture.strength =
			static_cast<float>(gltf_material.occlusionTexture.strength);

		// エミッシブテクスチャの設定
		material.data.emissive_texture.index = gltf_material.emissiveTexture.index;
		material.data.emissive_texture.texcoord = gltf_material.emissiveTexture.texCoord;
	}

	// --------------------------------------------
	// マテリアルデータをGPUに転送する（SRVとして使えるようにする）
	// --------------------------------------------

	// 全マテリアルの定数バッファデータを一時的にまとめる
	std::vector<material::cbuffer> material_data;
	for (std::vector<material>::const_reference material : materials)
	{
		material_data.emplace_back(material.data);
	}

	HRESULT hr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> material_buffer;

	// 構造化バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(material::cbuffer) * material_data.size());
	buffer_desc.StructureByteStride = sizeof(material::cbuffer);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	// 初期データを指定
	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = material_data.data();

	// バッファ作成（失敗時はログ出力）
	hr = device->CreateBuffer(&buffer_desc, &subresource_data, material_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// シェーダーリソースビュー（SRV）の作成
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
	shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファはフォーマット不要
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	shader_resource_view_desc.Buffer.NumElements = static_cast<UINT>(material_data.size());

	// SRV作成（失敗時はログ出力）
	hr = device->CreateShaderResourceView(material_buffer.Get(),
		&shader_resource_view_desc, material_resource_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

// UNIT.36
void gltf_model::fetch_textures(ID3D11Device* device, const tinygltf::Model& gltf_model)
{
	HRESULT hr{ S_OK };
	for (const tinygltf::Texture& gltf_texture : gltf_model.textures)
	{
		texture& texture{ textures.emplace_back() };
		texture.name = gltf_texture.name;
		texture.source = gltf_texture.source;
	}
	for (const tinygltf::Image& gltf_image : gltf_model.images)
	{
		image& image{ images.emplace_back() };
		image.name = gltf_image.name;
		image.width = gltf_image.width;
		image.height = gltf_image.height;
		image.component = gltf_image.component;
		image.bits = gltf_image.bits;
		image.pixel_type = gltf_image.pixel_type;
		image.buffer_view = gltf_image.bufferView;
		image.mime_type = gltf_image.mimeType;
		image.uri = gltf_image.uri;
		image.as_is = gltf_image.as_is;

		if (gltf_image.bufferView > -1)
		{
			const tinygltf::BufferView& buffer_view{ gltf_model.bufferViews.at(gltf_image.bufferView) };
			const tinygltf::Buffer& buffer{ gltf_model.buffers.at(buffer_view.buffer) };
			const ::byte* data = buffer.data.data() + buffer_view.byteOffset;

			ID3D11ShaderResourceView* texture_resource_view{};
			hr = load_texture_from_memory(device, data, buffer_view.byteLength, &texture_resource_view);
			if (hr == S_OK)
			{
				texture_resource_views.emplace_back().Attach(texture_resource_view);
			}
		}
		else
		{
			const std::filesystem::path path(filename);
			ID3D11ShaderResourceView* shader_resource_view{};
			D3D11_TEXTURE2D_DESC texture2d_desc;
			std::wstring filename{ path.parent_path().concat(L"/").wstring() + std::wstring(gltf_image.uri.begin(), gltf_image.uri.end()) };
			hr = load_texture_from_file(device, filename.c_str(), &shader_resource_view, &texture2d_desc);
			if (hr == S_OK)
			{
				texture_resource_views.emplace_back().Attach(shader_resource_view);
			}
		}
	}
}

// UNIT.37
void gltf_model::fetch_animations(const tinygltf::Model& gltf_model)
{
	using namespace std;
	using namespace tinygltf;
	using namespace DirectX;

	for (vector<Skin>::const_reference transmission_skin : gltf_model.skins) 
	{
		skin& skin{ skins.emplace_back() };
		const Accessor& gltf_accessor{ gltf_model.accessors.at(transmission_skin.inverseBindMatrices) };
		const BufferView& gltf_buffer_view{ gltf_model.bufferViews.at(gltf_accessor.bufferView) };
		skin.inverse_bind_matrices.resize(gltf_accessor.count);
		memcpy(skin.inverse_bind_matrices.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() +
			gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(XMFLOAT4X4));
		skin.joints = transmission_skin.joints;
	}

	for (vector<Animation>::const_reference gltf_animation : gltf_model.animations) 
	{
		animation& animation{ animations.emplace_back() };
		animation.name = gltf_animation.name;
		for (vector<AnimationSampler>::const_reference gltf_sampler : gltf_animation.samplers) 
		{
			animation::sampler& sampler{ animation.samplers.emplace_back() };
			sampler.input = gltf_sampler.input;
			sampler.output = gltf_sampler.output;
			sampler.interpolation = gltf_sampler.interpolation;

			const Accessor& gltf_accessor{ gltf_model.accessors.at(gltf_sampler.input) };
			const BufferView& gltf_buffer_view{ gltf_model.bufferViews.at(gltf_accessor.bufferView) };
			pair<unordered_map<int, vector<float>>::iterator, bool>& timelines{
				animation.timelines.emplace(gltf_sampler.input,gltf_accessor.count) };
			if (timelines.second) 
			{
				memcpy(timelines.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() +
				      gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(FLOAT));
			}
		}

		for (vector<AnimationChannel>::const_reference gltf_channel : gltf_animation.channels) 
		{
			animation::channel& channel{ animation.channels.emplace_back() };
			channel.sampler = gltf_channel.sampler;
			channel.target_node = gltf_channel.target_node;
			channel.target_path = gltf_channel.target_path;

			const AnimationSampler& gltf_sampler{ gltf_animation.samplers.at(gltf_channel.sampler) };
			const Accessor& gltf_accessor{ gltf_model.accessors.at(gltf_sampler.output) };
			const BufferView& gltf_buffer_view{ gltf_model.bufferViews.at(gltf_accessor.bufferView) };
			if (gltf_channel.target_path == "scale") 
			{
				pair<unordered_map<int, vector<XMFLOAT3>>::iterator, bool>& scales{
				 animation.scales.emplace(gltf_sampler.output, gltf_accessor.count) };
				if (scales.second)
				{
					memcpy(scales.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() +
					      gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(XMFLOAT3));
				}
			}
			else if (gltf_channel.target_path == "rotation")
			{
				pair<unordered_map<int, vector<XMFLOAT4>>::iterator, bool>& rotations{
					 animation.rotations.emplace(gltf_sampler.output, gltf_accessor.count) };
				if (rotations.second) 
				{
					memcpy(rotations.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() +
					      gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(XMFLOAT4));
				}
			}
			else if (gltf_channel.target_path == "translation")
			{
				pair<unordered_map<int, vector<XMFLOAT3>>::iterator, bool>& translations{
					 animation.translations.emplace(gltf_sampler.output, gltf_accessor.count) };
				if (translations.second) 
				{
					memcpy(translations.first->second.data(), gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() +
						gltf_buffer_view.byteOffset + gltf_accessor.byteOffset, gltf_accessor.count * sizeof(XMFLOAT3));
				}
			}
		}
	}
	// Find a longest animation duration in timeline of each channel.
	// 各チャンネルのタイムライン内で最も長いアニメーション時間を取得する。
	for (decltype(animations)::reference animation : animations)
	{
		// Find a longest animation duration in timeline of each channel.
		for (decltype(animation.timelines)::reference timelines : animation.timelines)
		{
			animation.duration = std::max<float>(animation.duration, timelines.second.back());
		}
	}
}

//UNIT.37
void gltf_model::animate(size_t animation_index, float time, std::vector<node>& animated_nodes)
{
	using namespace std;
	using namespace DirectX;

	_ASSERT_EXPR(animations.size() > animation_index, L"");
	_ASSERT_EXPR(animated_nodes.size() == nodes.size(), L"");

	function<size_t(const vector<float>&, float, float&)> indexof{ [](const vector<float>& timelines, float time, float& interpolation_factor)->size_t {
		const size_t keyframe_count{ timelines.size() };
		if (time > timelines.at(keyframe_count - 1))
		{
			interpolation_factor = 1.0f;
			return keyframe_count - 2;
		}
		else if (time < timelines.at(0))
		{
			interpolation_factor = 0.0f;
			return 0;
		}
		size_t keyframe_index{ 0 };
		for (size_t time_index = 1; time_index < keyframe_count; ++time_index)
		{
			if (time < timelines.at(time_index))
			{
				keyframe_index = max<size_t>(0LL, time_index - 1);
				break;
			}
		}
		interpolation_factor = (time - timelines.at(keyframe_index + 0)) / (timelines.at(keyframe_index + 1) - timelines.at(keyframe_index + 0));
		return keyframe_index;
	} };

	if (animations.size() > 0)
	{
		const animation& animation{ animations.at(animation_index) };
		for (vector<animation::channel>::const_reference channel : animation.channels)
		{
			const animation::sampler& sampler{ animation.samplers.at(channel.sampler) };
			const vector<float>& timeline{ animation.timelines.at(sampler.input) };
			if (timeline.size() == 0)
			{
				continue;
			}
			float interpolation_factor{};
			size_t keyframe_index{ indexof(timeline, time, interpolation_factor) };
			if (channel.target_path == "scale")
			{
				const vector<XMFLOAT3>& scales{ animation.scales.at(sampler.output) };
				XMStoreFloat3(&animated_nodes.at(channel.target_node).scale, XMVectorLerp(XMLoadFloat3(&scales.at(keyframe_index + 0)), XMLoadFloat3(&scales.at(keyframe_index + 1)), interpolation_factor));
			}
			else if (channel.target_path == "rotation")
			{
				const vector<XMFLOAT4>& rotations{ animation.rotations.at(sampler.output) };
				XMStoreFloat4(&animated_nodes.at(channel.target_node).rotation, XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(&rotations.at(keyframe_index + 0)), XMLoadFloat4(&rotations.at(keyframe_index + 1)), interpolation_factor)));
			}
			else if (channel.target_path == "translation")
			{
				const vector<XMFLOAT3>& translations{ animation.translations.at(sampler.output) };
				XMStoreFloat3(&animated_nodes.at(channel.target_node).translation, XMVectorLerp(XMLoadFloat3(&translations.at(keyframe_index + 0)), XMLoadFloat3(&translations.at(keyframe_index + 1)), interpolation_factor));
			}
			else if (channel.target_path == "weights")
			{

			}
			else
			{

			}
		}
		cumulate_transforms(animated_nodes);
	}
}

// ノードインデックス取得
int gltf_model::GetNodeIndex(const char* name) const
{
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		if (nodes.at(nodeIndex).name == name)
		{
			return static_cast<int>(nodeIndex);
		}
	}
	return -1;
}

void gltf_model::build_static_batches(ID3D11Device* device)
{
	using namespace DirectX;

	//マテリアルIDごとに頂点データを集約するための構造体
	struct vertex_data
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT4 tangent;
		XMFLOAT2 texcoord;
	};

	//マテリアルIDごとに頂点データを集約するためのマップ
	std::unordered_map<int, std::vector<vertex_data>> batched_vertices;
	std::unordered_map<int, std::vector<uint32_t>> batched_indices;

	//スキンなしノードのプリミティブだけ収集
	std::function<void(int)> collect{ [&](int node_index)
	{
		const node& nd{ nodes.at(node_index) };

		//スキンありは対象外
		if (nd.skin == -1 && nd.mesh > -1)
		{
			const mesh& m{ meshes.at(nd.mesh) };
			XMMATRIX global = XMLoadFloat4x4(&nd.global_transform);

			for (const auto& prim : m.primitives)
			{
				// 頂点バッファからCPUデータを読み直す手段がないため、
				// CPU側データは fetch_meshes 時に保存しておく必要がある。
				auto& verts = batched_vertices[prim.material];
				auto& inds = batched_indices[prim.material];

				//既存超点数
				uint32_t base_vertex = static_cast<uint32_t>(verts.size());

				//POSITIONなどの頂点属性を読み取るためのバッファビュー
				const auto& pos_bv = prim.vertex_buffer_views.at("POSITION");
				const auto& norm_bv = prim.vertex_buffer_views.at("NORMAL");
				const auto& tan_bv = prim.vertex_buffer_views.at("TANGENT");
				const auto& tex_bv = prim.vertex_buffer_views.at("TEXCOORD_0");

				size_t vertex_count = pos_bv.count();
				for (size_t v = 0; v < vertex_count; ++v)
				{
					vertex_data vd{};

					//CPUバッファから読む
					if (prim.cpu_positions.size() > v)
					{
						// ノードのglobal_transformを適用してワールド空間に変換
						XMVECTOR p = XMVector3TransformCoord(
							XMLoadFloat3(&prim.cpu_positions[v]), global);
						XMStoreFloat3(&vd.position, p);

						XMVECTOR n = XMVector3TransformNormal(
							XMLoadFloat3(&prim.cpu_normals[v]), global);
						XMStoreFloat3(&vd.normal, XMVector3Normalize(n));

						if (prim.cpu_tangents.size() > v)
						{
							XMVECTOR t = XMVector3TransformNormal(
								XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(
									&prim.cpu_tangents[v])), global);
							vd.tangent = { XMVectorGetX(t), XMVectorGetY(t),
										   XMVectorGetZ(t), prim.cpu_tangents[v].w };
						}
						if (prim.cpu_texcoords.size() > v)
							vd.texcoord = prim.cpu_texcoords[v];
					}
					verts.push_back(vd);
				}
				//インデックスをコピー
				for (uint32_t idx : prim.cpu_indices)
				{
					inds.push_back(idx + base_vertex);
				}
			}
		}
		for(int child : nd.children)
		{
			collect(child);
		}
	}};

	//シーンのルートノードから収集開始
	for (int root : scenes.at(0).nodes)
	{
		collect(root);
	}

	//D3D11バッファを作成
	for (auto& [mat_id, verts] : batched_vertices)
	{
		auto& bp = batched_primitives.emplace_back();
		bp.material = mat_id;

		auto& indices = batched_indices.at(mat_id);
		bp.index_count = static_cast<UINT>(indices.size());

		//各属性を分離して格納
		std::vector<XMFLOAT3> positions, normals;
		std::vector<XMFLOAT4> tangents;
		std::vector<XMFLOAT2> texcoords;
		for (auto& v : verts)
		{
			positions.push_back(v.position);
			normals.push_back(v.normal);
			tangents.push_back(v.tangent);
			texcoords.push_back(v.texcoord);
			//JOINTS_0やWEIGHTS_0はスキンなしなのでダミー値を入れる
		}

		//ダミーバッファ
		std::vector<uint16_t> dummy_joints(verts.size() * 4, 0);
		std::vector<float> dummy_weights(verts.size() * 4, 0.0f);

		auto create_vb = [&](ID3D11Device* dev, const void* data, size_t bytes,
			ID3D11Buffer** out)
			{
				D3D11_BUFFER_DESC bd{};
				bd.ByteWidth = static_cast<UINT>(bytes);
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				D3D11_SUBRESOURCE_DATA sd{ data };
				HRESULT hr = dev->CreateBuffer(&bd, &sd, out);
				_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
			};

		create_vb(device, positions.data(), positions.size() * sizeof(XMFLOAT3), bp.position_buffer.GetAddressOf());
		create_vb(device, normals.data(), normals.size() * sizeof(XMFLOAT3), bp.normal_buffer.GetAddressOf());
		create_vb(device, tangents.data(), tangents.size() * sizeof(XMFLOAT4), bp.tangent_buffer.GetAddressOf());
		create_vb(device, texcoords.data(), texcoords.size() * sizeof(XMFLOAT2), bp.texcoord_buffer.GetAddressOf());
		create_vb(device, dummy_joints.data(), dummy_joints.size() * sizeof(uint16_t), bp.joint_buffer.GetAddressOf());
		create_vb(device, dummy_weights.data(), dummy_weights.size() * sizeof(float), bp.weight_buffer.GetAddressOf());


		// インデックスバッファ
		D3D11_BUFFER_DESC ibd{};
		ibd.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32_t));
		ibd.Usage = D3D11_USAGE_DEFAULT;
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		D3D11_SUBRESOURCE_DATA isd{ indices.data() };
		HRESULT hr = device->CreateBuffer(&ibd, &isd, bp.index_buffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	}
}

void gltf_model::render_batched(ID3D11DeviceContext* immediate_context,
    const DirectX::XMFLOAT4X4& world, const std::vector<node>& animated_nodes)
{
    using namespace DirectX;

    const std::vector<node>& nodes{
        animated_nodes.size() > 0 ? animated_nodes : gltf_model::nodes };

    immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
    immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);
    immediate_context->IASetInputLayout(input_layout.Get());
    immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    immediate_context->PSSetShaderResources(0, 1, material_resource_view.GetAddressOf());

    // -------------------------------------------------------
    // ① スキンありノード → 通常描画（アニメーション対応）
    // -------------------------------------------------------
    std::function<void(int)> traverse_skinned{ [&](int node_index)->void
    {
        const node& nd{ nodes.at(node_index) };

        if (nd.skin > -1)
        {
            // ボーン行列を更新
            const skin& sk{ skins.at(nd.skin) };
            primitive_joint_constants joint_data{};
            for (size_t ji = 0; ji < sk.joints.size(); ++ji)
            {
                XMStoreFloat4x4(&joint_data.matrices[ji],
                    XMLoadFloat4x4(&sk.inverse_bind_matrices.at(ji)) *
                    XMLoadFloat4x4(&nodes.at(sk.joints.at(ji)).global_transform) *
                    XMMatrixInverse(nullptr, XMLoadFloat4x4(&nd.global_transform)));
            }
            immediate_context->UpdateSubresource(
                primitive_joint_cbuffer.Get(), 0, 0, &joint_data, 0, 0);
            immediate_context->VSSetConstantBuffers(2, 1, primitive_joint_cbuffer.GetAddressOf());

            if (nd.mesh > -1)
            {
                const mesh& m{ meshes.at(nd.mesh) };
                for (const auto& prim : m.primitives)
                {
                    // テクスチャバインド
                    const material& mat{ materials.at(prim.material) };
                    const int tex_indices[]
                    {
                        mat.data.pbr_metallic_roughness.basecolor_texture.index,
                        mat.data.pbr_metallic_roughness.metallic_roughness_texture.index,
                        mat.data.normal_texture.index,
                        mat.data.emissive_texture.index,
                        mat.data.occlusion_texture.index,
                    };
                    ID3D11ShaderResourceView* null_srv{};
                    std::vector<ID3D11ShaderResourceView*> srvs(_countof(tex_indices));
                    for (int i = 0; i < (int)srvs.size(); ++i)
                        srvs[i] = tex_indices[i] > -1
                            ? texture_resource_views.at(
                                textures.at(tex_indices[i]).source).Get()
                            : null_srv;
                    immediate_context->PSSetShaderResources(1, (UINT)srvs.size(), srvs.data());

                    // 定数バッファ
                    primitive_constants prim_data{};
                    prim_data.material    = prim.material;
                    prim_data.has_tangent = prim.vertex_buffer_views.at("TANGENT").buffer != nullptr;
                    prim_data.skin        = nd.skin;
                    XMStoreFloat4x4(&prim_data.world,
                        XMLoadFloat4x4(&nd.global_transform) * XMLoadFloat4x4(&world));
                    immediate_context->UpdateSubresource(
                        primitive_cbuffer.Get(), 0, 0, &prim_data, 0, 0);
                    immediate_context->VSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());
                    immediate_context->PSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());

                    // 頂点バッファ
                    ID3D11Buffer* vbs[]
                    {
                        prim.vertex_buffer_views.at("POSITION").buffer.Get(),
                        prim.vertex_buffer_views.at("NORMAL").buffer.Get(),
                        prim.vertex_buffer_views.at("TANGENT").buffer.Get(),
                        prim.vertex_buffer_views.at("TEXCOORD_0").buffer.Get(),
                        prim.vertex_buffer_views.at("JOINTS_0").buffer.Get(),
                        prim.vertex_buffer_views.at("WEIGHTS_0").buffer.Get(),
                    };
                    UINT strides[]
                    {
                        (UINT)prim.vertex_buffer_views.at("POSITION").stride_in_bytes,
                        (UINT)prim.vertex_buffer_views.at("NORMAL").stride_in_bytes,
                        (UINT)prim.vertex_buffer_views.at("TANGENT").stride_in_bytes,
                        (UINT)prim.vertex_buffer_views.at("TEXCOORD_0").stride_in_bytes,
                        (UINT)prim.vertex_buffer_views.at("JOINTS_0").stride_in_bytes,
                        (UINT)prim.vertex_buffer_views.at("WEIGHTS_0").stride_in_bytes,
                    };
                    UINT offsets[_countof(vbs)]{};
                    immediate_context->IASetVertexBuffers(0, _countof(vbs), vbs, strides, offsets);
                    immediate_context->IASetIndexBuffer(
                        prim.index_buffer_view.buffer.Get(),
                        prim.index_buffer_view.format, 0);
                    immediate_context->DrawIndexed(
                        (UINT)prim.index_buffer_view.count(), 0, 0);
                }
            }
        }

        for (int child : nd.children)
            traverse_skinned(child);
    }};

    // スキンありノードを先に描画
    bool has_skinned = false;
    for (const auto& nd : nodes)
        if (nd.skin > -1) { has_skinned = true; break; }

    if (has_skinned)
    {
        for (int root : scenes.at(0).nodes)
            traverse_skinned(root);
    }

    // -------------------------------------------------------
    // ② スキンなしノード → バッチから描画
    // -------------------------------------------------------
	//バッチ未構築なら通常描画にフォールバック
	if (batched_primitives.empty())
	{
		//スキンなしノードだけ通常描画
		std::function<void(int)> traverse_static{ [&](int node_index)->void
		{
			const node& nd{ nodes.at(node_index) };

			if (nd.skin == -1 && nd.mesh > -1)
			{
				const mesh& m{ meshes.at(nd.mesh) };
				for (const auto& prim : m.primitives)
				{
					const material& mat{ materials.at(prim.material) };
					const int tex_indices[]
					{
						mat.data.pbr_metallic_roughness.basecolor_texture.index,
						mat.data.pbr_metallic_roughness.metallic_roughness_texture.index,
						mat.data.normal_texture.index,
						mat.data.emissive_texture.index,
						mat.data.occlusion_texture.index,
					};
					ID3D11ShaderResourceView* null_srv{};
					std::vector<ID3D11ShaderResourceView*> srvs(_countof(tex_indices));
					for (int i = 0; i < (int)srvs.size(); ++i)
						srvs[i] = tex_indices[i] > -1
							? texture_resource_views.at(
								textures.at(tex_indices[i]).source).Get()
							: null_srv;
					immediate_context->PSSetShaderResources(1, (UINT)srvs.size(), srvs.data());

					primitive_constants prim_data{};
					prim_data.material = prim.material;
					prim_data.has_tangent = prim.vertex_buffer_views.at("TANGENT").buffer != nullptr;
					prim_data.skin = -1;
					XMStoreFloat4x4(&prim_data.world,
						XMLoadFloat4x4(&nd.global_transform) * XMLoadFloat4x4(&world));
					immediate_context->UpdateSubresource(
						primitive_cbuffer.Get(), 0, 0, &prim_data, 0, 0);
					immediate_context->VSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());
					immediate_context->PSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());

					ID3D11Buffer* vbs[]
					{
						prim.vertex_buffer_views.at("POSITION").buffer.Get(),
						prim.vertex_buffer_views.at("NORMAL").buffer.Get(),
						prim.vertex_buffer_views.at("TANGENT").buffer.Get(),
						prim.vertex_buffer_views.at("TEXCOORD_0").buffer.Get(),
						prim.vertex_buffer_views.at("JOINTS_0").buffer.Get(),
						prim.vertex_buffer_views.at("WEIGHTS_0").buffer.Get(),
					};
					UINT strides[]
					{
						(UINT)prim.vertex_buffer_views.at("POSITION").stride_in_bytes,
						(UINT)prim.vertex_buffer_views.at("NORMAL").stride_in_bytes,
						(UINT)prim.vertex_buffer_views.at("TANGENT").stride_in_bytes,
						(UINT)prim.vertex_buffer_views.at("TEXCOORD_0").stride_in_bytes,
						(UINT)prim.vertex_buffer_views.at("JOINTS_0").stride_in_bytes,
						(UINT)prim.vertex_buffer_views.at("WEIGHTS_0").stride_in_bytes,
					};
					UINT offsets[_countof(vbs)]{};
					immediate_context->IASetVertexBuffers(0, _countof(vbs), vbs, strides, offsets);
					immediate_context->IASetIndexBuffer(
						prim.index_buffer_view.buffer.Get(),
						prim.index_buffer_view.format, 0);
					immediate_context->DrawIndexed(
						(UINT)prim.index_buffer_view.count(), 0, 0);
				}
			}
			for (int child : nd.children)
				traverse_static(child);
		} };

		for (int root : scenes.at(0).nodes)
			traverse_static(root);

		return; //バッチ描画はしない
	}

    primitive_constants prim_data{};
    XMStoreFloat4x4(&prim_data.world, XMLoadFloat4x4(&world));
    prim_data.skin        = -1;
    prim_data.has_tangent =  1;

    for (auto& bp : batched_primitives)
    {
        prim_data.material = bp.material;

        const material& mat{ materials.at(bp.material) };
        const int tex_indices[]
        {
            mat.data.pbr_metallic_roughness.basecolor_texture.index,
            mat.data.pbr_metallic_roughness.metallic_roughness_texture.index,
            mat.data.normal_texture.index,
            mat.data.emissive_texture.index,
            mat.data.occlusion_texture.index,
        };
        ID3D11ShaderResourceView* null_srv{};
        std::vector<ID3D11ShaderResourceView*> srvs(_countof(tex_indices));
        for (int i = 0; i < (int)srvs.size(); ++i)
            srvs[i] = tex_indices[i] > -1
                ? texture_resource_views.at(textures.at(tex_indices[i]).source).Get()
                : null_srv;
        immediate_context->PSSetShaderResources(1, (UINT)srvs.size(), srvs.data());

        immediate_context->UpdateSubresource(primitive_cbuffer.Get(), 0, 0, &prim_data, 0, 0);
        immediate_context->VSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());
        immediate_context->PSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());

        ID3D11Buffer* vbs[] =
        {
            bp.position_buffer.Get(),
            bp.normal_buffer.Get(),
            bp.tangent_buffer.Get(),
            bp.texcoord_buffer.Get(),
            bp.joint_buffer.Get(),
            bp.weight_buffer.Get(),
        };
        UINT strides[] = { 12, 12, 16, 8, 8, 16 };
        UINT offsets[6]{};
        immediate_context->IASetVertexBuffers(0, 6, vbs, strides, offsets);
        immediate_context->IASetIndexBuffer(bp.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        immediate_context->DrawIndexed(bp.index_count, 0, 0);
    }
}
