#include "pch.h"
#include "Planet.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace SolarSystem
{
	RTTI_DEFINITIONS(Planet)

	Planet::Planet(Game* game , float rotation, const wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate, Planet* orbitAround) :
		mRotationRate(rotation), mRevolutionRate(revolutionRate), mWorldMatrix(MatrixHelper::Identity), mAxialTilt(axialTilt), mOrbitAround(orbitAround), mRotation(0.0f), mRevolution(0.0f), mOrbit(orbitalDistance)
	{
		mScale = XMMatrixScaling(scale, scale, scale);
		mOrbitalDistance = XMMatrixTranslation(orbitalDistance, 0.0f, 0.0f);

		ThrowIfFailed(CreateWICTextureFromFile(game->Direct3DDevice(), texture.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");
	}

	void* Planet::operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void Planet::operator delete(void* p)
	{
		return _mm_free(p);
	}

	XMMATRIX Planet::WorldMatrix()
	{
		return XMLoadFloat4x4(&mWorldMatrix);
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Planet::ColorTexture()
	{
		return mColorTexture;
	}

	void Planet::Update(const Library::GameTime& gameTime)
	{
			mRotation += gameTime.ElapsedGameTimeSeconds().count() * mRotationRate;
			mRevolution += gameTime.ElapsedGameTimeSeconds().count() * mRevolutionRate;
			
			if (mOrbitAround == nullptr)
			{
				XMStoreFloat4x4(&mWorldMatrix, mScale * XMMatrixRotationY(mRotation) * XMMatrixRotationZ(mAxialTilt) * mOrbitalDistance * XMMatrixRotationY(mRevolution));
			}

			else
			{
				float angle = mOrbitAround->mRevolution;
				XMStoreFloat4x4(&mWorldMatrix, mScale * XMMatrixRotationY(mRotation) * XMMatrixRotationZ(mAxialTilt) * mOrbitalDistance * XMMatrixRotationY(mRevolution) * XMMatrixTranslation(mOrbitAround->mOrbit * XMScalarCos(angle), 0.0f, mOrbitAround->mOrbit * XMScalarSin(angle) * -1.0f));
			}
	}
}