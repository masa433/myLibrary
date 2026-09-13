#pragma once
#include <DirectXMath.h>

class ScreenScaler
{
private:
	//基準解像度
	static constexpr float BASE_WIDTH = 1920.0f;
	static constexpr float BASE_HEIGHT = 1080.0f;

	float scaleX = 1.0f;
	float scaleY = 1.0f;

public:
	ScreenScaler() = default;
	
	//現在の画面解像度に基づいてスケールを計算する
	void UpdateScale(float currentWidth, float currentHeight)
	{
		scaleX = currentWidth / BASE_WIDTH;
		scaleY = currentHeight / BASE_HEIGHT;
	}

	//位置のX座標をスケーリングする
	float ScaleX(float baseX) const
	{
		return baseX * scaleX;
	}

	//位置のY座標をスケーリングする
	float ScaleY(float baseY) const
	{
		return baseY * scaleY;
	}

	//サイズの幅をスケーリングする
	float ScaleWidth(float baseWidth) const
	{
		return baseWidth * scaleX;
	}

	//サイズの高さをスケーリングする
	float ScaleHeight(float baseHeight) const
	{
		return baseHeight * scaleY;
	}

	/// DirectX::XMFLOAT2の位置をスケーリング
	DirectX::XMFLOAT2 Scale(const DirectX::XMFLOAT2& basePos) const
	{
		return { ScaleX(basePos.x), ScaleY(basePos.y) };
	}

	/// DirectX::XMFLOAT2のサイズをスケーリング
	DirectX::XMFLOAT2 ScaleSize(const DirectX::XMFLOAT2& baseSize) const
	{
		return { ScaleWidth(baseSize.x), ScaleHeight(baseSize.y) };
	}

	float GetUniformScale() const { return (std::min)(scaleX, scaleY); }

	// 右下基準: 1920の右から baseX 離れた位置を、現在の解像度でも右から同じ距離に配置
	float ToRightAligned(float baseX, float currentScreenWidth) const
	{
		return currentScreenWidth - (BASE_WIDTH - baseX);
	}

	// 下基準: 1080の下から baseY 離れた位置を、現在の解像度でも下から同じ距離に配置
	float ToBottomAligned(float baseY, float currentScreenHeight) const
	{
		return currentScreenHeight - (BASE_HEIGHT - baseY);
	}

	// 右下基準の位置
	DirectX::XMFLOAT2 ToRightBottomAligned(const DirectX::XMFLOAT2& basePos, float currentScreenWidth, float currentScreenHeight) const
	{
		return {
			ToRightAligned(basePos.x, currentScreenWidth),
			ToBottomAligned(basePos.y, currentScreenHeight)
		};
	}


	float GetScaleX() const { return scaleX; }
	float GetScaleY() const { return scaleY; }
	float GetCurrentWidth() const { return BASE_WIDTH * scaleX; }
	float GetCurrentHeight() const { return BASE_HEIGHT * scaleY; }

	static float GetBaseWidth() { return BASE_WIDTH; }
	static float GetBaseHeight() { return BASE_HEIGHT; }
};