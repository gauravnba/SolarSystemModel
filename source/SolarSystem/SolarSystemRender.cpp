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
		DrawableGameComponent(game, camera), mPointLight(game, XMFLOAT3(0.0f, 0.0f, 0.0f), 25000.0f),
		mRenderStateHelper(game), mIndexCount(0), mTextPosition(0.0f, 40.0f), mAnimationEnabled(true)
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

		// Load a compiled pixel shader
		vector<char> compiledPixelShader;
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

		// Load a proxy model for the point light
		mProxyModel = make_unique<ProxyModel>(*mGame, mCamera, "Content\\Models\\Sphere.obj.bin", 0.5f);
		mProxyModel->Initialize();
		mProxyModel->SetPosition(mPointLight.Position());

		const float earthRotation = XM_PI * 5;
		const float earthAxialTilt = 0.4101524f;
		const float earthScale = 1.0f;
		const float earthOrbitalDistance = 200.0f;
		const float earthRevolution = earthRotation / 365;

		mPlanetList.push_back(Planet(mGame, earthRotation * 0.017f, L"Content\\Textures\\mercurymap.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.387f, earthScale * 0.382f, earthRevolution * 4.149f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation * 0.004f, L"Content\\Textures\\venusmap.jpg", earthAxialTilt * 0.959f, earthOrbitalDistance * 0.723f, earthScale * 0.949f, earthRevolution * 1.624f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation, L"Content\\Textures\\EarthComposite.jpg", earthAxialTilt, earthOrbitalDistance, earthScale, earthRevolution, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation, L"Content\\Textures\\moonmap2k.jpg", earthAxialTilt * 0, earthOrbitalDistance * 0.002f, earthScale / 27, earthRevolution / 12, &mPlanetList[2]));
		mPlanetList.push_back(Planet(mGame, earthRotation, L"Content\\Textures\\marsmap1k.jpg", 0.4392f, earthOrbitalDistance * 1.524f, earthScale * 0.532f, earthRevolution * 0.531f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation * 2.4f, L"Content\\Textures\\jupiter2_2k.jpg", 0.05352f, earthOrbitalDistance * 5.203f, earthScale * 11.19f, earthRevolution * 0.084f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation * 2.3f, L"Content\\Textures\\saturnmap.jpg", 0.4712f, earthOrbitalDistance * 9.582f, earthScale * 9.26f, earthRevolution * 0.034f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation * 1.39f, L"Content\\Textures\\uranusmap.jpg", 1.6927f, earthOrbitalDistance * 19.20f, earthScale * 4.01f, earthRevolution * 0.011f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation * 1.489f, L"Content\\Textures\\neptunemap.jpg", 0.5166f, earthOrbitalDistance * 30.5f, earthScale * 3.88f, earthRevolution * 0.0061f, nullptr));
		mPlanetList.push_back(Planet(mGame, earthRotation * 0.156f, L"Content\\Textures\\plutomap2k.jpg", 2.129f, earthOrbitalDistance * 39.48f, earthScale * 0.18f, earthRevolution * 0.004f, nullptr));
	}

	void SolarSystemRender::Update(const GameTime& gameTime)
	{
		if (mAnimationEnabled)
		{
			for (uint32_t i = 0; i < mPlanetList.size(); ++i)
			{
				mPlanetList[i].Update(gameTime);
			}
		}

		if (mKeyboard != nullptr)
		{
			if (mKeyboard->WasKeyPressedThisFrame(Keys::Space))
			{
				ToggleAnimation();
			}

			UpdateAmbientLight(gameTime);
			UpdatePointLight(gameTime);
		}

		mProxyModel->Update(gameTime);
	}

	void SolarSystemRender::Draw(const GameTime& gameTime)
	{
		UNREFERENCED_PARAMETER(gameTime);
		assert(mCamera != nullptr);

		ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		UINT stride = sizeof(VertexPositionTextureNormal);
		UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
		direct3DDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		for (uint32_t i = 0; i < mPlanetList.size(); ++i)
		{
			XMMATRIX worldMatrix = mPlanetList[i].WorldMatrix();
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

			ID3D11ShaderResourceView* PSShaderResources[] = { mPlanetList[i].ColorTexture().Get() };
			direct3DDeviceContext->PSSetShaderResources(0, ARRAYSIZE(PSShaderResources), PSShaderResources);
			direct3DDeviceContext->PSSetSamplers(0, 1, SamplerStates::TrilinearWrap.GetAddressOf());

			direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
		}

		mProxyModel->Draw(gameTime);
		
		// Draw help text
		mRenderStateHelper.SaveAll();
		mSpriteBatch->Begin();

		wostringstream helpLabel;
		helpLabel << "Ambient Intensity (+PgUp/-PgDn): " << mPSCBufferPerFrameData.AmbientColor.x << "\n";
		//helpLabel << L"Specular Intensity (+Insert/-Delete): " << mPSCBufferPerObjectData.SpecularColor.x << "\n";
		//helpLabel << L"Specular Power (+O/-P): " << mPSCBufferPerObjectData.SpecularPower << "\n";
		helpLabel << L"Point Light Intensity (+Home/-End): " << mPSCBufferPerFrameData.LightColor.x << "\n";
		helpLabel << L"Point Light Radius (+V/-B): " << mVSCBufferPerFrameData.LightRadius << "\n";
		helpLabel << L"Move Point Light (8/2, 4/6, 3/9)" << "\n";
		helpLabel << L"Toggle Grid (G)" << "\n";
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

	void SolarSystemRender::UpdateAmbientLight(const GameTime& gameTime)
	{
		static float ambientIntensity = mPSCBufferPerFrameData.AmbientColor.x;

		assert(mKeyboard != nullptr);

		if (mKeyboard->IsKeyDown(Keys::PageUp) && ambientIntensity < 1.0f)
		{
			ambientIntensity += gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = min(ambientIntensity, 1.0f);

			mPSCBufferPerFrameData.AmbientColor = XMFLOAT3(ambientIntensity, ambientIntensity, ambientIntensity);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
		else if (mKeyboard->IsKeyDown(Keys::PageDown) && ambientIntensity > 0.0f)
		{
			ambientIntensity -= gameTime.ElapsedGameTimeSeconds().count();
			ambientIntensity = max(ambientIntensity, 0.0f);

			mPSCBufferPerFrameData.AmbientColor = XMFLOAT3(ambientIntensity, ambientIntensity, ambientIntensity);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
	}

	void SolarSystemRender::UpdatePointLight(const GameTime& gameTime)
	{
		static float lightIntensity = mPSCBufferPerFrameData.LightColor.x;

		assert(mKeyboard != nullptr);

		float elapsedTime = gameTime.ElapsedGameTimeSeconds().count();
		bool updateCBuffer = false;

		// Update point light intensity
		if (mKeyboard->IsKeyDown(Keys::Home) && lightIntensity < 1.0f)
		{
			lightIntensity += elapsedTime;
			lightIntensity = min(lightIntensity, 1.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT3(lightIntensity, lightIntensity, lightIntensity);
			mPointLight.SetColor(mPSCBufferPerFrameData.LightColor.x, mPSCBufferPerFrameData.LightColor.y, mPSCBufferPerFrameData.LightColor.z, 1.0f);
			updateCBuffer = true;
		}
		else if (mKeyboard->IsKeyDown(Keys::End) && lightIntensity > 0.0f)
		{
			lightIntensity -= elapsedTime;
			lightIntensity = max(lightIntensity, 0.0f);

			mPSCBufferPerFrameData.LightColor = XMFLOAT3(lightIntensity, lightIntensity, lightIntensity);
			mPointLight.SetColor(mPSCBufferPerFrameData.LightColor.x, mPSCBufferPerFrameData.LightColor.y, mPSCBufferPerFrameData.LightColor.z, 1.0f);
			updateCBuffer = true;
		}

		// Move point light
		XMFLOAT3 movementAmount = Vector3Helper::Zero;
		if (mKeyboard != nullptr)
		{
			if (mKeyboard->IsKeyDown(Keys::NumPad4))
			{
				movementAmount.x = -1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad6))
			{
				movementAmount.x = 1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad9))
			{
				movementAmount.y = 1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad3))
			{
				movementAmount.y = -1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad8))
			{
				movementAmount.z = -1.0f;
			}

			if (mKeyboard->IsKeyDown(Keys::NumPad2))
			{
				movementAmount.z = 1.0f;
			}
		}

		if (movementAmount.x != 0.0f || movementAmount.y != 0.0f || movementAmount.z != 0.0f)
		{
			XMVECTOR movement = XMLoadFloat3(&movementAmount) * LightMovementRate * elapsedTime;
			mPointLight.SetPosition(mPointLight.PositionVector() + movement);
			mProxyModel->SetPosition(mPointLight.Position());
			mVSCBufferPerFrameData.LightPosition = mPointLight.Position();
			mPSCBufferPerFrameData.LightPosition = mPointLight.Position();
			updateCBuffer = true;
		}

		// Update the light's radius
		if (mKeyboard->IsKeyDown(Keys::V))
		{
			float radius = mPointLight.Radius() + LightModulationRate * elapsedTime;
			mPointLight.SetRadius(radius);
			mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
			updateCBuffer = true;
		}

		if (mKeyboard->IsKeyDown(Keys::B))
		{
			float radius = mPointLight.Radius() - LightModulationRate * elapsedTime;
			radius = max(radius, 0.0f);
			mPointLight.SetRadius(radius);
			mVSCBufferPerFrameData.LightRadius = mPointLight.Radius();
			updateCBuffer = true;
		}

		if (updateCBuffer)
		{
			mGame->Direct3DDeviceContext()->UpdateSubresource(mVSCBufferPerFrame.Get(), 0, nullptr, &mVSCBufferPerFrameData, 0, 0);
			mGame->Direct3DDeviceContext()->UpdateSubresource(mPSCBufferPerFrame.Get(), 0, nullptr, &mPSCBufferPerFrameData, 0, 0);
		}
	}
}