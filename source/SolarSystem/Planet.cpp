#include "pch.h"
#include "Planet.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace SolarSystem
{
	RTTI_DEFINITIONS(Planet)

	Planet::Planet(Game* game , float rotation, const wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate, Planet* orbitAround) :
		mRotationRate(rotation), mRevolutionRate(revolutionRate), mWorldMatrix(MatrixHelper::Identity), mAxialTilt(axialTilt), mOrbitAround(orbitAround)
	{
		XMFLOAT4 temp(0.0f, 1.0f, 0.0f, 0.0f);
		mAxis = XMVector4Transform(XMLoadFloat4(&temp), XMMatrixRotationZ(axialTilt));

		mScale = XMMatrixScaling(scale, scale, scale);
		mOrbitalDistance = XMMatrixTranslation(orbitalDistance, 0.0f, 0.0f);

		ThrowIfFailed(CreateWICTextureFromFile(game->Direct3DDevice(), texture.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");
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
			static float rotation = 0.0f;
			static float revolution = 0.0f;

			rotation += gameTime.ElapsedGameTimeSeconds().count() * mRotationRate;
			revolution += gameTime.ElapsedGameTimeSeconds().count() * mRevolutionRate;

			//mAxis = XMVector4Transform(mAxis, XMMatrixRotationY(revolution));
			
			if (mOrbitAround == nullptr)
			{
				XMStoreFloat4x4(&mWorldMatrix, mScale * XMMatrixRotationY(rotation) * XMMatrixRotationZ(mAxialTilt) * mOrbitalDistance * XMMatrixRotationY(revolution));
			}

			else
			{
				//XMMATRIX orbitAround = mOrbitAround->mOrbitalDistance;
				//XMStoreFloat4x4(&mWorldMatrix, mScale * XMMatrixRotationZ(mAxialTilt) * XMMatrixRotationAxis(mAxis, rotation) * orbitAround * mOrbitalDistance * XMMatrixRotationAxis(mOrbitAround->mAxis, revolution));
			}
	}
}