#pragma once

#include "DrawableGameComponent.h"
#include "RenderStateHelper.h"
#include "PointLight.h"
#include "CelestialBody.h"
#include <DirectXMath.h>
#include <DirectXColors.h>

namespace Library
{
	class Mesh;
	class ProxyModel;
	class KeyboardComponent;	
}

namespace DirectX
{
	class SpriteBatch;
	class SpriteFont;
}

namespace Rendering
{
	class SolarSystemRender final : public Library::DrawableGameComponent
	{
		RTTI_DECLARATIONS(SolarSystemRender, Library::DrawableGameComponent)

	public:
		SolarSystemRender(Library::Game& game, const std::shared_ptr<Library::Camera>& camera);

		bool AnimationEnabled() const;
		void SetAnimationEnabled(bool enabled);

		virtual void Initialize() override;
		virtual void Update(const Library::GameTime& gameTime) override;
		virtual void Draw(const Library::GameTime& gameTime) override;

	private:
		struct VSCBufferPerFrame
		{
			DirectX::XMFLOAT3 LightPosition;
			float LightRadius;

			VSCBufferPerFrame() :
				LightPosition(Library::Vector3Helper::Zero), LightRadius(50.0f) { }
			VSCBufferPerFrame(const DirectX::XMFLOAT3 lightPosition, float lightRadius) :
				LightPosition(lightPosition), LightRadius(lightRadius) { }
		};

		struct VSCBufferPerObject
		{
			DirectX::XMFLOAT4X4 WorldViewProjection;
			DirectX::XMFLOAT4X4 World;

			VSCBufferPerObject() = default;
			VSCBufferPerObject(const DirectX::XMFLOAT4X4& wvp, const DirectX::XMFLOAT4X4& world) :
				WorldViewProjection(wvp), World(world) { }
		};

		struct PSCBufferPerFrame
		{
			DirectX::XMFLOAT3 CameraPosition;
			float Padding;
			DirectX::XMFLOAT3 AmbientColor;
			float Padding2;
			DirectX::XMFLOAT3 LightPosition;
			float Padding3;
			DirectX::XMFLOAT3 LightColor;
			float Padding4;

			PSCBufferPerFrame() :
				CameraPosition(Library::Vector3Helper::Zero), AmbientColor(Library::Vector3Helper::Zero),
				LightPosition(Library::Vector3Helper::Zero), LightColor(Library::Vector3Helper::Zero)
			{
			}

			PSCBufferPerFrame(const DirectX::XMFLOAT3& cameraPosition, const DirectX::XMFLOAT3& ambientColor, const DirectX::XMFLOAT3& lightPosition, const DirectX::XMFLOAT3& lightColor) :
				CameraPosition(cameraPosition), AmbientColor(ambientColor),
				LightPosition(lightPosition), LightColor(lightColor)
			{
			}
		};

		struct PSCBufferPerObject
		{
			DirectX::XMFLOAT3 SpecularColor;
			float SpecularPower;

			PSCBufferPerObject() :
				SpecularColor(1.0f, 1.0f, 1.0f), SpecularPower(128.0f) { }

			PSCBufferPerObject(const DirectX::XMFLOAT3& specularColor, float specularPower) :
				SpecularColor(specularColor), SpecularPower(specularPower) { }
		};

		void CreateVertexBuffer(const Library::Mesh& mesh, ID3D11Buffer** vertexBuffer) const;
		void ToggleAnimation();
		void ReturnToStart();
		void JumpToNextPlanet();

		static const float LightModulationRate;
		static const float LightMovementRate;

		PSCBufferPerFrame mPSCBufferPerFrameData;
		VSCBufferPerFrame mVSCBufferPerFrameData;
		VSCBufferPerObject mVSCBufferPerObjectData;
		PSCBufferPerObject mPSCBufferPerObjectData;
		Library::PointLight mPointLight;
		Library::RenderStateHelper mRenderStateHelper;
		Library::Skybox mSkyBox;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mSunShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerFrame;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;
		Library::KeyboardComponent* mKeyboard;
		std::uint32_t mIndexCount;
		std::unique_ptr<DirectX::SpriteBatch> mSpriteBatch;
		std::unique_ptr<DirectX::SpriteFont> mSpriteFont;
		DirectX::XMFLOAT2 mTextPosition;
		std::vector<std::unique_ptr<SolarSystem::CelestialBody>> mCelestialBodiesList;
		std::uint32_t mCurrentPlanet;
		bool mAnimationEnabled;
	};
}
