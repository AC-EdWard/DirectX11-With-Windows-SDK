#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// ��������Effects.h��d3dUtil.h����
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;


//
// ScreenFadeEffect::Impl ��Ҫ����ScreenFadeEffect�Ķ���
//

class ScreenFadeEffect::Impl : public AlignedType<ScreenFadeEffect::Impl>
{
public:

	//
	// ��Щ�ṹ���ӦHLSL�Ľṹ�塣��Ҫ��16�ֽڶ���
	//

	struct CBChangesEveryFrame
	{
		float fadeAmount;
		XMFLOAT3 pad;
	};

	struct CBChangesRarely
	{
		XMMATRIX worldViewProj;
	};


public:
	// ������ʽָ��
	Impl() = default;
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryFrame> m_CBFrame;		// ÿ֡�޸ĵĳ���������
	CBufferObject<1, CBChangesRarely>	m_CBRarely;		    // �����޸ĵĳ���������
	

	BOOL m_IsDirty;										    // �Ƿ���ֵ���
	std::vector<CBufferBase*> m_pCBuffers;				    // ͳһ�����������еĳ���������

	ComPtr<ID3D11VertexShader> m_pScreenFadeVS;
	ComPtr<ID3D11PixelShader> m_pScreenFadePS;

	ComPtr<ID3D11InputLayout> m_pVertexPosTexLayout;

	ComPtr<ID3D11ShaderResourceView> m_pTexture;			// ���ڵ��뵭��������
};



//
// ScreenFadeEffect
//

namespace
{
	// ScreenFadeEffect����
	static ScreenFadeEffect * g_pInstance = nullptr;
}

ScreenFadeEffect::ScreenFadeEffect()
{
	if (g_pInstance)
		throw std::exception("ScreenFadeEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<ScreenFadeEffect::Impl>();
}

ScreenFadeEffect::~ScreenFadeEffect()
{
}

ScreenFadeEffect::ScreenFadeEffect(ScreenFadeEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

ScreenFadeEffect & ScreenFadeEffect::operator=(ScreenFadeEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

ScreenFadeEffect & ScreenFadeEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("ScreenFadeEffect needs an instance!");
	return *g_pInstance;
}

bool ScreenFadeEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->m_pCBuffers.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;

	// ******************
	// ����������ɫ��
	//

	HR(CreateShaderFromFile(L"HLSL\\ScreenFade_VS.cso", L"HLSL\\ScreenFade_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pScreenFadeVS.GetAddressOf()));
	// �������㲼��
	HR(device->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosTexLayout.GetAddressOf()));

	// ******************
	// ����������ɫ��
	//

	HR(CreateShaderFromFile(L"HLSL\\ScreenFade_PS.cso", L"HLSL\\ScreenFade_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pScreenFadePS.GetAddressOf()));



	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBFrame,
		&pImpl->m_CBRarely
		});

	// ��������������
	for (auto& pBuffer : pImpl->m_pCBuffers)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	return true;
}

void ScreenFadeEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pScreenFadeVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->m_pScreenFadePS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV ScreenFadeEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX W, DirectX::CXMMATRIX V, DirectX::CXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.worldViewProj = XMMatrixTranspose(W * V * P);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV ScreenFadeEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX WVP)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.worldViewProj = XMMatrixTranspose(WVP);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void ScreenFadeEffect::SetFadeAmount(float fadeAmount)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.fadeAmount = fadeAmount;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}


void ScreenFadeEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> m_pTexture)
{
	pImpl->m_pTexture = m_pTexture;
}

void ScreenFadeEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->m_pCBuffers;
	// ���������󶨵���Ⱦ������
	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	// ����SRV
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTexture.GetAddressOf());

	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}
