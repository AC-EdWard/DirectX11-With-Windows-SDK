#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Effects.h"
#include "Vertex.h"

class GameApp : public D3DApp
{
public:
	enum class Mode { SplitedTriangle, CylinderNoCap, CylinderNoCapWithNormal };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	void ResetTriangle();
	void ResetRoundWire();



private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	ComPtr<ID3D11Buffer> m_pVertexBuffer;						// ���㼯��
	int m_VertexCount;										    // ������Ŀ
	Mode m_ShowMode;											// ��ǰ��ʾģʽ

	BasicEffect m_BasicEffect;							        // ������Ⱦ��Ч����

};


#endif