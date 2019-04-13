#include "GameObject.h"
#include "d3dUtil.h"
using namespace DirectX;

GameObject::GameObject()
	: m_IndexCount(),
	m_Material(),
	m_VertexStride(),
	m_WorldMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f)
{
}

DirectX::XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3(m_WorldMatrix(3, 0), m_WorldMatrix(3, 1), m_WorldMatrix(3, 2));
}




void GameObject::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	m_pTexture = texture;
}

void GameObject::SetMaterial(const Material & material)
{
	m_Material = material;
}

void GameObject::SetWorldMatrix(const XMFLOAT4X4 & world)
{
	m_WorldMatrix = world;
}

void XM_CALLCONV GameObject::SetWorldMatrix(FXMMATRIX world)
{
	XMStoreFloat4x4(&m_WorldMatrix, world);
}

void GameObject::Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect)
{
	// ���ö���/����������
	UINT strides = m_VertexStride;
	UINT offsets = 0;
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &strides, &offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// �������ݲ�Ӧ��
	effect.SetWorldMatrix(XMLoadFloat4x4(&m_WorldMatrix));
	effect.SetTexture(m_pTexture);
	effect.SetMaterial(m_Material);
	effect.Apply(deviceContext);

	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}
