#pragma once

#include"System/ModelRenderer.h"
#include"Character.h"


class Pitcher :public Character
{
public:
	Pitcher() {};
	~Pitcher() override {};

	static Pitcher& Instance()
	{
		static Pitcher instance;
		return instance;
	}

	void Initialize();

	void Finalize();

	//XVˆ—
	void Update(float elapsedTime);

	//•`‰æˆ—
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	void DrawImGui();

private:

	enum class State
	{
		Throwing,
	};

	enum Animation
	{
		Pitching,
		EnumCount
	};

	void SetPitchingState();

	void UpdatePitchingState(float elapsedTime);

	State state = State::Throwing;

private:

	std::unique_ptr<Model>	pitcher = nullptr;
};
