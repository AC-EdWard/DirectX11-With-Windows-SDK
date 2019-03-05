#include "d3dApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include <sstream>

D3DApp::D3DApp(HINSTANCE hInstance)
	: mhAppInst(hInstance),
	md3dDevice(nullptr),
	md3dImmediateContext(nullptr)
{
}

D3DApp::~D3DApp()
{
	// �ָ�����Ĭ���趨
	if (md3dImmediateContext)
		md3dImmediateContext->ClearState();
}

HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

int D3DApp::Run()
{
	Compute();

	return 0;
}

bool D3DApp::Init()
{
	if (!InitDirect3D())
		return false;

	return true;
}

bool D3DApp::InitDirect3D()
{
	HRESULT hr = S_OK;

	// ����D3D�豸 �� D3D�豸������
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;	// Direct2D��Ҫ֧��BGRA��ʽ
#if defined(DEBUG) || defined(_DEBUG)  
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	// ������������
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// ���Եȼ�����
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL featureLevel;
	D3D_DRIVER_TYPE d3dDriverType;
	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		d3dDriverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, md3dDevice.GetAddressOf(), &featureLevel, md3dImmediateContext.GetAddressOf());
		
		if (hr == E_INVALIDARG)
		{
			// Direct3D 11.0 ��API������D3D_FEATURE_LEVEL_11_1������������Ҫ�������Եȼ�11.0�Լ����µİ汾
			hr = D3D11CreateDevice(nullptr, d3dDriverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, md3dDevice.GetAddressOf(), &featureLevel, md3dImmediateContext.GetAddressOf());
		}

		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	// ����Ƿ�֧�����Եȼ�11.0��11.1
	if (featureLevel != D3D_FEATURE_LEVEL_11_0 && featureLevel != D3D_FEATURE_LEVEL_11_1)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// �鿴�Ƿ�֧��Direct3D 11.1(����ʹ��)
	md3dDevice.As(&md3dDevice1);
	md3dImmediateContext.As(&md3dImmediateContext1);

	return true;
}

