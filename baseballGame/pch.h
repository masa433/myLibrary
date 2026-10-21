#pragma once
//C++標準ライブラリ
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <deque>
#include <cmath>
#include <functional>
#include <mutex>
#include <thread>
#include <cstdio>
#include <random>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <queue>
#include <map>

//Windows / DirectX関連
#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl.h>
#include <Windows.h>
#include <wrl/client.h>
#include <commdlg.h>//DirectXTex
#include <WICTextureLoader.h>
#include <d3dcompiler.h>
#include <math.h>
#include <stdlib.h>

//JSONライブラリ
#include "json.hpp"

//imgui
#ifdef USE_IMGUI
#include "imgui.h"
#endif

//エフェクシア
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>