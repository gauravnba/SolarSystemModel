#pragma once

#include "DrawableGameComponent.h"
#include <DirectXMath.h>
#include <string>

namespace SolarSystem
{
	class Planet final : public Library::GameComponent
	{
		RTTI_DECLARATIONS(Planet, Library::GameComponent)

	public:
		Planet(float rotation, const std::wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate);

		void ToggleAnimation();

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;

		DirectX::XMMATRIX WorldMatrix();
		std::wstring TextureString();

	private:
		float mRotationRate;
		float mRevolutionRate;
		DirectX::XMMATRIX mWorldMatrix;
		std::wstring mTextureString;
		DirectX::XMVECTOR mAxis;
		bool mAnimationEnabled;
	};
}