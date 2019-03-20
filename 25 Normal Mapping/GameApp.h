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
	// ����ģʽ
	enum class GroundMode { Floor, Stones };

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

	ComPtr<ID3D11ShaderResourceView> m_FloorDiffuse;			// �ذ�����
	ComPtr<ID3D11ShaderResourceView> m_StonesDiffuse;		    // ����ʯ������

	Model m_GroundModel;										// ��������ģ��
	Model m_GroundTModel;									    // �����ߵĵ�������ģ��

	GameObject m_Sphere;										// ��
	GameObject m_Ground;										// ����
	GameObject m_GroundT;									    // �����������ĵ���
	GameObject m_Cylinder;									    // Բ��
	GameObject m_CylinderT;									    // ������������Բ��
	GroundMode m_GroundMode;									// ����ģʽ

	ComPtr<ID3D11ShaderResourceView> m_BricksNormalMap;		    // ש�鷨����ͼ
	ComPtr<ID3D11ShaderResourceView> m_FloorNormalMap;		    // ���淨����ͼ
	ComPtr<ID3D11ShaderResourceView> m_StonesNormalMap;		    // ʯͷ���淨����ͼ
	bool m_EnableNormalMap;									    // ����������ͼ

	BasicEffect m_BasicEffect;								    // ������Ⱦ��Ч����
	SkyEffect m_SkyEffect;									    // ��պ���Ч����
	

	std::unique_ptr<DynamicSkyRender> m_pDaylight;			    // ��պ�(����)

	std::shared_ptr<Camera> m_pCamera;						    // �����
	CameraMode m_CameraMode;									// �����ģʽ

	ObjReader m_ObjReader;									    // ģ�Ͷ�ȡ����
};


#endif