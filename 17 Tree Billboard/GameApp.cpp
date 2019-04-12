#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::Free),
	m_EnableAlphaToCoverage(true),
	m_FogEnabled(true),
	m_FogRange(75.0f),
	m_IsNight(false),
	m_TreeMat()
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// ����ȳ�ʼ��������Ⱦ״̬���Թ��������Чʹ��
	RenderStates::InitAll(m_pd3dDevice);

	if (!m_BasicEffect.InitAll(m_pd3dDevice))
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

	if (m_pCamera != nullptr)
	{
		m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
		m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
		m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
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
	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

	if (m_CameraMode == CameraMode::Free)
	{
		// ******************
		// ����������Ĳ���
		//

		// �����ƶ�
		if (keyState.IsKeyDown(Keyboard::W))
		{
			cam1st->MoveForward(dt * 3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::S))
		{
			cam1st->MoveForward(dt * -3.0f);
		}
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// ��Ұ��ת����ֹ��ʼ�Ĳ�ֵ�����µ�ͻȻ��ת
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);
	}

	// ******************
	// ���������
	//

	// ��λ��������[-49.9f, 49.9f]��������
	// ��������
	XMFLOAT3 adjustedPos;
	XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), 
		XMVectorSet(-49.9f, 0.0f, -49.9f, 0.0f), XMVectorSet(49.9f, 99.9f, 49.9f, 0.0f)));
	cam1st->SetPosition(adjustedPos);

	// ���¹۲����
	m_pCamera->UpdateViewMatrix();
	m_BasicEffect.SetEyePos(m_pCamera->GetPositionXM());
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());

	// ******************
	// ������Ч
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_FogEnabled = !m_FogEnabled;
		m_BasicEffect.SetFogState(m_FogEnabled);
	}

	// ******************
	// ����/��ҹ�任
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_IsNight = !m_IsNight;
		if (m_IsNight)
		{
			// ��ҹģʽ�±�Ϊ�𽥺ڰ�
			m_BasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
			m_BasicEffect.SetFogStart(5.0f);
		}
		else
		{
			// ����ģʽ���Ӧ��Ч
			m_BasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
			m_BasicEffect.SetFogStart(15.0f);
		}
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		m_EnableAlphaToCoverage = !m_EnableAlphaToCoverage;
	}

	// ******************
	// ������ķ�Χ
	//
	if (mouseState.scrollWheelValue != 0)
	{
		// һ�ι��ֹ�������С��λΪ120
		m_FogRange += mouseState.scrollWheelValue / 120;
		if (m_FogRange < 15.0f)
			m_FogRange = 15.0f;
		else if (m_FogRange > 175.0f)
			m_FogRange = 175.0f;
		m_BasicEffect.SetFogRange(m_FogRange);
	}
	

	// ���ù���ֵ
	m_pMouse->ResetScrollWheelValue();

	

	// �˳���������Ӧ�򴰿ڷ���������Ϣ
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
		SendMessage(MainWnd(), WM_DESTROY, 0, 0);

}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	// ******************
	// ����Direct3D����
	//
	
	// ���ñ���ɫ
	if (m_IsNight)
	{
		m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	}
	else
	{
		m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Silver));
	}
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ���Ƶ���
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext);
	m_Ground.Draw(m_pd3dImmediateContext, m_BasicEffect);

	// ������
	m_BasicEffect.SetRenderBillboard(m_pd3dImmediateContext, m_EnableAlphaToCoverage);
	m_BasicEffect.SetMaterial(m_TreeMat);
	UINT stride = sizeof(VertexPosSize);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, mPointSpritesBuffer.GetAddressOf(), &stride, &offset);
	m_BasicEffect.Apply(m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(16, 0);

	// ******************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"1-��Ч���� 2-����/��ҹ��Ч�л� 3-AlphaToCoverage���� Esc-�˳�\n"
			"����-������Ч��Χ\n"
			"��֧�������ӽ������\n";
		text += std::wstring(L"AlphaToCoverage״̬: ") + (m_EnableAlphaToCoverage ? L"����\n" : L"�ر�\n");
		text += std::wstring(L"��Ч״̬: ") + (m_FogEnabled ? L"����\n" : L"�ر�\n");
		if (m_FogEnabled)
		{
			text += std::wstring(L"�������: ") + (m_IsNight ? L"��ҹ\n" : L"����\n");
			text += L"��Ч��Χ: " + std::to_wstring(m_IsNight ? 5 : 15) + L"-" +
				std::to_wstring((m_IsNight ? 5 : 15) + (int)m_FogRange);
		}


		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));

}



bool GameApp::InitResource()
{
	// ******************
	// ��ʼ����������
	//

	// ��ʼ����������Դ
	ComPtr<ID3D11Texture2D> test;
	HR(CreateDDSTexture2DArrayFromFile(
		m_pd3dDevice.Get(),
		m_pd3dImmediateContext.Get(),
		std::vector<std::wstring>{
			L"Texture\\tree0.dds",
			L"Texture\\tree1.dds",
			L"Texture\\tree2.dds",
			L"Texture\\tree3.dds"},
		test.GetAddressOf(),
		mTreeTexArray.GetAddressOf()));
	m_BasicEffect.SetTextureArray(mTreeTexArray);

	// ��ʼ���㾫�黺����
	InitPointSpritesBuffer();

	// ��ʼ�����Ĳ���
	m_TreeMat.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_TreeMat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_TreeMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	ComPtr<ID3D11ShaderResourceView> texture;
	// ��ʼ���ذ�
	m_Ground.SetBuffer(m_pd3dDevice, Geometry::CreatePlane(XMFLOAT3(0.0f, -5.0f, 0.0f), XMFLOAT2(100.0f, 100.0f), XMFLOAT2(10.0f, 10.0f)));
	HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\Grass.dds", nullptr, texture.GetAddressOf()));
	m_Ground.SetTexture(texture);
	Material material;
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_Ground.SetMaterial(material);

	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight[4];
	dirLight[0].ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	dirLight[0].specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	dirLight[0].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	dirLight[1] = dirLight[0];
	dirLight[1].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
	dirLight[2] = dirLight[0];
	dirLight[2].direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
	dirLight[3] = dirLight[0];
	dirLight[3].direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
	for (int i = 0; i < 4; ++i)
		m_BasicEffect.SetDirLight(i, dirLight[i]);

	// ******************
	// ��ʼ�������
	//
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetPosition(XMFLOAT3());
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();

	m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());
	m_BasicEffect.SetEyePos(camera->GetPositionXM());

	// ******************
	// ��ʼ����Ч��������
	//

	m_BasicEffect.SetFogState(m_FogEnabled);
	m_BasicEffect.SetFogColor(XMVectorSet(0.75f, 0.75f, 0.75f, 1.0f));
	m_BasicEffect.SetFogStart(15.0f);
	m_BasicEffect.SetFogRange(75.0f);

	
	
	return true;
}

void GameApp::InitPointSpritesBuffer()
{
	srand((unsigned)time(nullptr));
	VertexPosSize vertexes[16];
	float theta = 0.0f;
	for (int i = 0; i < 16; ++i)
	{
		// ȡ20-50�İ뾶�����������
		float radius = (float)(rand() % 31 + 20);
		float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
		vertexes[i].pos = XMFLOAT3(radius * cosf(theta + randomRad), 8.0f, radius * sinf(theta + randomRad));
		vertexes[i].size = XMFLOAT2(30.0f, 30.0f);
		theta += XM_2PI / 16;
	}

	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;	// ���ݲ����޸�
	vbd.ByteWidth = sizeof (vertexes);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertexes;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, mPointSpritesBuffer.GetAddressOf()));
}
