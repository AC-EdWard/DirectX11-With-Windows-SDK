#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Geometry.h"
#include "LightHelper.h"
#include "Camera.h"
#include "RenderStates.h"

class GameApp : public D3DApp
{
public:

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		Material material;
	};

	struct CBDrawingStates
	{
		int isReflection;
		DirectX::XMFLOAT3 pad;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT4 eyePos;

	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};


	struct CBChangesRarely
	{
		DirectX::XMMATRIX reflection;
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// �����֤16�ֽڶ���
	};

	// һ��������С����Ϸ������
	class GameObject
	{
	public:
		GameObject();

		// ��ȡλ��
		DirectX::XMFLOAT3 GetPosition() const;

		// ���û�����
		template<class VertexType, class IndexType>
		void SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData);
		// ��������
		void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
		// ���ò���
		void SetMaterial(const Material& material);
		// ���þ���
		void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
		void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX world);
		// ����
		void Draw(ComPtr<ID3D11DeviceContext> deviceContext);
	private:
		DirectX::XMFLOAT4X4 m_WorldMatrix;				    // �������
		Material m_Material;								// �������
		ComPtr<ID3D11ShaderResourceView> m_pTexture;		// ����
		ComPtr<ID3D11Buffer> m_pVertexBuffer;				// ���㻺����
		ComPtr<ID3D11Buffer> m_pIndexBuffer;				// ����������
		UINT m_VertexStride;								// �����ֽڴ�С
		UINT m_IndexCount;								    // ������Ŀ	
	};

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
	bool InitEffect();
	bool InitResource();

private:
	
	ComPtr<ID2D1SolidColorBrush> m_pColorBrush;				    // ��ɫ��ˢ
	ComPtr<IDWriteFont> m_pFont;								// ����
	ComPtr<IDWriteTextFormat> m_pTextFormat;					// �ı���ʽ

	ComPtr<ID3D11InputLayout> m_pVertexLayout2D;				// ����2D�Ķ������벼��
	ComPtr<ID3D11InputLayout> m_pVertexLayout3D;				// ����3D�Ķ������벼��
	ComPtr<ID3D11Buffer> m_pConstantBuffers[5];				    // ����������

	GameObject m_WireFence;									    // ��ʺ�
	GameObject m_Floor;										    // �ذ�
	std::vector<GameObject> m_Walls;							// ǽ��
	GameObject m_Water;										    // ˮ
	GameObject m_Mirror;										// ����

	ComPtr<ID3D11VertexShader> m_pVertexShader3D;				// ����3D�Ķ�����ɫ��
	ComPtr<ID3D11PixelShader> m_pPixelShader3D;				    // ����3D��������ɫ��
	ComPtr<ID3D11VertexShader> m_pVertexShader2D;				// ����2D�Ķ�����ɫ��
	ComPtr<ID3D11PixelShader> m_pPixelShader2D;				    // ����2D��������ɫ��

	CBDrawingStates m_CBStates;								    // �û�������Ż���״̬�ı��� 
	CBChangesEveryFrame m_CBFrame;							    // �û�������Ž���ÿһ֡���и��µı���
	CBChangesOnResize m_CBOnResize;							    // �û�������Ž��ڴ��ڴ�С�仯ʱ���µı���
	CBChangesRarely m_CBRarely;								    // �û�������Ų����ٽ����޸ĵı���

	ComPtr<ID3D11SamplerState> m_pSamplerState;				    // ������״̬

	std::shared_ptr<Camera> m_pCamera;						    // �����
	CameraMode m_CameraMode;									// �����ģʽ

};


#endif