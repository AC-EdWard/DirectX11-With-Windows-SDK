#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_ShowMode(Mode::SplitedTriangle),
	m_CurrIndex(),
	m_IsWireFrame(false),
	m_ShowNormal(false),
	m_InitVertexCounts()
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
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	// ��ʼ����꣬���̲���Ҫ
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

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

	// ����ͶӰ����
	m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

}

void GameApp::UpdateScene(float dt)
{

	// ��������¼�����ȡ���ƫ����
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	UINT stride = (m_ShowMode != Mode::SplitedSphere ? sizeof(VertexPosColor) : sizeof(VertexPosNormalColor));
	UINT offset = 0;


	// ******************
	// �л�����
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
	{
		m_ShowMode = Mode::SplitedTriangle;
		ResetSplitedTriangle();
		m_IsWireFrame = false;
		m_ShowNormal = false;
		m_CurrIndex = 0;
		stride = sizeof(VertexPosColor);
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::W))
	{
		m_ShowMode = Mode::SplitedSnow;
		ResetSplitedSnow();
		m_IsWireFrame = true;
		m_ShowNormal = false;
		m_CurrIndex = 0;
		stride = sizeof(VertexPosColor);
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::E))
	{
		m_ShowMode = Mode::SplitedSphere;
		ResetSplitedSphere();
		m_IsWireFrame = false;
		m_ShowNormal = false;
		m_CurrIndex = 0;
		stride = sizeof(VertexPosNormalColor);
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[0].GetAddressOf(), &stride, &offset);
	}

	// ******************
	// �л�����
	//
	for (int i = 0; i < 7; ++i)
	{
		if (m_KeyboardTracker.IsKeyPressed((Keyboard::Keys)((int)Keyboard::D1 + i)))
		{
			m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[i].GetAddressOf(), &stride, &offset);
			m_CurrIndex = i;
		}
	}

	// ******************
	// �л��߿�/��
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::M))
	{
		if (m_ShowMode != Mode::SplitedSnow)
		{
			m_IsWireFrame = !m_IsWireFrame;
		}
	}

	// ******************
	// �Ƿ���ӷ�����
	//
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::N))
	{
		if (m_ShowMode == Mode::SplitedSphere)
		{
			m_ShowNormal = !m_ShowNormal;
		}
	}

	// ******************
	// ����ÿ֡�仯��ֵ
	//
	if (m_ShowMode == Mode::SplitedSphere)
	{
		// ������ת����
		static float theta = 0.0f;
		theta += 0.3f * dt;
		m_BasicEffect.SetWorldMatrix(XMMatrixRotationY(theta));
	}
	else
	{
		m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	}

}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);


	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	// ���ݵ�ǰ����ģʽ������Ҫ������Ⱦ�ĸ�����Դ
	if (m_ShowMode == Mode::SplitedTriangle)
	{
		m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext.Get());
	}
	else if (m_ShowMode == Mode::SplitedSnow)
	{
		m_BasicEffect.SetRenderSplitedSnow(m_pd3dImmediateContext.Get());
	}
	else if (m_ShowMode == Mode::SplitedSphere)
	{
		m_BasicEffect.SetRenderSplitedSphere(m_pd3dImmediateContext.Get());
	}

	// �����߿�/��ģʽ
	if (m_IsWireFrame)
	{
		m_pd3dImmediateContext->RSSetState(RenderStates::RSWireframe.Get());
	}
	else
	{
		m_pd3dImmediateContext->RSSetState(nullptr);
	}

	// Ӧ�ó����������ı��
	m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
	// ��������Ϊ0�Ļ�����ȱ���ڲ�ͼԪ��Ŀ��¼�����඼����ʹ��DrawAuto����
	if (m_CurrIndex <= 0)
	{
		m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
	}
	else
	{
		m_pd3dImmediateContext->DrawAuto();
	}
		
	// ���Ʒ�����
	if (m_ShowNormal)
	{
		m_BasicEffect.SetRenderNormal(m_pd3dImmediateContext.Get());
		m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
		// ��������Ϊ0�Ļ�����ȱ���ڲ�ͼԪ��Ŀ��¼�����඼����ʹ��DrawAuto����
		if (m_CurrIndex <= 0)
		{
			m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
		}
		else
		{
			m_pd3dImmediateContext->DrawAuto();
		}
	}


	// ******************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"�л����Σ�Q-������(��/�߿�) W-ѩ��(�߿�) E-��(��/�߿�)\n"
			L"����������1 - 7�����ν�����Խ��Խ��ϸ\n"
			L"M-��/�߿��л�\n\n"
			L"��ǰ����: " + std::to_wstring(m_CurrIndex + 1) + L"\n"
			"��ǰ����: ";
		if (m_ShowMode == Mode::SplitedTriangle)
			text += L"������";
		else if (m_ShowMode == Mode::SplitedSnow)
			text += L"ѩ��";
		else
			text += L"��";

		if (m_IsWireFrame)
			text += L"(�߿�)";
		else
			text += L"(��)";

		if (m_ShowMode == Mode::SplitedSphere)
		{
			if (m_ShowNormal)
				text += L"(N-�رշ�����)";
			else
				text += L"(N-����������)";
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
	// ��ʼ������
	//

	// Ĭ�ϻ���������
	ResetSplitedTriangle();
	// Ԥ�Ȱ󶨶��㻺����
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[0].GetAddressOf(), &stride, &offset);

	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight{};
	dirLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	m_BasicEffect.SetDirLight(0, dirLight);
	// ����
	Material material{};
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	m_BasicEffect.SetMaterial(material);
	// �����λ��
	m_BasicEffect.SetEyePos(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f));
	// ����
	m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	m_BasicEffect.SetViewMatrix(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f),
		XMVectorZero(),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
	m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));

	m_BasicEffect.SetSphereCenter(XMFLOAT3());
	m_BasicEffect.SetSphereRadius(2.0f);

	return true;
}


void GameApp::ResetSplitedTriangle()
{
	// ******************
	// ��ʼ��������
	//

	// ���������ζ���
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.0f * 3, 0.866f * 3, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f * 3, -0.866f * 3, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};
	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;	// ������Ҫ����������׶�ͨ��GPUд��
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// ��Ҫ��������������ǩ
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

	//#if defined(DEBUG) | defined(_DEBUG)
	//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
	//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
	//	graphicsAnalysis->BeginCapture();
	//#endif

	// �����ζ�����
	m_InitVertexCounts = 3;
	// ��ʼ�����ж��㻺����
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 3;
		HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
		m_BasicEffect.SetStreamOutputSplitedTriangle(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
		// ��һ�λ�����Ҫ����һ�����ָ�֮��Ϳ���ʹ��DrawAuto��
		if (i == 1)
		{
			m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
		}
		else
		{
			m_pd3dImmediateContext->DrawAuto();
		}

	}

	//#if defined(DEBUG) | defined(_DEBUG)
	//	graphicsAnalysis->EndCapture();
	//#endif

	D3D11SetDebugObjectName(m_pVertexBuffers[0].Get(), "TriangleVertexBuffer[0]");
	D3D11SetDebugObjectName(m_pVertexBuffers[1].Get(), "TriangleVertexBuffer[1]");
	D3D11SetDebugObjectName(m_pVertexBuffers[2].Get(), "TriangleVertexBuffer[2]");
	D3D11SetDebugObjectName(m_pVertexBuffers[3].Get(), "TriangleVertexBuffer[3]");
	D3D11SetDebugObjectName(m_pVertexBuffers[4].Get(), "TriangleVertexBuffer[4]");
	D3D11SetDebugObjectName(m_pVertexBuffers[5].Get(), "TriangleVertexBuffer[5]");
	D3D11SetDebugObjectName(m_pVertexBuffers[6].Get(), "TriangleVertexBuffer[6]");
}

void GameApp::ResetSplitedSnow()
{
	// ******************
	// ѩ�����δӳ�ʼ�������ο�ʼ����Ҫ6������
	//

	// ���������ζ���
	float sqrt3 = sqrt(3.0f);
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, sqrt3 / 2, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-3.0f / 4, -sqrt3 / 4, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
	};
	// �������ο�Ⱥ͸߶ȶ��Ŵ�3��
	for (VertexPosColor& v : vertices)
	{
		v.pos.x *= 3;
		v.pos.y *= 3;
	}

	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;	// ������Ҫ����������׶�ͨ��GPUд��
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// ��Ҫ��������������ǩ
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

	//#if defined(DEBUG) | defined(_DEBUG)
	//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
	//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
	//	graphicsAnalysis->BeginCapture();
	//#endif

	// ������
	m_InitVertexCounts = 6;
	// ��ʼ�����ж��㻺����
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 4;
		HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
		m_BasicEffect.SetStreamOutputSplitedSnow(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
		// ��һ�λ�����Ҫ����һ�����ָ�֮��Ϳ���ʹ��DrawAuto��
		if (i == 1)
		{
			m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
		}
		else
		{
			m_pd3dImmediateContext->DrawAuto();
		}
	}

	//#if defined(DEBUG) | defined(_DEBUG)
	//	graphicsAnalysis->EndCapture();
	//#endif

	D3D11SetDebugObjectName(m_pVertexBuffers[0].Get(), "SnowVertexBuffer[0]");
	D3D11SetDebugObjectName(m_pVertexBuffers[1].Get(), "SnowVertexBuffer[1]");
	D3D11SetDebugObjectName(m_pVertexBuffers[2].Get(), "SnowVertexBuffer[2]");
	D3D11SetDebugObjectName(m_pVertexBuffers[3].Get(), "SnowVertexBuffer[3]");
	D3D11SetDebugObjectName(m_pVertexBuffers[4].Get(), "SnowVertexBuffer[4]");
	D3D11SetDebugObjectName(m_pVertexBuffers[5].Get(), "SnowVertexBuffer[5]");
	D3D11SetDebugObjectName(m_pVertexBuffers[6].Get(), "SnowVertexBuffer[6]");
}

void GameApp::ResetSplitedSphere()
{
	// ******************
	// ��������
	//

	VertexPosNormalColor basePoint[] = {
		{ XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(-2.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ XMFLOAT3(0.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
	};
	int indices[] = { 0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1, 5 };

	std::vector<VertexPosNormalColor> vertices;
	for (int pos : indices)
	{
		vertices.push_back(basePoint[pos]);
	}


	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;	// ������Ҫ����������׶�ͨ��GPUд��
	vbd.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPosNormalColor));
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;	// ��Ҫ��������������ǩ
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

	//#if defined(DEBUG) | defined(_DEBUG)
	//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
	//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
	//	graphicsAnalysis->BeginCapture();
	//#endif

	// ������
	m_InitVertexCounts = 24;
	// ��ʼ�����ж��㻺����
	for (int i = 1; i < 7; ++i)
	{
		vbd.ByteWidth *= 4;
		HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
		m_BasicEffect.SetStreamOutputSplitedSphere(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
		// ��һ�λ�����Ҫ����һ�����ָ�֮��Ϳ���ʹ��DrawAuto��
		if (i == 1)
		{
			m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
		}
		else
		{
			m_pd3dImmediateContext->DrawAuto();
		}
	}

	//#if defined(DEBUG) | defined(_DEBUG)
	//	graphicsAnalysis->EndCapture();
	//#endif

	D3D11SetDebugObjectName(m_pVertexBuffers[0].Get(), "SphereVertexBuffer[0]");
	D3D11SetDebugObjectName(m_pVertexBuffers[1].Get(), "SphereVertexBuffer[1]");
	D3D11SetDebugObjectName(m_pVertexBuffers[2].Get(), "SphereVertexBuffer[2]");
	D3D11SetDebugObjectName(m_pVertexBuffers[3].Get(), "SphereVertexBuffer[3]");
	D3D11SetDebugObjectName(m_pVertexBuffers[4].Get(), "SphereVertexBuffer[4]");
	D3D11SetDebugObjectName(m_pVertexBuffers[5].Get(), "SphereVertexBuffer[5]");
	D3D11SetDebugObjectName(m_pVertexBuffers[6].Get(), "SphereVertexBuffer[6]");
}



