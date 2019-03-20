#ifndef D3DAPP_H
#define D3DAPP_H

#include <wrl/client.h>
#include <string>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include "GameTimer.h"

// �������Ҫ���õĿ�
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);    // �ڹ��캯���ĳ�ʼ���б�Ӧ�����úó�ʼ����
	virtual ~D3DApp();

	HINSTANCE AppInst()const;       // ��ȡӦ��ʵ���ľ��

	int Run();                      // ���г���ִ����Ϣ�¼���ѭ��

	// ��ܷ������ͻ���������Ҫ������Щ������ʵ���ض���Ӧ������
	virtual bool Init();            // �ø��෽����Ҫ��ʼ��Direct3D����
	virtual void Compute() = 0;     // ������Ҫʵ�ָ÷��������ÿһ֡�Ļ���
	// ���ڵ���Ϣ�ص�����
protected:
	bool InitDirect3D();			// Direct3D��ʼ��

protected:

	HINSTANCE m_hAppInst;			// Ӧ��ʵ�����
	GameTimer m_Timer;				// ��ʱ��

	// ʹ��ģ�����(C++11)��������
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	// Direct3D 11
	ComPtr<ID3D11Device> m_pd3dDevice;								// D3D11�豸
	ComPtr<ID3D11DeviceContext> m_pd3dImmediateContext;				// D3D11�豸������
	// Direct3D 11.1
	ComPtr<ID3D11Device1> m_pd3dDevice1;							// D3D11.1�豸
	ComPtr<ID3D11DeviceContext1> m_pd3dImmediateContext1;			// D3D11.1�豸������
};

#endif // D3DAPP_H