#include "pch.h"
#include "shader.h"
#include <cassert>
#include "misc.h"

HRESULT create_vs_from_cso(ID3D11Device* device, const char* cso_name, ID3D11VertexShader** vertex_shader,
    ID3D11InputLayout** input_layout, D3D11_INPUT_ELEMENT_DESC* input_element_desc, UINT num_elements) {
    if (!device || !cso_name || !vertex_shader || (input_layout && (!input_element_desc || num_elements == 0))) return E_INVALIDARG;
    FILE* fp{ nullptr };
    fopen_s(&fp, cso_name, "rb");
    if (!fp) { _ASSERT_EXPR(FALSE, "Vertex shader file could not be opened"); return HRESULT_FROM_WIN32(GetLastError() ? GetLastError() : ERROR_FILE_NOT_FOUND); }

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); _ASSERT_EXPR(FALSE, "Vertex shader file seek failed"); return E_FAIL; }
    long cso_sz{ ftell(fp) };
    if (cso_sz <= 0 || fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); _ASSERT_EXPR(FALSE, "Vertex shader file is empty or invalid"); return E_FAIL; }

    unique_ptr<unsigned char[]> cso_data{ make_unique<unsigned char[]>(cso_sz) };
    const bool readOk = fread(cso_data.get(), static_cast<size_t>(cso_sz), 1, fp) == 1;
    fclose(fp);
    if (!readOk) { _ASSERT_EXPR(FALSE, "Vertex shader file read failed"); return E_FAIL; }

    HRESULT hr = device->CreateVertexShader(cso_data.get(), static_cast<SIZE_T>(cso_sz), nullptr, vertex_shader);
    if (FAILED(hr) || !*vertex_shader) { _ASSERT_EXPR(FALSE, "Vertex shader creation failed"); return FAILED(hr) ? hr : E_FAIL; }

    if (input_layout)
    {
        hr = device->CreateInputLayout(input_element_desc, num_elements,
            cso_data.get(), cso_sz, input_layout);
        if (FAILED(hr) || !*input_layout)
        {
            (*vertex_shader)->Release();
            *vertex_shader = nullptr;
            _ASSERT_EXPR(FALSE, "Input layout creation failed");
            return FAILED(hr) ? hr : E_FAIL;
        }
    }
    return hr;
}


HRESULT create_ps_from_cso(ID3D11Device* device, const char* cso_name, ID3D11PixelShader** pixel_shader)
{
    if (!device || !cso_name || !pixel_shader) return E_INVALIDARG;
    FILE* fp{ nullptr };
    fopen_s(&fp, cso_name, "rb");
    if (!fp) { _ASSERT_EXPR(FALSE, "Pixel shader file could not be opened"); return HRESULT_FROM_WIN32(GetLastError() ? GetLastError() : ERROR_FILE_NOT_FOUND); }

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); _ASSERT_EXPR(FALSE, "Pixel shader file seek failed"); return E_FAIL; }
    long cso_sz{ ftell(fp) };
    if (cso_sz <= 0 || fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); _ASSERT_EXPR(FALSE, "Pixel shader file is empty or invalid"); return E_FAIL; }

    unique_ptr<unsigned char[]> cso_data{ make_unique<unsigned char[]>(cso_sz) };
    const bool readOk = fread(cso_data.get(), static_cast<size_t>(cso_sz), 1, fp) == 1;
    fclose(fp);
    if (!readOk) { _ASSERT_EXPR(FALSE, "Pixel shader file read failed"); return E_FAIL; }

    HRESULT hr = device->CreatePixelShader(cso_data.get(), static_cast<SIZE_T>(cso_sz), nullptr, pixel_shader);
    if (FAILED(hr) || !*pixel_shader) { _ASSERT_EXPR(FALSE, "Pixel shader creation failed"); return FAILED(hr) ? hr : E_FAIL; }

    return hr;
}
