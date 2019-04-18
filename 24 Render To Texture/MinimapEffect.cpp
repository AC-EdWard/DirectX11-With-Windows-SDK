#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// ��������Effects.h��d3dUtil.h����
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;


//
// MinimapEffect::Impl ��Ҫ����MinimapEffect�Ķ���
//

class MinimapEffect::Impl : public AlignedType<MinimapEffect::Impl>
{
public:

	//
	// ��Щ�ṹ���ӦHLSL�Ľṹ�塣��Ҫ��16�ֽڶ���
	//

	struct CBChangesEveryFrame
	{
		XMVECTOR eyePos;
	};

	struct CBDrawingStates
	{
		int fogEnabled;
		float visibleRange;
		XMFLOAT2 pad;
		XMVECTOR rect;
		XMVECTOR invisibleColor;
	};


public:
	// ������ʽָ��
	Impl() : m_IsDirty() {}
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryFrame> m_CBFrame;		// ÿ֡�޸ĵĳ���������
	CBufferObject<1, CBDrawingStates>	m_CBStates;		    // ÿ�λ���״̬�ı�ĳ���������


	BOOL m_IsDirty;										    // �Ƿ���ֵ���
	std::vector<CBufferBase*> m_pCBuffers;				    // ͳһ�����������еĳ���������

	ComPtr<ID3D11VertexShader> m_pMinimapVS;
	ComPtr<ID3D11PixelShader> m_pMinimapPS;

	ComPtr<ID3D11InputLayout> m_pVertexPosTexLayout;

	ComPtr<ID3D11ShaderResourceView> m_pTexture;			// ���ڵ��뵭��������
};



//
// MinimapEffect
//

namespace
{
	// MinimapEffect����
	static MinimapEffect * g_pInstance = nullptr;
}

MinimapEffect::MinimapEffect()
{
	if (g_pInstance)
		throw std::exception("MinimapEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<MinimapEffect::Impl>();
}

MinimapEffect::~MinimapEffect()
{
}

MinimapEffect::MinimapEffect(MinimapEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

MinimapEffect & MinimapEffect::operator=(MinimapEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

MinimapEffect & MinimapEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("MinimapEffect needs an instance!");
	return *g_pInstance;
}

bool MinimapEffect::InitAll(ComPtr<ID3D11Device> device)
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

	HR(CreateShaderFromFile(L"HLSL\\Minimap_VS.cso", L"HLSL\\Minimap_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pMinimapVS.GetAddressOf()));
	// �������㲼��
	HR(device->CreateInputLayout(VertexPosTex::inputLayout, ARRAYSIZE(VertexPosTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosTexLayout.GetAddressOf()));

	// ******************
	// ����������ɫ��
	//

	HR(CreateShaderFromFile(L"HLSL\\Minimap_PS.cso", L"HLSL\\Minimap_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pMinimapPS.GetAddressOf()));


	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBFrame,
		&pImpl->m_CBStates
		});

	// ��������������
	for (auto& pBuffer : pImpl->m_pCBuffers)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	// ���õ��Զ�����
	D3D11SetDebugObjectName(pImpl->m_pVertexPosTexLayout.Get(), "MinimapEffect.VertexPosTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "MinimapEffect.CBFrame");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "MinimapEffect.CBStates");
	D3D11SetDebugObjectName(pImpl->m_pMinimapVS.Get(), "MinimapEffect.Minimap_VS");
	D3D11SetDebugObjectName(pImpl->m_pMinimapPS.Get(), "MinimapEffect.Minimap_PS");

	return true;
}

void MinimapEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pMinimapVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->m_pMinimapPS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 0);	// �ر���Ȳ���
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void MinimapEffect::SetFogState(bool isOn)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogEnabled = isOn;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void MinimapEffect::SetVisibleRange(float range)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.visibleRange = range;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV MinimapEffect::SetEyePos(DirectX::FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV MinimapEffect::SetMinimapRect(DirectX::FXMVECTOR rect)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.rect = rect;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV MinimapEffect::SetInvisibleColor(DirectX::FXMVECTOR color)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.invisibleColor = color;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void MinimapEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> m_pTexture)
{
	pImpl->m_pTexture = m_pTexture;
}

void MinimapEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->m_pCBuffers;
	// ���������󶨵���Ⱦ������
	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
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
