#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "SkyRender.h"
#include "ObjReader.h"
#include "Collision.h"
class GameApp : public D3DApp
{
public:
	// �����ģʽ
	enum class CameraMode { FirstPerson, ThirdPerson, Free };
	// ��պ�ģʽ
	enum class SkyBoxMode { Daylight, Sunset, Desert };
	// ���嵱ǰģʽ
	enum class SphereMode { None, Reflection, Refraction };

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();
	
	void DrawScene(bool drawCenterSphere);

private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	GameObject m_Sphere;										// ��
	GameObject m_Ground;										// ����
	GameObject m_Cylinder;									    // Բ��

	BasicEffect m_BasicEffect;								    // ������Ⱦ��Ч����
	SkyEffect m_SkyEffect;									    // ��պ���Ч����
	
	std::unique_ptr<DynamicSkyRender> m_pDaylight;			    // ��պ�(����)
	std::unique_ptr<DynamicSkyRender> m_pSunset;				// ��պ�(����)
	std::unique_ptr<DynamicSkyRender> m_pDesert;				// ��պ�(ɳĮ)
	SkyBoxMode m_SkyBoxMode;									// ��պ�ģʽ

	SphereMode m_SphereMode;									// ����Ⱦģʽ
	float m_Eta;												// ����/����������


	std::shared_ptr<Camera> m_pCamera;						    // �����
	CameraMode m_CameraMode;									// �����ģʽ

	ObjReader m_ObjReader;									    // ģ�Ͷ�ȡ����
};


#endif