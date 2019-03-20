#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"


class GameApp : public D3DApp
{
public:
	// �����ģʽ
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
	
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();
	void InitPointSpritesBuffer();

private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	ComPtr<ID3D11Buffer> mPointSpritesBuffer;				    // �㾫�鶥�㻺����
	ComPtr<ID3D11ShaderResourceView> mTreeTexArray;			    // ������������
	Material m_TreeMat;										    // ���Ĳ���

	GameObject m_Ground;										// ����
	
	BasicEffect m_BasicEffect;							        // ������Ⱦ��Ч����

	CameraMode m_CameraMode;									// �����ģʽ
	std::shared_ptr<Camera> m_pCamera;						    // �����

	bool m_FogEnabled;										    // �Ƿ�����Ч
	bool m_IsNight;											    // �Ƿ��ҹ
	bool m_EnableAlphaToCoverage;							    // �Ƿ���Alpha-To-Coverage

	float m_FogRange;										    // ��Ч��Χ
};


#endif