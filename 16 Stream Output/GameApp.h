#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Effects.h"
#include "Vertex.h"

// ��̲���֡(��֧��DirectX 11.2 API)
//#if defined(DEBUG) | defined(_DEBUG)
//#include <DXGItype.h>  
//#include <dxgi1_2.h>  
//#include <dxgi1_3.h>  
//#include <DXProgrammableCapture.h>  
//#endif

class GameApp : public D3DApp
{
public:
	enum class Mode { SplitedTriangle, SplitedSnow, SplitedSphere };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	void ResetSplitedTriangle();
	void ResetSplitedSnow();
	void ResetSplitedSphere();


private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	ComPtr<ID3D11Buffer> m_pVertexBuffers[7];					// ���㻺��������
	int m_VertexCounts[7];									    // ������Ŀ
	int m_CurrIndex;											// ��ǰ����
	Mode m_ShowMode;											// ��ǰ��ʾģʽ
	bool m_IsWireFrame;										    // �Ƿ�Ϊ�߿�ģʽ
	bool m_ShowNormal;										    // �Ƿ���ʾ������

	BasicEffect m_BasicEffect;							        // ���������Ч������

};


#endif