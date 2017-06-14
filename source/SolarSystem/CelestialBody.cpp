#include "pch.h"
#include "CelestialBody.h"

using namespace std;
using namespace Library;
using namespace DirectX;

namespace SolarSystem
{
	RTTI_DEFINITIONS(CelestialBody)

	CelestialBody::CelestialBody(Game* game , float rotation, const wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate, CelestialBody* orbitAround, bool isLit) :
		mRotationRate(rotation), mRevolutionRate(revolutionRate), mWorldMatrix(MatrixHelper::Identity), mAxialTilt(axialTilt), mParent(orbitAround), 
		mRotation(0.0f), mRevolution(0.0f), mOrbit(orbitalDistance), mIsLit(isLit), mSize(scale)
	{
		mScale = XMMatrixScaling(scale, scale, scale);
		mOrbitalDistance = XMMatrixTranslation(orbitalDistance, 0.0f, 0.0f);

		ThrowIfFailed(CreateWICTextureFromFile(game->Direct3DDevice(), texture.c_str(), nullptr, mColorTexture.ReleaseAndGetAddressOf()), "CreateDDSTextureFromFile() failed.");
	}

	void* CelestialBody::operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void CelestialBody::operator delete(void* p)
	{
		return _mm_free(p);
	}

	XMMATRIX CelestialBody::WorldMatrix()
	{
		return XMLoadFloat4x4(&mWorldMatrix);
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CelestialBody::ColorTexture()
	{
		return mColorTexture;
	}

	bool CelestialBody::IsLit()
	{
		return mIsLit;
	}

	void CelestialBody::Update(const Library::GameTime& gameTime)
	{
			mRotation += gameTime.ElapsedGameTimeSeconds().count() * mRotationRate;
			mRevolution += gameTime.ElapsedGameTimeSeconds().count() * mRevolutionRate;
			
			if (mParent == nullptr)
			{
				XMStoreFloat4x4(&mWorldMatrix, mScale * XMMatrixRotationY(mRotation) * XMMatrixRotationZ(mAxialTilt) * mOrbitalDistance * XMMatrixRotationY(mRevolution));
			}

			else
			{
				XMFLOAT3 parentPosition;
				MatrixHelper::GetTranslation(XMLoadFloat4x4(&mParent->mWorldMatrix), parentPosition);
				XMStoreFloat4x4(&mWorldMatrix, mScale * XMMatrixRotationY(mRotation) * XMMatrixRotationZ(mAxialTilt) * mOrbitalDistance * XMMatrixRotationY(mRevolution) * XMMatrixTranslation(parentPosition.x, parentPosition.y, parentPosition.z));
			}
	}

	float CelestialBody::BodySize()
	{
		return mSize;
	}
}