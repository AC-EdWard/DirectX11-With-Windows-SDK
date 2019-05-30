#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_CameraMode(CameraMode::Free),
	m_FadeAmount(0.0f),
	m_FadeSign(1.0f),
	m_FadeUsed(true),	// ��ʼ����
	m_PrintScreenStarted(false)
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

	if (!m_ScreenFadeEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!m_MinimapEffect.InitAll(m_pd3dDevice.Get()))
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
		OutputDebugStringW(L"\n���棺Direct2D��Direct3D�������Թ������ޣ��㽫�޷������ı���Ϣ�����ṩ������ѡ������\n"
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
		// С��ͼ����ģ������
		m_Minimap.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow(1.0f - 100.0f / m_ClientWidth * 2,  -1.0f + 100.0f / m_ClientHeight * 2,
			100.0f / m_ClientWidth * 2, 100.0f / m_ClientHeight * 2));
		// ��Ļ���뵭�������С����
		m_pScreenFadeRender = std::make_unique<TextureRender>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, false);
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

	auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

	// ���µ��뵭��ֵ��������������ж�
	if (m_FadeUsed)
	{
		m_FadeAmount += m_FadeSign * dt / 2.0f;	// 2sʱ�䵭��/����
		if (m_FadeSign > 0.0f && m_FadeAmount > 1.0f)
		{
			m_FadeAmount = 1.0f;
			m_FadeUsed = false;	// ��������
		}
		else if (m_FadeSign < 0.0f && m_FadeAmount < 0.0f)
		{
			m_FadeAmount = 0.0f;
			SendMessage(MainWnd(), WM_DESTROY, 0, 0);	// �رճ���
			// ���ﲻ������������Ϊ���͹رմ��ڵ���Ϣ��Ҫ��һ��������ر�
		}
	}
	else
	{
		// ******************
		// ����������Ĳ���
		//

		// �����ƶ�
		if (keyState.IsKeyDown(Keyboard::W))
			cam1st->Walk(dt * 3.0f);
		if (keyState.IsKeyDown(Keyboard::S))
			cam1st->Walk(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::A))
			cam1st->Strafe(dt * -3.0f);
		if (keyState.IsKeyDown(Keyboard::D))
			cam1st->Strafe(dt * 3.0f);

		// ��Ұ��ת����ֹ��ʼ�Ĳ�ֵ�����µ�ͻȻ��ת
		cam1st->Pitch(mouseState.y * dt * 1.25f);
		cam1st->RotateY(mouseState.x * dt * 1.25f);

		// �����ƶ���Χ
		XMFLOAT3 adjustedPos;
		XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorReplicate(-90.0f), XMVectorReplicate(90.0f)));
		cam1st->SetPosition(adjustedPos);
	}

	// ���¹۲����
	m_pCamera->UpdateViewMatrix();
	m_BasicEffect.SetViewMatrix(m_pCamera->GetViewXM());
	m_BasicEffect.SetEyePos(m_pCamera->GetPositionXM());
	m_MinimapEffect.SetEyePos(m_pCamera->GetPositionXM());
	
	// ����
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
		m_PrintScreenStarted = true;
		
	// ���ù���ֵ
	m_pMouse->ResetScrollWheelValue();

	// �˳����򣬿�ʼ����
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::Escape))
	{
		m_FadeSign = -1.0f;
		m_FadeUsed = true;
	}
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	
	// ******************
	// ����Direct3D����
	//

	// Ԥ����պ󱸻�����
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	if (m_FadeUsed)
	{
		// ��ʼ����/����
		m_pScreenFadeRender->Begin(m_pd3dImmediateContext.Get());
	}


	// ����������
	DrawScene(false);

	// �˴�����С��ͼ����Ļ����
	UINT strides[1] = { sizeof(VertexPosTex) };
	UINT offsets[1] = { 0 };
	
	// С��ͼ��ЧӦ��
	m_MinimapEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
	m_MinimapEffect.Apply(m_pd3dImmediateContext.Get());
	// ������С��ͼ
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_Minimap.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
	m_pd3dImmediateContext->IASetIndexBuffer(m_Minimap.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pd3dImmediateContext->DrawIndexed(6, 0, 0);

	if (m_FadeUsed)
	{
		// ��������/��������ʱ���Ƶĳ�������Ļ���뵭����Ⱦ������
		m_pScreenFadeRender->End(m_pd3dImmediateContext.Get());

		// ��Ļ���뵭����ЧӦ��
		m_ScreenFadeEffect.SetRenderDefault(m_pd3dImmediateContext.Get());
		m_ScreenFadeEffect.SetFadeAmount(m_FadeAmount);
		m_ScreenFadeEffect.SetTexture(m_pScreenFadeRender->GetOutputTexture());
		m_ScreenFadeEffect.SetWorldViewProjMatrix(XMMatrixIdentity());
		m_ScreenFadeEffect.Apply(m_pd3dImmediateContext.Get());
		// ������������������Ļ
		m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_FullScreenShow.modelParts[0].vertexBuffer.GetAddressOf(), strides, offsets);
		m_pd3dImmediateContext->IASetIndexBuffer(m_FullScreenShow.modelParts[0].indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
		m_pd3dImmediateContext->DrawIndexed(6, 0, 0);
		// ��ؽ��������ɫ���ϵ���Դ����Ϊ��һ֡��ʼ������Ϊ��ȾĿ��
		m_ScreenFadeEffect.SetTexture(nullptr);
		m_ScreenFadeEffect.Apply(m_pd3dImmediateContext.Get());
	}
	
	// ��������Q���£���ֱ𱣴浽output.dds��output.png��
	if (m_PrintScreenStarted)
	{
		ComPtr<ID3D11Texture2D> backBuffer;
		// �������
		m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
		HR(SaveDDSTextureToFile(m_pd3dImmediateContext.Get(), backBuffer.Get(), L"Screenshot\\output.dds"));
		HR(SaveWICTextureToFile(m_pd3dImmediateContext.Get(), backBuffer.Get(), GUID_ContainerFormatPng, L"Screenshot\\output.png"));
		// ��������
		m_PrintScreenStarted = false;
	}



	// ******************
	// ����Direct2D����
	//
	if (m_pd2dRenderTarget != nullptr)
	{
		m_pd2dRenderTarget->BeginDraw();
		std::wstring text = L"��ǰ�����ģʽ: ��һ�˳�  Esc�˳�\n"
			"����ƶ�������Ұ W/S/A/D�ƶ�\n"
			"Q-����(���output.dds��output.png��ScreenShot�ļ���)";



		m_pd2dRenderTarget->DrawTextW(text.c_str(), (UINT32)text.length(), m_pTextFormat.Get(),
			D2D1_RECT_F{ 0.0f, 0.0f, 600.0f, 200.0f }, m_pColorBrush.Get());
		HR(m_pd2dRenderTarget->EndDraw());
	}

	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{

	// ******************
	// ��ʼ������Render-To-Texture�Ķ���
	//
	m_pMinimapRender = std::make_unique<TextureRender>(m_pd3dDevice.Get(), 400, 400, true);
	m_pScreenFadeRender = std::make_unique<TextureRender>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, false);

	// ******************
	// ��ʼ����Ϸ����
	//

	// �����������
	CreateRandomTrees();

	// ��ʼ������
	m_ObjReader.Read(L"Model\\ground.mbo", L"Model\\ground.obj");
	m_Ground.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));

	// ��ʼ�����񣬷��������½�200x200
	m_Minimap.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow(0.75f, -0.66666666f, 0.25f, 0.33333333f));
	
	// ����������Ļ�������ģ��
	m_FullScreenShow.SetMesh(m_pd3dDevice.Get(), Geometry::Create2DShow());

	// ******************
	// ��ʼ�������
	//

	// Ĭ�������
	m_CameraMode = CameraMode::FirstPerson;
	auto camera = std::shared_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_pCamera = camera;
	camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
	camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
	camera->LookTo(
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera->UpdateViewMatrix();
	

	// С��ͼ�����
	m_MinimapCamera = std::unique_ptr<FirstPersonCamera>(new FirstPersonCamera);
	m_MinimapCamera->SetViewPort(0.0f, 0.0f, 200.0f, 200.0f);	// 200x200С��ͼ
	m_MinimapCamera->LookTo(
		XMVectorSet(0.0f, 10.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, -1.0f, 0.0f, 1.0f),
		XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
	m_MinimapCamera->UpdateViewMatrix();

	// ******************
	// ��ʼ����������仯��ֵ
	//

	// ��ҹ��Ч
	m_BasicEffect.SetFogColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	m_BasicEffect.SetFogStart(5.0f);
	m_BasicEffect.SetFogRange(20.0f);

	// С��ͼ��Χ����
	m_MinimapEffect.SetFogState(true);
	m_MinimapEffect.SetInvisibleColor(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	m_MinimapEffect.SetMinimapRect(XMVectorSet(-95.0f, 95.0f, 95.0f, -95.0f));
	m_MinimapEffect.SetVisibleRange(25.0f);

	// �����(Ĭ��)
	DirectionalLight dirLight[4];
	dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
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
	// ��ȾС��ͼ����
	// 

	m_BasicEffect.SetViewMatrix(m_MinimapCamera->GetViewXM());
	m_BasicEffect.SetProjMatrix(XMMatrixOrthographicLH(190.0f, 190.0f, 1.0f, 20.0f));	// ʹ������ͶӰ����(�����������λ��)
	// �ر���Ч
	m_BasicEffect.SetFogState(false);
	m_pMinimapRender->Begin(m_pd3dImmediateContext.Get());
	DrawScene(true);
	m_pMinimapRender->End(m_pd3dImmediateContext.Get());

	m_MinimapEffect.SetTexture(m_pMinimapRender->GetOutputTexture());


	// ������Ч���ָ�ͶӰ��������ƫ���Ĺ���
	m_BasicEffect.SetFogState(true);
	m_BasicEffect.SetProjMatrix(m_pCamera->GetProjXM());
	dirLight[0].ambient = XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f);
	dirLight[0].diffuse = XMFLOAT4(0.16f, 0.16f, 0.16f, 1.0f);
	for (int i = 0; i < 4; ++i)
		m_BasicEffect.SetDirLight(i, dirLight[i]);
	
	// ******************
	// ���õ��Զ�����
	//
	m_Trees.SetDebugObjectName("Trees");
	m_Ground.SetDebugObjectName("Ground");
	m_FullScreenShow.SetDebugObjectName("FullScreenShow");
	m_Minimap.SetDebugObjectName("Minimap");
	m_pMinimapRender->SetDebugObjectName("Minimap");
	m_pScreenFadeRender->SetDebugObjectName("ScreenFade");
	

	return true;
}

void GameApp::DrawScene(bool drawMinimap)
{

	m_BasicEffect.SetTextureUsed(true);


	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
	if (drawMinimap)
	{
		// С��ͼ�»���������
		m_Trees.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, m_InstancedData);
	}
	else
	{
		// ͳ��ʵ�ʻ��Ƶ�������Ŀ
		std::vector<XMMATRIX> acceptedData;
		// Ĭ����׶��ü�
		acceptedData = Collision::FrustumCulling(m_InstancedData, m_Trees.GetLocalBoundingBox(),
			m_pCamera->GetViewXM(), m_pCamera->GetProjXM());
		// Ĭ��Ӳ��ʵ��������
		m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderInstance);
		m_Trees.DrawInstanced(m_pd3dImmediateContext.Get(), m_BasicEffect, acceptedData);
	}
	
	// ���Ƶ���
	m_BasicEffect.SetRenderDefault(m_pd3dImmediateContext.Get(), BasicEffect::RenderObject);
	m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
}

void GameApp::CreateRandomTrees()
{
	srand((unsigned)time(nullptr));
	// ��ʼ����
	m_ObjReader.Read(L"Model\\tree.mbo", L"Model\\tree.obj");
	m_Trees.SetModel(Model(m_pd3dDevice.Get(), m_ObjReader));
	XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);

	BoundingBox treeBox = m_Trees.GetLocalBoundingBox();
	// ��ȡ����Χ�ж���
	m_TreeBoxData = Collision::CreateBoundingBox(treeBox, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	// ����ľ�ײ���������λ��y = -2��ƽ��
	treeBox.Transform(treeBox, S);
	XMMATRIX T0 = XMMatrixTranslation(0.0f, -(treeBox.Center.y - treeBox.Extents.y + 2.0f), 0.0f);
	// �������144������������
	float theta = 0.0f;
	for (int i = 0; i < 16; ++i)
	{
		// ȡ5-95�İ뾶�����������
		for (int j = 0; j < 3; ++j)
		{
			// ����ԽԶ����ľԽ��
			for (int k = 0; k < 2 * j + 1; ++k)
			{
				float radius = (float)(rand() % 30 + 30 * j + 5);
				float randomRad = rand() % 256 / 256.0f * XM_2PI / 16;
				XMMATRIX T1 = XMMatrixTranslation(radius * cosf(theta + randomRad), 0.0f, radius * sinf(theta + randomRad));
				XMMATRIX R = XMMatrixRotationY(rand() % 256 / 256.0f * XM_2PI);
				XMMATRIX World = S * R * T0 * T1;
				m_InstancedData.push_back(World);
			}
		}
		theta += XM_2PI / 16;
	}

	m_Trees.ResizeBuffer(m_pd3dDevice.Get(), 144);
}
