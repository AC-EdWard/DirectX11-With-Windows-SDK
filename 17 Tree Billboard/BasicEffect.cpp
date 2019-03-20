#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// ��������Effects.h��d3dUtil.h����
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;

//
// BasicEffect::Impl ��Ҫ����BasicEffect�Ķ���
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// ��Щ�ṹ���ӦHLSL�Ľṹ�塣��Ҫ��16�ֽڶ���
	//

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		Material material;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMVECTOR eyePos;
	};

	struct CBDrawingStates
	{
		DirectX::XMVECTOR fogColor;
		int fogEnabled;
		float fogStart;
		float fogRange;
		float pad;
	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};


	struct CBChangesRarely
	{
		DirectionalLight dirLight[BasicEffect::maxLights];
		PointLight pointLight[BasicEffect::maxLights];
		SpotLight spotLight[BasicEffect::maxLights];
	};

public:
	// ������ʽָ��
	Impl() = default;
	~Impl() = default;

public:
	// ��Ҫ16�ֽڶ�������ȷ���ǰ��
	CBufferObject<0, CBChangesEveryDrawing> m_CBDrawing;		// ÿ�ζ�����Ƶĳ���������
	CBufferObject<1, CBChangesEveryFrame>   m_CBFrame;		    // ÿ֡���Ƶĳ���������
	CBufferObject<2, CBDrawingStates>       m_CBStates;		    // ÿ�λ���״̬����ĳ���������
	CBufferObject<3, CBChangesOnResize>     m_CBOnResize;		// ÿ�δ��ڴ�С����ĳ���������
	CBufferObject<4, CBChangesRarely>		m_CBRarely;		    // �����������ĳ���������
	BOOL m_IsDirty;											    // �Ƿ���ֵ���
	std::vector<CBufferBase*> m_pCBuffers;					    // ͳһ�����������еĳ���������


	ComPtr<ID3D11VertexShader> m_pBasicVS;
	ComPtr<ID3D11PixelShader> m_pBasicPS;

	ComPtr<ID3D11VertexShader> m_pBillboardVS;
	ComPtr<ID3D11GeometryShader> m_pBillboardGS;
	ComPtr<ID3D11PixelShader> m_pBillboardPS;


	ComPtr<ID3D11InputLayout> m_pVertexPosSizeLayout;			// �㾫�����벼��
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;		// 3D�������벼��

	ComPtr<ID3D11ShaderResourceView> m_pTexture;				// ���ڻ��Ƶ�����
	ComPtr<ID3D11ShaderResourceView> m_pTextures;				// ���ڻ��Ƶ���������
};

//
// BasicEffect
//

namespace
{
	// BasicEffect����
	static BasicEffect * g_pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (g_pInstance)
		throw std::exception("BasicEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *g_pInstance;
}


bool BasicEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->m_pCBuffers.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;

	// ******************
	// ����3D����
	//
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS.cso", L"HLSL\\Basic_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicVS.GetAddressOf()));
	// �����������벼��
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBasicPS.GetAddressOf()));


	// ******************
	// ���ƹ����
	//
	HR(CreateShaderFromFile(L"HLSL\\Billboard_VS.cso", L"HLSL\\Billboard_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBillboardVS.GetAddressOf()));
	// �����������벼��
	HR(device->CreateInputLayout(VertexPosSize::inputLayout, ARRAYSIZE(VertexPosSize::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->m_pVertexPosSizeLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Billboard_GS.cso", L"HLSL\\Billboard_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBillboardGS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Billboard_PS.cso", L"HLSL\\Billboard_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pBillboardPS.GetAddressOf()));


	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBDrawing, 
		&pImpl->m_CBFrame, 
		&pImpl->m_CBStates, 
		&pImpl->m_CBOnResize, 
		&pImpl->m_CBRarely});

	// ��������������
	for (auto& pBuffer : pImpl->m_pCBuffers)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	return true;
}

void BasicEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pBasicVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->m_pBasicPS.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void BasicEffect::SetRenderBillboard(ComPtr<ID3D11DeviceContext> deviceContext, bool enableAlphaToCoverage)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosSizeLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pBillboardVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->m_pBillboardGS.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->m_pBillboardPS.Get(), nullptr, 0);
	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(
		(enableAlphaToCoverage ? RenderStates::BSAlphaToCoverage.Get() : nullptr),
		nullptr, 0xFFFFFFFF);

}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// ����ת�õ���
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.material = material;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> m_pTexture)
{
	pImpl->m_pTexture = m_pTexture;
}

void BasicEffect::SetTextureArray(ComPtr<ID3D11ShaderResourceView> m_pTextures)
{
	pImpl->m_pTextures = m_pTextures;
}

void XM_CALLCONV BasicEffect::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}



void BasicEffect::SetFogState(bool isOn)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogEnabled = isOn;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogStart(float fogStart)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogStart = fogStart;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogColor(DirectX::XMVECTOR fogColor)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogColor = fogColor;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetFogRange(float fogRange)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.fogRange = fogRange;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->m_pCBuffers;
	// ���������󶨵���Ⱦ������
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);

	pCBuffers[0]->BindGS(deviceContext);
	pCBuffers[1]->BindGS(deviceContext);
	pCBuffers[3]->BindGS(deviceContext);

	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[4]->BindPS(deviceContext);

	// ��������
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTexture.GetAddressOf());
	deviceContext->PSSetShaderResources(1, 1, pImpl->m_pTextures.GetAddressOf());

	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}

