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

private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	GameObject m_WoodCrate;									    // ľ��
	GameObject m_Floor;										    // �ذ�
	std::vector<GameObject> m_Walls;							// ǽ��
	GameObject m_Mirror;										// ����

	Material m_ShadowMat;									    // ��Ӱ����
	Material m_WoodCrateMat;									// ľ�в���

	BasicEffect m_BasicEffect;								    // ������Ⱦ��Ч����

	std::shared_ptr<Camera> m_pCamera;						    // �����
	CameraMode m_CameraMode;									// �����ģʽ

};


#endif