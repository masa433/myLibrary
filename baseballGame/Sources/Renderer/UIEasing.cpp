#include "UiEasing.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float PI = 3.14159265358979323846f;

	float EaseOutBounce(float time)
	{
		constexpr float n1 = 7.5625f;
		constexpr float d1 = 2.75f;

		if (time < 1.0f / d1)
		{
			return n1 * time * time;
		}
		if (time < 2.0f / d1)
		{
			time -= 1.5f / d1;
			return n1 * time * time + 0.75f;
		}
		if (time < 2.5f / d1)
		{
			time -= 2.25f / d1;
			return n1 * time * time + 0.9375f;
		}

		time -= 2.625f / d1;
		return n1 * time * time + 0.984375f;
	}
}

float UiEasing::Evaluate(EasingType type, float time)
{
	time = std::clamp(time, 0.0f, 1.0f);

	switch (type)
	{
	case EasingType::Linear:
		return time;

	case EasingType::InQuad:
		return time * time;
	case EasingType::OutQuad:
		return 1.0f - (1.0f - time) * (1.0f - time);
	case EasingType::InOutQuad:
		return time < 0.5f ? 2.0f * time * time : 1.0f - std::pow(-2.0f * time + 2.0f, 2.0f) * 0.5f;

	case EasingType::InCubic:
		return time * time * time;
	case EasingType::OutCubic:
		return 1.0f - std::pow(1.0f - time, 3.0f);
	case EasingType::InOutCubic:
		return time < 0.5f ? 4.0f * time * time * time : 1.0f - std::pow(-2.0f * time + 2.0f, 3.0f) * 0.5f;

	case EasingType::InQuart:
		return time * time * time * time;
	case EasingType::OutQuart:
		return 1.0f - std::pow(1.0f - time, 4.0f);
	case EasingType::InOutQuart:
		return time < 0.5f ? 8.0f * time * time * time * time : 1.0f - std::pow(-2.0f * time + 2.0f, 4.0f) * 0.5f;

	case EasingType::InQuint:
		return time * time * time * time * time;
	case EasingType::OutQuint:
		return 1.0f - std::pow(1.0f - time, 5.0f);
	case EasingType::InOutQuint:
		return time < 0.5f ? 16.0f * time * time * time * time * time : 1.0f - std::pow(-2.0f * time + 2.0f, 5.0f) * 0.5f;

	case EasingType::InSine:
		return 1.0f - std::cos((time * PI) * 0.5f);
	case EasingType::OutSine:
		return std::sin((time * PI) * 0.5f);
	case EasingType::InOutSine:
		return -(std::cos(PI * time) - 1.0f) * 0.5f;

	case EasingType::InExp:
		return time == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * time - 10.0f);
	case EasingType::OutExp:
		return time == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * time);
	case EasingType::InOutExp:
		if (time == 0.0f)
		{
			return 0.0f;
		}
		if (time == 1.0f)
		{
			return 1.0f;
		}
		return time < 0.5f
			? std::pow(2.0f, 20.0f * time - 10.0f) * 0.5f
			: (2.0f - std::pow(2.0f, -20.0f * time + 10.0f)) * 0.5f;

	case EasingType::InCirc:
		return 1.0f - std::sqrt(1.0f - time * time);
	case EasingType::OutCirc:
		return std::sqrt(1.0f - std::pow(time - 1.0f, 2.0f));
	case EasingType::InOutCirc:
		return time < 0.5f
			? (1.0f - std::sqrt(1.0f - std::pow(2.0f * time, 2.0f))) * 0.5f
			: (std::sqrt(1.0f - std::pow(-2.0f * time + 2.0f, 2.0f)) + 1.0f) * 0.5f;

	case EasingType::InBounce:
		return 1.0f - EaseOutBounce(1.0f - time);
	case EasingType::OutBounce:
		return EaseOutBounce(time);
	case EasingType::InOutBounce:
		return time < 0.5f
			? (1.0f - EaseOutBounce(1.0f - 2.0f * time)) * 0.5f
			: (1.0f + EaseOutBounce(2.0f * time - 1.0f)) * 0.5f;

	case EasingType::InBack:
	{
		constexpr float c1 = 1.70158f;
		constexpr float c3 = c1 + 1.0f;
		return c3 * time * time * time - c1 * time * time;
	}
	case EasingType::OutBack:
	{
		constexpr float c1 = 1.70158f;
		constexpr float c3 = c1 + 1.0f;
		return 1.0f + c3 * std::pow(time - 1.0f, 3.0f) + c1 * std::pow(time - 1.0f, 2.0f);
	}
	case EasingType::InOutBack:
	{
		constexpr float c1 = 1.70158f;
		constexpr float c2 = c1 * 1.525f;
		return time < 0.5f
			? std::pow(2.0f * time, 2.0f) * ((c2 + 1.0f) * 2.0f * time - c2) * 0.5f
			: (std::pow(2.0f * time - 2.0f, 2.0f) * ((c2 + 1.0f) * (time * 2.0f - 2.0f) + c2) + 2.0f) * 0.5f;
	}
	default:
		return time;
	}
}

float UiEasing::Lerp(float start, float end, float time, EasingType type)
{
	const float easedTime = Evaluate(type, time);
	return start + (end - start) * easedTime;
}

DirectX::XMFLOAT2 UiEasing::Lerp(const DirectX::XMFLOAT2& start, const DirectX::XMFLOAT2& end, float time, EasingType type)
{
	const float easedTime = Evaluate(type, time);
	return DirectX::XMFLOAT2(
		start.x + (end.x - start.x) * easedTime,
		start.y + (end.y - start.y) * easedTime
	);
}

DirectX::XMFLOAT3 UiEasing::Lerp(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, float time, EasingType type)
{
	const float easedTime = Evaluate(type, time);
	return DirectX::XMFLOAT3(
		start.x + (end.x - start.x) * easedTime,
		start.y + (end.y - start.y) * easedTime,
		start.z + (end.z - start.z) * easedTime
	);
}

DirectX::XMFLOAT4 UiEasing::Lerp(const DirectX::XMFLOAT4& start, const DirectX::XMFLOAT4& end, float time, EasingType type)
{
	const float easedTime = Evaluate(type, time);
	return DirectX::XMFLOAT4(
		start.x + (end.x - start.x) * easedTime,
		start.y + (end.y - start.y) * easedTime,
		start.z + (end.z - start.z) * easedTime,
		start.w + (end.w - start.w) * easedTime
	);
}
