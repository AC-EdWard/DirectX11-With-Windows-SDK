#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Camera.h"
#include "GameObject.h"
#include "TextureRender.h"
#include "ObjReader.h"
#include "Collision.h"

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
	void CreateRandomTrees();

	void DrawScene(bool drawMiniMap);

private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	GameObject m_Trees;										    // ��
	GameObject m_Ground;										// ����
	std::vector<DirectX::XMMATRIX> m_InstancedData;			    // ����ʵ������
	Collision::WireFrameData m_TreeBoxData;					    // ����Χ���߿�����

	BasicEffect m_BasicEffect;								    // ������Ⱦ��Ч����
	ScreenFadeEffect m_ScreenFadeEffect;						// ��Ļ���뵭����Ч����
	MinimapEffect m_MinimapEffect;							    // С��ͼ��Ч����

	std::unique_ptr<TextureRender> m_pMinimapRender;			// С��ͼ��������
	std::unique_ptr<TextureRender> m_pScreenFadeRender;		    // ������������


	ComPtr<ID3D11Texture2D> m_pMinimapTexture;				    // С��ͼ����
	ComPtr<ID3D11ShaderResourceView> m_pMininmapSRV;			// С��ͼ��ɫ����Դ
	Model m_Minimap;											// С��ͼ����ģ��
	Model m_FullScreenShow;									    // ȫ����ʾ����ģ��

	std::shared_ptr<Camera> m_pCamera;						    // �����
	std::unique_ptr<FirstPersonCamera> m_MinimapCamera;		    // С��ͼ���������
	CameraMode m_CameraMode;									// �����ģʽ

	ObjReader m_ObjReader;									    // ģ�Ͷ�ȡ����

	bool m_PrintScreenStarted;								    // ������ǰ֡
	bool m_FadeUsed;											// �Ƿ�ʹ�õ���/����
	float m_FadeAmount;										    // ����/����ϵ��
	float m_FadeSign;										    // 1.0f��ʾ���룬-1.0f��ʾ����
};


#endif