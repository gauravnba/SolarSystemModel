#include "pch.h"

using namespace std;
using namespace Library;
using namespace DirectX;
using namespace SolarSystem;

namespace Rendering
{
	RTTI_DEFINITIONS(SolarSystemRender)

	const float SolarSystemRender::LightModulationRate = UCHAR_MAX;
	const float SolarSystemRender::LightMovementRate = 10.0f;

	SolarSystemRender::SolarSystemRender(Game& game, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(game, camera), mPointLight(game, XMFLOAT3(0.0f, 0.0f, 0.0f), 30000.0f),
		mRenderStateHelper(game), mIndexCount(0), mTextPosition(0.0f, 40.0f), mAnimationEnabled(true), mSkyBox(game, camera, L"Content\\Textures\\stars.dds", 1000.0f), mCurrentPlanet(0)
	{
	}

	bool SolarSystemRender::AnimationEnabled() const
	{
		return mAnimationEnabled;
	}

	void SolarSystemRender::SetAnimationEnabled(bool enabled)
	{
		mAnimationEnabled = enabled;
	}

	void SolarSystemRender::Initialize()
	{
		// Load a compiled vertex shader
		vector<char> compiledVertexShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\SolarSystemVS.cso", compiledVertexShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateVertexShader(&compiledVertexShader[0], compiledVertexShader.size(), nullptr, mVertexShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedVertexShader() failed.");

		// Load compiled pixel shaders
		vector<char> compiledPixelShader;
		Utility::LoadBinaryFile(L"Content\\Shaders\\SunShaderPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mSunShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		compiledPixelShader.clear();
		Utility::LoadBinaryFile(L"Content\\Shaders\\SolarSystemPS.cso", compiledPixelShader);
		ThrowIfFailed(mGame->Direct3DDevice()->CreatePixelShader(&compiledPixelShader[0], compiledPixelShader.size(), nullptr, mPixelShader.ReleaseAndGetAddressOf()), "ID3D11Device::CreatedPixelShader() failed.");

		// Create an input layout
		D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		ThrowIfFailed(mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions, ARRAYSIZE(inputElementDescriptions), &compiledVertexShader[0], compiledVertexShader.size(), mInputLayout.ReleaseAndGetAddressOf()), "ID3D11Device::CreateInputLayout() failed.");

		// Load the model
		Library::Model model("Content\\Models\\Sphere.obj.bin");

		// Create vertex and index buffers for the model
		Library::Mesh* mesh = model.Meshes().at(0).get();
		CreateVertexBuffer(*mesh, mVertexBuffer.ReleaseAndGetAddressOf());
		mesh->CreateIndexBuffer(*mGame->Direct3DDevice(), mIndexBuffer.ReleaseAndGetAddressOf());
		mIndexCount = static_cast<uint32_t>(mesh->Indices().size());

		// Create constant buffers
		D3D11_BUFFER_DESC constantBufferDesc = { 0 };
		constantBufferDesc.ByteWidth = sizeof(VSCBufferPerFrame);
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVSCBufferPerFrame.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(VSCBufferPerObject);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mVSCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PSCBufferPerFrame);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPSCBufferPerFrame.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		constantBufferDesc.ByteWidth = sizeof(PSCBufferPerObject);
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, mPSCBufferPerObject.ReleaseAndGetAddressOf()), "ID3D11Device::CreateBuffer() failed.");

		// Create text rendering helpers
		mSpriteBatch = make_unique<SpriteBatch>(mGame->Direct3DDeviceContext());
		mSpriteFont = make_unique<SpriteFont>(mGame->Direct3DDevice(), L"Content\\Fonts\\Arial_14_Regular.spritefont");

		// Retrieve the keyboard service
		mKeyboard = reinterpret_cast<KeyboardComponent*>(mGame->Services().GetService(KeyboardComponent::TypeIdClass()));
		
		// Setup the point light
		mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
		mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
		mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
		mPSCBufferPerFrameData.LightColor = ColorHelper::ToFloat3(mPointLight.Color(), true);

		// Update the vertex and pixel shader constant buffers
		mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
		mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &mPSCBufferPerObjectData, 0, 0);

		mSkyBox.Initialize();

		// Earth properties declarations
		const float earthRotation = XM_PI;
		const float earthAxialTilt = 0.4101524f;
		const float earthScale = 1.0f;
		const float earthOrbitalDistance = 500.0f;
		const float earthRevolution = earthRotation / 365;

		// Populate the planet list
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.0408f, L"Content\\Textures\\2k_sun.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.0f, earthScale * 20.0f, earthRevolution, nullptr, false)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.017f, L"Content\\Textures\\mercurymap.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.387f, earthScale * 0.382f, earthRevolution * 4.149f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.004f, L"Content\\Textures\\venusmap.jpg", earthAxialTilt * 0.959f, earthOrbitalDistance * 0.723f, earthScale * 0.949f, earthRevolution * 1.624f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation, L"Content\\Textures\\EarthComposite.jpg", earthAxialTilt, earthOrbitalDistance, earthScale, earthRevolution, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRevolution * 12, L"Content\\Textures\\moonmap2k.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.05f, earthScale / 20, earthRevolution * 12, mCelestialBodiesList[3].get(), true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation, L"Content\\Textures\\marsmap1k.jpg", 0.4392f, earthOrbitalDistance * 1.524f, earthScale * 0.532f, earthRevolution * 0.531f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 2.4f, L"Content\\Textures\\jupiter2_2k.jpg", 0.05352f, earthOrbitalDistance * 5.203f, earthScale * 11.19f, earthRevolution * 0.084f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.01f, L"Content\\Textures\\callisto.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.4f, earthScale / 3, earthRotation * 2.4f / 16.7f, mCelestialBodiesList[6].get(), true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.2f, L"Content\\Textures\\europa.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.3f, earthScale / 4, earthRotation * 2.4f / 3.551f, mCelestialBodiesList[6].get(), true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.05f, L"Content\\Textures\\ganymede.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.35f, earthScale / 2.5f, earthRotation * 2.4f / 7.155f, mCelestialBodiesList[6].get(), true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.4f, L"Content\\Textures\\Io.png", earthAxialTilt * 0, earthOrbitalDistance * 0.25f, earthScale / 3, earthRotation * 2.4f / 1.769f, mCelestialBodiesList[6].get(), true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 2.3f, L"Content\\Textures\\saturnmap.jpg", 0.4712f, earthOrbitalDistance * 9.582f, earthScale * 9.26f, earthRevolution * 0.034f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 1.39f, L"Content\\Textures\\uranusmap.jpg", 1.6927f, earthOrbitalDistance * 19.20f, earthScale * 4.01f, earthRevolution * 0.011f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 1.489f, L"Content\\Textures\\neptunemap.jpg", 0.5166f, earthOrbitalDistance * 30.5f, earthScale * 3.88f, earthRevolution * 0.0061f, nullptr, true)));
		mCelestialBodiesList.push_back(make_unique<CelestialBody>(CelestialBody(mGame, earthRotation * 0.156f, L"Content\\Textures\\plutomap2k.jpg", 2.129f, earthOrbitalDistance * 39.48f, earthScale * 0.18f, earthRevolution * 0.004f, nullptr, true)));
	}

	void SolarSystemRender::Update(const GameTime& gameTime)
	{
		if (mAnimationEnabled)
		{
			for (uint32_t i = 0; i < mCelestialBodiesList.size(); ++i)
			{
				mCelestialBodiesList[i]->Update(gameTime);
			}
		}

		if (mKeyboard != nullptr)
		{
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
			{
				ToggleAnimation();
			}
			if (mKeyboard->WasKeyPressedThisFrame(Keys::R))
			{
				ReturnToStart();
			}
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Up))
			{
				JumpToNextPlanet();
			}
		}

		mSkyBox.Update(gameTime);
	}

	void SolarSystemRender::Draw(const GameTime& gameTime)
	{
		assert(mCamera != nullptr);

		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		UINT stride = sizeof(VertexPositionTextureNormal);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
		direct3DDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);

		for (uint32_t i = 0; i < mCelestialBodiesList.size(); ++i)
		{
			if (mCelestialBodiesList[i]->IsLit())
			{
				direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);
			}
			else
			{
				direct3DDeviceContext->PSSetShader(mSunShader.Get(), nullptr, 0);
			}

			XMMATRIX worldMatrix = mCelestialBodiesList[i]->WorldMatrix();
			XMMATRIX wvp = worldMatrix * mCamera->ViewProjectionMatrix();
			wvp = XMMatrixTranspose(wvp);
			XMStoreFloat4x4(&mVSCBufferPerObjectData.WorldViewProjection, wvp);
			XMStoreFloat4x4(&mVSCBufferPerObjectData.World, XMMatrixTranspose(worldMatrix));
			direct3DDeviceContext->UpdateSubresource(mVSCBufferPerObject.Get(), 0, nullptr, &mVSCBufferPerObjectData, 0, 0);

			ID3D11Buffer* VSConstantBuffers[] = { mVSCBufferPerFrame.Get(), mVSCBufferPerObject.Get() };
			direct3DDeviceContext->VSSetConstantBuffers(0, ARRAYSIZE(VSConstantBuffers), VSConstantBuffers);

			mPSCBufferPerFrameData.CameraPosition = mCamera->Position();
			direct3DDeviceContext->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);

			ID3D11Buffer* PSConstantBuffers[] = { mPSCBufferPerFrame.Get(), mPSCBufferPerObject.Get() };
			direct3DDeviceContext->PSSetConstantBuffers(0, ARRAYSIZE(PSConstantBuffers), PSConstantBuffers);

			ID3D11ShaderResourceView* PSShaderResources[] = { mCelestialBodiesList[i]->ColorTexture().Get() };
			direct3DDeviceContext->PSSetShaderResources(0, ARRAYSIZE(PSShaderResources), PSShaderResources);
			direct3DDeviceContext->PSSetSamplers(0, 1, SamplerStates::TrilinearWrap.GetAddressOf());

			direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
		}

		mSkyBox.Draw(gameTime);
		
		// Draw help text
		mRenderStateHelper.SaveAll();
		mSpriteBatch->Begin();

		wostringstream helpLabel;
		helpLabel << L"Move(Mouse + WASD)" << "\n";
		helpLabel << L"Change Camera Speed (Scroll Wheel): " << static_pointer_cast<FirstPersonCamera>(mCamera)->MovementFactor() << "\n";
		helpLabel << L"Jump to next celestial body (Up)" << "\n";
		helpLabel << L"Return to Sun (R)" << "\n";
		helpLabel << L"Toggle Animation (Space)" << "\n";
	
		mSpriteFont->DrawString(mSpriteBatch.get(), helpLabel.str().c_str(), mTextPosition);
		mSpriteBatch->End();
		mRenderStateHelper.RestoreAll();
	}

	void SolarSystemRender::CreateVertexBuffer(const Mesh& mesh, ID3D11Buffer** vertexBuffer) const
	{
		const vector<XMFLOAT3>& sourceVertices = mesh.Vertices();
		const vector<XMFLOAT3>& sourceNormals = mesh.Normals();
		const auto& sourceUVs = mesh.TextureCoordinates().at(0);

		vector<VertexPositionTextureNormal> vertices;
		vertices.reserve(sourceVertices.size());
		for (UINT i = 0; i < sourceVertices.size(); i++)
		{
			const XMFLOAT3& position = sourceVertices.at(i);
			const XMFLOAT3& uv = sourceUVs->at(i);
			const XMFLOAT3& normal = sourceNormals.at(i);

			vertices.push_back(VertexPositionTextureNormal(XMFLOAT4(position.x, position.y, position.z, 1.0f), XMFLOAT2(uv.x, uv.y), normal));
		}
		D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
		vertexBufferDesc.ByteWidth = sizeof(VertexPositionTextureNormal) * static_cast<UINT>(vertices.size());
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexSubResourceData = { 0 };
		vertexSubResourceData.pSysMem = &vertices[0];
		ThrowIfFailed(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, vertexBuffer), "ID3D11Device::CreateBuffer() failed.");
	}

	void SolarSystemRender::ToggleAnimation()
	{
		mAnimationEnabled = !mAnimationEnabled;
	}

	void SolarSystemRender::ReturnToStart()
	{
		mCamera->SetPosition(0.0f, 2.5f, 500.0f);
	}

	void SolarSystemRender::JumpToNextPlanet()
	{
		XMFLOAT3 translateTo;
		if (++mCurrentPlanet >= mCelestialBodiesList.size())
		{
			mCurrentPlanet = 1;
		}
		MatrixHelper::GetTranslation(mCelestialBodiesList[mCurrentPlanet]->WorldMatrix(), translateTo);
		mCamera->SetPosition(translateTo.x, translateTo.y, translateTo.z);
	}
}