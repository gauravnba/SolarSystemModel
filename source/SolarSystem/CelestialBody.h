#pragma once

#include "DrawableGameComponent.h"
#include <DirectXMath.h>
#include <string>

namespace SolarSystem
{
	class CelestialBody final : public Library::GameComponent
	{
		RTTI_DECLARATIONS(CelestialBody, Library::GameComponent)

	public:
		CelestialBody(Library::Game* game, float rotation, const std::wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate, CelestialBody* orbitAround, bool isLit);
		virtual void Update(const Library::GameTime& gameTime) override;

		void* operator new(size_t i);
		void operator delete(void* p);

		DirectX::XMMATRIX WorldMatrix();
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ColorTexture();
		float BodySize();
		bool IsLit();

	private:
		float mRotationRate;
		float mRevolutionRate;
		float mAxialTilt;
		float mRotation;
		float mRevolution;
		float mOrbit;
		float mSize;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mColorTexture;
		DirectX::XMFLOAT4X4 mWorldMatrix;
		DirectX::XMMATRIX mScale;
		DirectX::XMMATRIX mOrbitalDistance;
		CelestialBody* mParent;
		bool mIsLit;
	};
}