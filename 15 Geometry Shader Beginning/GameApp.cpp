#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_ShowMode(Mode::SplitedTriangle),
	m_VertexCount()
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

	// ����ÿ֡�仯��ֵ
	if (m_ShowMode == Mode::SplitedTriangle)
	{
		m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	}
	else
	{
		static float phi = 0.0f, theta = 0.0f;
		phi += 0.2f * dt, theta += 0.3f * dt;
		m_BasicEffect.SetWorldMatrix(XMMatrixRotationX(phi) * XMMatrixRotationY(theta));
	}

	// �л���ʾģʽ
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_ShowMode = Mode::SplitedTriangle;
		ResetTriangle();
		// ����װ��׶εĶ��㻺��������
		UINT stride = sizeof(VertexPosColor);		// ��Խ�ֽ���
		UINT offset = 0;							// ��ʼƫ����
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
		m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext);
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_ShowMode = Mode::CylinderNoCap;
		ResetRoundWire();
		// ����װ��׶εĶ��㻺��������
		UINT stride = sizeof(VertexPosNormalColor);		// ��Խ�ֽ���
		UINT offset = 0;								// ��ʼƫ����
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
		m_BasicEffect.SetRenderCylinderNoCap(m_pd3dImmediateContext);
	}

	// ��ʾ������
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
	{
		if (m_ShowMode == Mode::CylinderNoCap)
			m_ShowMode = Mode::CylinderNoCapWithNormal;
		else if (m_ShowMode == Mode::CylinderNoCapWithNormal)
			m_ShowMode = Mode::CylinderNoCap;
	}

}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Ӧ�ó����������ı仯
	m_BasicEffect.Apply(m_pd3dImmediateContext);
	m_pd3dImmediateContext->Draw(m_VertexCount, 0);
	// ���Ʒ��������������ǵù�λ
	if (m_ShowMode == Mode::CylinderNoCapWithNormal)
	{
		m_BasicEffect.SetRenderNormal(m_pd3dImmediateContext);
		// Ӧ�ó����������ı仯
		m_BasicEffect.Apply(m_pd3dImmediateContext);
		m_pd3dImmediateContext->Draw(m_VertexCount, 0);
		m_BasicEffect.SetRenderCylinderNoCap(m_pd3dImmediateContext);
	}


	// ******************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"�л����ͣ�1-���ѵ������� 2-Բ�߹�������\n"
			"��ǰģʽ: ";
		if (m_ShowMode == Mode::SplitedTriangle)
			text += L"���ѵ�������";
		else if (m_ShowMode == Mode::CylinderNoCap)
			text += L"Բ�߹�������(Q-��ʾԲ�ߵķ�����)";
		else
			text += L"Բ�߹�������(Q-����Բ�ߵķ�����)";
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
	m_ShowMode = Mode::SplitedTriangle;
	ResetTriangle();
	
	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight;
	dirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.Direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	m_BasicEffect.SetDirLight(0, dirLight);
	// ����
	Material material;
	material.Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
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
	// Բ���߶�
	m_BasicEffect.SetCylinderHeight(2.0f);




	// ����װ��׶εĶ��㻺��������
	UINT stride = sizeof(VertexPosColor);		// ��Խ�ֽ���
	UINT offset = 0;							// ��ʼƫ����
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	// ����Ĭ����Ⱦ״̬
	m_BasicEffect.SetRenderSplitedTriangle(m_pd3dImmediateContext);


	return true;
}


void GameApp::ResetTriangle()
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
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));
	// �����ζ�����
	m_VertexCount = 3;
}

void GameApp::ResetRoundWire()
{
	// ****************** 
	// ��ʼ��Բ��
	// ����Բ���ϸ�����
	// ����Ҫ��˳ʱ������
	// ����Ҫ�γɱջ�����ʼ����Ҫʹ��2��
	//  ______
	// /      \
	// \______/
	//

	VertexPosNormalColor vertices[41];
	for (int i = 0; i < 40; ++i)
	{
		vertices[i].pos = XMFLOAT3(cosf(XM_PI / 20 * i), -1.0f, -sinf(XM_PI / 20 * i));
		vertices[i].normal = XMFLOAT3(cosf(XM_PI / 20 * i), 0.0f, -sinf(XM_PI / 20 * i));
		vertices[i].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	vertices[40] = vertices[0];

	// ���ö��㻺��������
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// �½����㻺����
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));
	// �߿򶥵���
	m_VertexCount = 41;
}



