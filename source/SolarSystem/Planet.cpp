#include "pch.h"
//#include "Planet.h"
//
//using namespace std;
//using namespace DirectX;
//
//namespace SolarSystem
//{
//	Planet::Planet(float rotation, const wstring& texture, float axialTilt, float orbitalDistance, float scale, float revolutionRate) :
//		mRotationRate(rotation), mTextureString(texture), mRevolutionRate(revolutionRate)
//	{
//		XMFLOAT4 temp(0.0f, 1.0f, 0.0f, 0.0f);
//		mAxis = XMVector4Transform(XMLoadFloat4(&temp), XMMatrixRotationZ(axialTilt));
//
//		mWorldMatrix *= XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(orbitalDistance, 0.0f, 0.0f);
//	}
//}