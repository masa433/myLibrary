#pragma once

#include <DirectXMath.h>

// Easing functions for UI animation.


class UiEasing
{
public:

	enum EasingType
	{
		Linear,
		InQuad,
		OutQuad,
		InOutQuad,
		InCubic,
		OutCubic,
		InOutCubic,
		InQuart,
		OutQuart,
		InOutQuart,
		InQuint,
		OutQuint,
		InOutQuint,
		InSine,
		OutSine,
		InOutSine,
		InExp,
		OutExp,
		InOutExp,
		InCirc,
		OutCirc,
		InOutCirc,
		InBounce,
		OutBounce,
		InOutBounce,
		InBack,
		OutBack,
		InOutBack
	};
	UiEasing() = delete;

	static float Evaluate(EasingType type, float time);
	static float Lerp(float start, float end, float time, EasingType type = EasingType::Linear);
	static DirectX::XMFLOAT2 Lerp(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float time, EasingType type = EasingType::Linear);
	static DirectX::XMFLOAT3 Lerp(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float time, EasingType type = EasingType::Linear);
	static DirectX::XMFLOAT4 Lerp(const DirectX::XMFLOAT4& start, const DirectX::XMFLOAT4& end, float time, EasingType type = EasingType::Linear);
};
