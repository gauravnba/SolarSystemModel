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
		Planet(Library::Game* game, float rotation, const std::wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate, Planet* orbitAround);
		virtual void Update(const Library::GameTime& gameTime) override;

		DirectX::XMMATRIX WorldMatrix();
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ColorTexture();

	private:
		float mRotationRate;
		float mRevolutionRate;
		float mAxialTilt;
		float mRotation;
		float mRevolution;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		DirectX::XMFLOAT4X4 mWorldMatrix;
		DirectX::XMMATRIX mScale;
		DirectX::XMMATRIX mOrbitalDistance;
		Planet* mOrbitAround;
	};
}