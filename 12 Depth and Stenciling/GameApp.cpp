#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	if (!InitEffect())
		return false;

	if (!InitResource())
		return false;

	// ��ʼ����꣬���̲���Ҫ
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_RELATIVE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);
	// �ͷ�D2D�������Դ
	m_pColorBrush.Reset();
	m_pd2dRenderTarget.Reset();

	D3DApp::OnResize();

	// ΪD2D����DXGI������ȾĿ��
	ComPtr<IDXGISurface> surface;
	HR(m_pSwapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(surface.GetAddressOf())));
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	HRESULT hr = m_pd2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, m_pd2dRenderTarget.GetAddressOf());
	surface.Reset();

	if (hr == E_NOINTERFACE)
	{
		OutputDebugString(L"\n���棺Direct2D��Direct3D�������Թ������ޣ��㽫�޷������ı���Ϣ�����ṩ������ѡ������\n"
			"1. ����Win7ϵͳ����Ҫ������Win7 SP1������װKB2670838������֧��Direct2D��ʾ��\n"
			"2. �������Direct3D 10.1��Direct2D�Ľ�����������ģ�"
			"https://docs.microsoft.com/zh-cn/windows/desktop/Direct2D/direct2d-and-direct3d-interoperation-overview""\n"
			"3. ʹ�ñ������⣬����FreeType��\n\n");
	}
	else if (hr == S_OK)
	{
		// �����̶���ɫˢ���ı���ʽ
		HR(m_pd2dRenderTarget->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White),
			m_pColorBrush.GetAddressOf()));
		HR(m_pdwriteFactory->CreateTextFormat(L"����", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15, L"zh-cn",
			m_pTextFormat.GetAddressOf()));
	}
	else
	{
		// �����쳣����
		assert(m_pd2dRenderTarget);
	}
	
	// ����������ʾ
	if (m_pConstantBuffers[3] != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[3].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
		memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
		m_pd3dImmediateContext->Unmap(m_pConstantBuffers[3].Get(), 0);
	}
}

void GameApp::UpdateScene(float dt)
{

	// ��������¼�����ȡ���ƫ����
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	// ��ȡ����
	auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);
	
	if (m_CameraMode == CameraMode::Free)
	{
		// ********************
		// ��һ�˳�/����������Ĳ���
		//

		// �����ƶ�
		if (keyState.IsKeyDown(Keyboard::W))
			cam1st->MoveForward(dt * 3.0f);
		if (keyState.IsKeyDown(Keyboard::S))
			cam1st->MoveForward(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// ��Ұ��ת����ֹ��ʼ�Ĳ�ֵ�����µ�ͻȻ��ת
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}
	else if (m_CameraMode == CameraMode::ThirdPerson)
	{
		// ********************
		// �����˳�������Ĳ���
		//

		cam3rd->SetTarget(m_WireFence.GetPosition());

		// ��������ת
		cam3rd->RotateX(mouseState.y * dt * 1.25f);
		cam3rd->RotateY(mouseState.x * dt * 1.25f);
		cam3rd->Approach(-mouseState.scrollWheelValue / 120 * 1.0f);
	}

	// ���¹۲����
	m_pCamera->UpdateViewMatrix();
	XMStoreFloat4(&m_CBFrame.eyePos, m_pCamera->GetPositionXM());
	m_CBFrame.view = XMMatrixTranspose(m_pCamera->GetViewXM());

	// ���ù���ֵ
	m_pMouse->ResetScrollWheelValue();

	// ********************
	// �����ģʽ�л�
	//
	
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1) && m_CameraMode != CameraMode::ThirdPerson)
	{
		if (!cam3rd)
		{
			cam3rd.reset(new ThirdPersonCamera);
			cam3rd->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam3rd;
		}
		XMFLOAT3 target = m_WireFence.GetPosition();
		cam3rd->SetTarget(target);
		cam3rd->SetDistance(8.0f);
		cam3rd->SetDistanceMinMax(3.0f, 20.0f);
		// ��ʼ��ʱ������󷽿�
		// cam3rd->RotateY(-XM_PIDIV2);

		m_CameraMode = CameraMode::ThirdPerson;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2) && m_CameraMode != CameraMode::Free)
	{
		if (!cam1st)
		{
			cam1st.reset(new FirstPersonCamera);
			cam1st->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
			m_pCamera = cam1st;
		}
		// �������Ϸ���ʼ
		XMFLOAT3 pos = m_WireFence.GetPosition();
		XMFLOAT3 look{ 0.0f, 0.0f, 1.0f };
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		pos.y += 3;
		cam1st->LookTo(pos, look, up);

		m_CameraMode = CameraMode::Free;
	}
	
	// �˳���������Ӧ�򴰿ڷ���������Ϣ
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);
	
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[2].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesEveryFrame), &m_CBFrame, sizeof(CBChangesEveryFrame));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[2].Get(), 0);
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	
	
	// *********************
	// 1. �����淴������д��ֵ1��ģ�建����
	// 

	// �ü�������������
	// ��Ǿ��������ģ��ֵΪ1
	// ��д��������ɫ
	m_pd3dImmediateContext->RSSetState(nullptr);
	m_pd3dImmediateContext->OMSetDepthStencilState(RenderStates::DSSMarkMirror.Get(), 1);
	m_pd3dImmediateContext->OMSetBlendState(RenderStates::BSNoColorWrite.Get(), nullptr, 0xFFFFFFFF);


	m_Mirror.Draw(m_pd3dImmediateContext);

	// ***********************
	// 2. ���Ʋ�͸���ķ�������
	//

	// �����������
	m_CBStates.isReflection = true;
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBDrawingStates), &m_CBStates, sizeof(CBDrawingStates));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);
	
	// ���Ʋ�͸�����壬��Ҫ˳ʱ��ü�
	// ����ģ��ֵΪ1�ľ����������
	m_pd3dImmediateContext->RSSetState(RenderStates::RSCullClockWise.Get());
	m_pd3dImmediateContext->OMSetDepthStencilState(RenderStates::DSSDrawReflection.Get(), 1);
	m_pd3dImmediateContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	
	m_Walls[2].Draw(m_pd3dImmediateContext);
	m_Walls[3].Draw(m_pd3dImmediateContext);
	m_Walls[4].Draw(m_pd3dImmediateContext);
	m_Floor.Draw(m_pd3dImmediateContext);

	// ***********************
	// 3. ����͸���ķ�������
	//

	// �ر�˳��ʱ��ü�
	// ����ģ��ֵΪ1�ľ����������
	// ͸�����
	m_pd3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
	m_pd3dImmediateContext->OMSetDepthStencilState(RenderStates::DSSDrawReflection.Get(), 1);
	m_pd3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);

	m_WireFence.Draw(m_pd3dImmediateContext);
	m_Water.Draw(m_pd3dImmediateContext);
	m_Mirror.Draw(m_pd3dImmediateContext);
	
	// �رշ������
	m_CBStates.isReflection = false;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBDrawingStates), &m_CBStates, sizeof(CBDrawingStates));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);


	// ************************
	// 4. ���Ʋ�͸������������
	//

	m_pd3dImmediateContext->RSSetState(nullptr);
	m_pd3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	m_pd3dImmediateContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	for (auto& wall : m_Walls)
		wall.Draw(m_pd3dImmediateContext);
	m_Floor.Draw(m_pd3dImmediateContext);

	// ***********************
	// 5. ����͸������������
	//

	// �ر�˳��ʱ��ü�
	// ͸�����
	m_pd3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
	m_pd3dImmediateContext->OMSetDepthStencilState(nullptr, 0);
	m_pd3dImmediateContext->OMSetBlendState(RenderStates::BSTransparent.Get(), nullptr, 0xFFFFFFFF);

	m_WireFence.Draw(m_pd3dImmediateContext);
	m_Water.Draw(m_pd3dImmediateContext);

	// ********************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"�л������ģʽ: 1-�����˳� 2-�����ӽ�\n"
			"W/S/A/D ǰ��/����/��ƽ��/��ƽ�� (�����˳���Ч)  Esc�˳�\n"
			"����ƶ�������Ұ ���ֿ��Ƶ����˳ƹ۲����\n"
			"��ǰģʽ: ";
		if (m_CameraMode == CameraMode::ThirdPerson)
			text += L"�����˳�";
		else
			text += L"�����ӽ�";
		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;

	// ����������ɫ��(2D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_2D.cso", L"HLSL\\Basic_VS_2D.hlsl", "VS_2D", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader2D.GetAddressOf()));
	// �������㲼��(2D)
	HR(m_pd3dDevice->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout2D.GetAddressOf()));

	// ����������ɫ��(2D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_2D.cso", L"HLSL\\Basic_PS_2D.hlsl", "PS_2D", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader2D.GetAddressOf()));

	// ����������ɫ��(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_3D.cso", L"HLSL\\Basic_VS_3D.hlsl", "VS_3D", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader3D.GetAddressOf()));
	// �������㲼��(3D)
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout3D.GetAddressOf()));

	// ����������ɫ��(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_3D.cso", L"HLSL\\Basic_PS_3D.hlsl", "PS_3D", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader3D.GetAddressOf()));

	return true;
}

bool GameApp::InitResource()
{
	
	// ******************
	// ���ó�������������
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// �½�����VS��PS�ĳ���������
	cbd.ByteWidth = sizeof(CBChangesEveryDrawing);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBDrawingStates);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesEveryFrame);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[2].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesOnResize);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[3].GetAddressOf()));
	cbd.ByteWidth = sizeof(CBChangesRarely);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[4].GetAddressOf()));
	// ******************
	// ��ʼ����Ϸ����
	ComPtr<ID3D11ShaderResourceView> texture;
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	// ��ʼ����ʺ�
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\WireFence.dds", nullptr, texture.GetAddressOf()));
	m_WireFence.SetBuffer(m_pd3dDevice, Geometry::CreateBox());
	// ̧��߶ȱ�����Ȼ�������Դ����
	m_WireFence.SetWorldMatrix(XMMatrixTranslation(0.0f, 0.01f, 7.5f));
	m_WireFence.SetTexture(texture);
	m_WireFence.SetMaterial(material);
	
	

	// ��ʼ���ذ�
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\floor.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Floor.SetBuffer(m_pd3dDevice, 
		Geometry::CreatePlane(XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(20.0f, 20.0f), XMFLOAT2(5.0f, 5.0f)));
	m_Floor.SetTexture(texture);
	m_Floor.SetMaterial(material);

	// ��ʼ��ǽ��
	m_Walls.resize(5);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\brick.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	// �������ǽ�����������ɣ�0��1���м�λ�����ڷ��þ���
	//     ____     ____
	//    /| 0 |   | 1 |\
	//   /4|___|___|___|2\
	//  /_/_ _ _ _ _ _ _\_\
	// | /       3       \ |
	// |/_________________\|
	//
	for (int i = 0; i < 5; ++i)
	{
		m_Walls[i].SetMaterial(material);
		m_Walls[i].SetTexture(texture);
	}
	m_Walls[0].SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	m_Walls[1].SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(6.0f, 8.0f), XMFLOAT2(1.5f, 2.0f)));
	m_Walls[2].SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	m_Walls[3].SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	m_Walls[4].SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 8.0f), XMFLOAT2(5.0f, 2.0f)));
	
	m_Walls[0].SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(-7.0f, 3.0f, 10.0f));
	m_Walls[1].SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(7.0f, 3.0f, 10.0f));
	m_Walls[2].SetWorldMatrix(XMMatrixRotationY(-XM_PIDIV2) * XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(10.0f, 3.0f, 0.0f));
	m_Walls[3].SetWorldMatrix(XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0.0f, 3.0f, -10.0f));
	m_Walls[4].SetWorldMatrix(XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(-XM_PIDIV2) * XMMatrixTranslation(-10.0f, 3.0f, 0.0f));
	
		
	// ��ʼ��ˮ
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.Specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\water.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Water.SetBuffer(m_pd3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(20.0f, 20.0f), XMFLOAT2(10.0f, 10.0f)));
	m_Water.SetTexture(texture);
	m_Water.SetMaterial(material);

	// ��ʼ������
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	material.Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\ice.dds", nullptr, texture.ReleaseAndGetAddressOf()));
	m_Mirror.SetBuffer(m_pd3dDevice,
		Geometry::CreatePlane(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(8.0f, 8.0f), XMFLOAT2(1.0f, 1.0f)));
	m_Mirror.SetWorldMatrix(XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(0.0f, 3.0f, 10.0f));
	m_Mirror.SetTexture(texture);
	m_Mirror.SetMaterial(material);

	// ********************
	// ��ʼ��������״̬
	//
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.GetAddressOf()));

	
	// ******************
	// ��ʼ��������������ֵ
	//

	// ��ʼ��ÿ֡���ܻ�仯��ֵ
	m_CameraMode = CameraMode::ThirdPerson;
	auto camera = std::shared_ptr<ThirdPersonCamera>(new ThirdPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetTarget(XMFLOAT3(0.0f, 0.5f, 0.0f));
	camera->SetDistance(5.0f);
	camera->SetDistanceMinMax(2.0f, 14.0f);
	m_CBFrame.view = XMMatrixTranspose(m_pCamera->GetViewXM());
	XMStoreFloat4(&m_CBFrame.eyePos, m_pCamera->GetPositionXM());

	// ��ʼ�����ڴ��ڴ�С�䶯ʱ�޸ĵ�ֵ
	m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 0.5f, 1000.0f);
	m_CBOnResize.proj = XMMatrixTranspose(m_pCamera->GetProjXM());

	// ��ʼ������仯��ֵ
	m_CBRarely.reflection = XMMatrixTranspose(XMMatrixReflect(XMVectorSet(0.0f, 0.0f, -1.0f, 10.0f)));
	// ������
	m_CBRarely.dirLight[0].Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.dirLight[0].Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_CBRarely.dirLight[0].Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.dirLight[0].Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	// �ƹ�
	m_CBRarely.pointLight[0].Position = XMFLOAT3(0.0f, 15.0f, 0.0f);
	m_CBRarely.pointLight[0].Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_CBRarely.pointLight[0].Diffuse = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	m_CBRarely.pointLight[0].Specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_CBRarely.pointLight[0].Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_CBRarely.pointLight[0].Range = 25.0f;
	m_CBRarely.numDirLight = 1;
	m_CBRarely.numPointLight = 1;
	m_CBRarely.numSpotLight = 0;
	


	// ���²����ױ��޸ĵĳ�����������Դ
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[3].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesOnResize), &m_CBOnResize, sizeof(CBChangesOnResize));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[3].Get(), 0);

	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[4].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesRarely), &m_CBRarely, sizeof(CBChangesRarely));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[4].Get(), 0);
	// ��ʼ��������Ⱦ״̬
	RenderStates::InitAll(m_pd3dDevice);
	
	
	// ******************
	// ����Ⱦ���߸����׶ΰ󶨺�������Դ
	//

	// ����ͼԪ���ͣ��趨���벼��
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout3D.Get());
	
	m_pd3dImmediateContext->VSSetShader(m_pVertexShader3D.Get(), nullptr, 0);
	// Ԥ�Ȱ󶨸�������Ļ�����������ÿ֡���µĻ�������Ҫ�󶨵�������������
	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(2, 1, m_pConstantBuffers[2].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(3, 1, m_pConstantBuffers[3].GetAddressOf());
	m_pd3dImmediateContext->VSSetConstantBuffers(4, 1, m_pConstantBuffers[4].GetAddressOf());

	m_pd3dImmediateContext->PSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->PSSetConstantBuffers(2, 1, m_pConstantBuffers[2].GetAddressOf());
	m_pd3dImmediateContext->PSSetConstantBuffers(4, 1, m_pConstantBuffers[4].GetAddressOf());
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader3D.Get(), nullptr, 0);
	m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
	return true;
}

GameApp::GameObject::GameObject()
	: m_WorldMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f)
{
}

DirectX::XMFLOAT3 GameApp::GameObject::GetPosition() const
{
	return XMFLOAT3(m_WorldMatrix(3, 0), m_WorldMatrix(3, 1), m_WorldMatrix(3, 2));
}


template<class VertexType, class IndexType>
void GameApp::GameObject::SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
	// �ͷž���Դ
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	// ���ö��㻺��������
	m_VertexStride = sizeof(VertexType);
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * m_VertexStride;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(device->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));


	// ������������������
	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = m_IndexCount * sizeof(IndexType);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// �½�����������
	InitData.pSysMem = meshData.indexVec.data();
	HR(device->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));



}

void GameApp::GameObject::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	m_pTexture = texture;
}

void GameApp::GameObject::SetMaterial(const Material & material)
{
	m_Material = material;
}

void GameApp::GameObject::SetWorldMatrix(const XMFLOAT4X4 & world)
{
	m_WorldMatrix = world;
}

void XM_CALLCONV GameApp::GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&m_WorldMatrix, world);
}

void GameApp::GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext)
{
	// ���ö���/����������
	UINT strides = m_VertexStride;
	UINT offsets = 0;
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// ��ȡ֮ǰ�Ѿ��󶨵���Ⱦ�����ϵĳ����������������޸�
	ComPtr<ID3D11Buffer> cBuffer = nullptr;
	deviceContext->VSGetConstantBuffers(0, 1, cBuffer.GetAddressOf());
	XMMATRIX W = XMLoadFloat4x4(&m_WorldMatrix);
	CBChangesEveryDrawing cbDrawing;
	cbDrawing.world = XMMatrixTranspose(W);
	cbDrawing.worldInvTranspose = XMMatrixInverse(nullptr, W);
	cbDrawing.material = m_Material;

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(deviceContext->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(CBChangesEveryDrawing), &cbDrawing, sizeof(CBChangesEveryDrawing));
	deviceContext->Unmap(cBuffer.Get(), 0);
	// ��������
	deviceContext->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());
	// ���Կ�ʼ����
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}
