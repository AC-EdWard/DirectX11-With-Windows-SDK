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

	// ����������ʾ
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

	// ******************
	// ��¼����������λ�ú���ת����
	//
	static float theta = 0.0f, phi = 0.0f;
	static XMMATRIX Left = XMMatrixTranslation(-5.0f, 0.0f, 0.0f);
	static XMMATRIX Top = XMMatrixTranslation(0.0f, 4.0f, 0.0f);
	static XMMATRIX Right = XMMatrixTranslation(5.0f, 0.0f, 0.0f);
	static XMMATRIX Bottom = XMMatrixTranslation(0.0f, -4.0f, 0.0f);

	theta += dt * 0.5f;
	phi += dt * 0.3f;
	// ���������˶�
	m_Sphere.SetWorldMatrix(Left);
	m_Cube.SetWorldMatrix(XMMatrixRotationX(-phi) * XMMatrixRotationY(theta) * Top);
	m_Cylinder.SetWorldMatrix(XMMatrixRotationX(phi) * XMMatrixRotationY(theta) * Right);
	m_House.SetWorldMatrix(XMMatrixScaling(0.005f, 0.005f, 0.005f) * XMMatrixRotationY(theta) * Bottom);
	m_Triangle.SetWorldMatrix(XMMatrixRotationY(theta));

	// ******************
	// ʰȡ���
	//
	m_PickedObjStr = L"��";
	Ray ray = Ray::ScreenToRay(*m_pCamera, (float)mouseState.x, (float)mouseState.y);
	
	// �����ζ���任
	static XMVECTOR V[3];
	for (int i = 0; i < 3; ++i)
	{
		V[i] = XMVector3TransformCoord(XMLoadFloat3(&m_TriangleMesh.vertexVec[i].pos), 
			XMMatrixRotationY(theta));
	}

	bool hitObject = false;
	if (ray.Hit(m_BoundingSphere))
	{
		m_PickedObjStr = L"����";
		hitObject = true;
	}
	else if (ray.Hit(m_Cube.GetBoundingOrientedBox()))
	{
		m_PickedObjStr = L"������";
		hitObject = true;
	}
	else if (ray.Hit(m_Cylinder.GetBoundingOrientedBox()))
	{
		m_PickedObjStr = L"Բ����";
		hitObject = true;
	}
	else if (ray.Hit(m_House.GetBoundingOrientedBox()))
	{
		m_PickedObjStr = L"����";
		hitObject = true;
	}
	else if (ray.Hit(V[0], V[1], V[2]))
	{
		m_PickedObjStr = L"������";
		hitObject = true;
	}

	if (hitObject == true && m_MouseTracker.leftButton == Mouse::ButtonStateTracker::PRESSED)
	{
		std::wstring wstr = L"������";
		wstr += m_PickedObjStr + L"!";
		MessageBox(nullptr, wstr.c_str(), L"ע��", 0);
	}

	// ���ù���ֵ
	m_pMouse->ResetScrollWheelValue();
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	// ******************
	// ����Direct3D����
	//
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ���Ʋ���Ҫ�����ģ��
	m_BasicEffect.SetTextureUsed(false);
	m_Sphere.Draw(m_pd3dImmediateContext, m_BasicEffect);
	m_Cube.Draw(m_pd3dImmediateContext, m_BasicEffect);
	m_Cylinder.Draw(m_pd3dImmediateContext, m_BasicEffect);
	m_Triangle.Draw(m_pd3dImmediateContext, m_BasicEffect);

	// ������Ҫ�����ģ��
	m_BasicEffect.SetTextureUsed(true);
	m_House.Draw(m_pd3dImmediateContext, m_BasicEffect);

	// ******************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"��ǰʰȡ����: " + m_PickedObjStr;

		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	// ******************
	// ��ʼ����Ϸ����
	//
	
	// ����(Ԥ����ð�Χ��)
	m_Sphere.SetModel(Model(m_pd3dDevice, Geometry::CreateSphere()));
	m_BoundingSphere.Center = XMFLOAT3(-5.0f, 0.0f, 0.0f);
	m_BoundingSphere.Radius = 1.0f;
	// ������
	m_Cube.SetModel(Model(m_pd3dDevice, Geometry::CreateBox()));
	// Բ����
	m_Cylinder.SetModel(Model(m_pd3dDevice, Geometry::CreateCylinder()));
	// ����
	m_ObjReader.Read(L"Model\\house.mbo", L"Model\\house.obj");
	m_House.SetModel(Model(m_pd3dDevice, m_ObjReader));
	// ������(������)
	m_TriangleMesh.vertexVec.assign({
		VertexPosNormalTex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2()),
		VertexPosNormalTex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2())
		});
	m_TriangleMesh.indexVec.assign({ 0, 1, 2, 3, 4, 5 });
	m_Triangle.SetModel(Model(m_pd3dDevice, m_TriangleMesh));

	// ******************
	// ��ʼ�������
	//
	m_CameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	// ��ʼ�������¹۲����ͶӰ����(����������̶�)
	camera->UpdateViewMatrix();
	m_BasicEffect.SetViewMatrix(camera->GetViewXM());
	m_BasicEffect.SetProjMatrix(camera->GetProjXM());
	

	// ******************
	// ��ʼ������仯��ֵ
	//

	// �����
	DirectionalLight dirLight;
	dirLight.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	dirLight.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
	dirLight.direction = XMFLOAT3(-0.707f, -0.707f, 0.707f);
	m_BasicEffect.SetDirLight(0, dirLight);

	// Ĭ��ֻ���������
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext, BasicEffect::RenderObject);

	return true;
}

