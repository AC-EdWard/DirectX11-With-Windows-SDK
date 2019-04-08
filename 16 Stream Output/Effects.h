//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// ������Ч������
// Simple effect management framework.
//***************************************************************************************

#ifndef EFFECTS_H
#define EFFECTS_H

#include <memory>
#include "LightHelper.h"
#include "RenderStates.h"

class IEffect
{
public:
	// ʹ��ģ�����(C++11)��������
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	IEffect() = default;

	// ��֧�ָ��ƹ���
	IEffect(const IEffect&) = delete;
	IEffect& operator=(const IEffect&) = delete;

	// ����ת��
	IEffect(IEffect&& moveFrom) = default;
	IEffect& operator=(IEffect&& moveFrom) = default;

	virtual ~IEffect() = default;

	// ���²��󶨳���������
	virtual void Apply(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
};


class BasicEffect : public IEffect
{
public:

	BasicEffect();
	virtual ~BasicEffect() override;

	BasicEffect(BasicEffect&& moveFrom) noexcept;
	BasicEffect& operator=(BasicEffect&& moveFrom) noexcept;

	// ��ȡ����
	static BasicEffect& Get();

	

	// ��ʼ��Basic.hlsli������Դ����ʼ����Ⱦ״̬
	bool InitAll(ComPtr<ID3D11Device> device);


	//
	// ��Ⱦģʽ�ı��
	//

	// ���������η���
	void SetRenderSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext);
	// ����ѩ��
	void SetRenderSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext);
	// ��������
	void SetRenderSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext);
	// ͨ��������׶λ�ȡ�����η��ѵ���һ�׷���
	void SetStreamOutputSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut);
	// ͨ��������׶λ�ȡѩ������һ�׷���
	void SetStreamOutputSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut);
	// ͨ��������׶λ�ȡ�����һ�׷���
	void SetStreamOutputSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut);

	// �������ж���ķ�����
	void SetRenderNormal(ComPtr<ID3D11DeviceContext> deviceContext);


	//
	// ��������
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W);
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);

	
	//
	// ���ա����ʺ������������
	//

	// �������͵ƹ�����������Ŀ
	static const int maxLights = 5;

	void SetDirLight(size_t pos, const DirectionalLight& dirLight);
	void SetPointLight(size_t pos, const PointLight& pointLight);
	void SetSpotLight(size_t pos, const SpotLight& spotLight);

	void SetMaterial(const Material& material);



	void XM_CALLCONV SetEyePos(DirectX::FXMVECTOR eyePos);

	//
	// ��������
	//

	void SetSphereCenter(const DirectX::XMFLOAT3& center);
	void SetSphereRadius(float radius);

	// Ӧ�ó�����������������Դ�ı��
	void Apply(ComPtr<ID3D11DeviceContext> deviceContext);
	
private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};










#endif
