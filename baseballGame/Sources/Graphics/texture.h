#pragma once

#include <d3d11.h>
#include "WICTextureLoader.h"
#include <wrl.h>
#include <map>
#include <string>
#include"misc.h"
// UNIT.16
#include <sstream>
#include <iomanip>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

HRESULT load_texture_from_file(ID3D11Device* device, const wchar_t* filename,
    ID3D11ShaderResourceView** shader_resource_view, D3D11_TEXTURE2D_DESC* texture2d_desc);

void release_all_textures();
// UNIT.16
HRESULT make_dummy_texture(ID3D11Device* device, ID3D11ShaderResourceView** shader_resource_view, DWORD value/*0xAABBGGRR*/, UINT dimension);

//UNIT.36
HRESULT load_texture_from_memory(ID3D11Device* device, const void* data, size_t size,
    ID3D11ShaderResourceView** shader_resource_view);