#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"

using namespace DirectX;
using namespace std::experimental;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	if (!InitResource())
		return false;

	return true;
}

void GameApp::Compute()
{
	assert(m_pd3dImmediateContext);

//#if defined(DEBUG) | defined(_DEBUG)
//	ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//	HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//	graphicsAnalysis->BeginCapture();
//#endif

	// GPU����
	m_Timer.Reset();
	m_Timer.Start();
	GPUSort();
	m_Timer.Tick();
	m_Timer.Stop();
	float gpuTotalTime = m_Timer.TotalTime();

	// �����������
	m_pd3dImmediateContext->CopyResource(m_pTypedBufferCopy.Get(), m_pTypedBuffer1.Get());
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pTypedBufferCopy.Get(), 0, D3D11_MAP_READ, 0, &mappedData));

//#if defined(DEBUG) | defined(_DEBUG)
//	graphicsAnalysis->EndCapture();
//#endif

	// CPU����
	m_Timer.Reset();
	m_Timer.Start();
	std::sort(m_RandomNums.begin(), m_RandomNums.begin() + m_RandomNumsCount);
	m_Timer.Tick();
	m_Timer.Stop();
	float cpuTotalTime = m_Timer.TotalTime();

	bool isSame = !memcmp(mappedData.pData, m_RandomNums.data(),
		sizeof(UINT) * m_RandomNums.size());

	m_pd3dImmediateContext->Unmap(m_pTypedBufferCopy.Get(), 0);

	std::wstring wstr = L"����Ԫ����Ŀ��" + std::to_wstring(m_RandomNumsCount) +
		L"/" + std::to_wstring(m_RandomNums.size());
	wstr += L"\nGPU��ʱ��" + std::to_wstring(gpuTotalTime) + L"��";
	wstr += L"\nCPU��ʱ��" + std::to_wstring(cpuTotalTime) + L"��";
	wstr += isSame ? L"\n������һ��" : L"\n��������һ��";
	MessageBox(nullptr, wstr.c_str(), L"�������", MB_OK);

}



bool GameApp::InitResource()
{
	// ��ʼ�����������
	std::mt19937 randEngine;
	randEngine.seed(std::random_device()());
	std::uniform_int_distribution<UINT> powRange(9, 18);
	// Ԫ����Ŀ����Ϊ2�Ĵ����Ҳ�С��512�����������ֵ���
	m_RandomNums.assign((UINT)(1 << powRange(randEngine)), UINT_MAX);
	// ��������Ŀ�����������Ŀ��һ���������������֮��
	std::uniform_int_distribution<UINT> numsCountRange((UINT)m_RandomNums.size() / 2,
		(UINT)m_RandomNums.size());
	m_RandomNumsCount = numsCountRange(randEngine);
	std::generate(m_RandomNums.begin(), m_RandomNums.begin() + m_RandomNumsCount, [&] {return randEngine(); });

	HR(CreateTypedBuffer(m_pd3dDevice.Get(), m_RandomNums.data(), (UINT)m_RandomNums.size() * sizeof(UINT),
		m_pTypedBuffer1.GetAddressOf(), false, true));
	
	HR(CreateTypedBuffer(m_pd3dDevice.Get(), nullptr, (UINT)m_RandomNums.size() * sizeof(UINT),
		m_pTypedBuffer2.GetAddressOf(), false, true));

	HR(CreateTypedBuffer(m_pd3dDevice.Get(), nullptr, (UINT)m_RandomNums.size() * sizeof(UINT),
		m_pTypedBufferCopy.GetAddressOf(), true, true));

	HR(CreateConstantBuffer(m_pd3dDevice.Get(), nullptr, sizeof(CB),
		m_pConstantBuffer.GetAddressOf(), false, true));

	// ������ɫ����Դ��ͼ
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = (UINT)m_RandomNums.size();
	HR(m_pd3dDevice->CreateShaderResourceView(m_pTypedBuffer1.Get(), &srvDesc,
		m_pDataSRV1.GetAddressOf()));
	HR(m_pd3dDevice->CreateShaderResourceView(m_pTypedBuffer2.Get(), &srvDesc,
		m_pDataSRV2.GetAddressOf()));

	// �������������ͼ
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R32_UINT;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = (UINT)m_RandomNums.size();
	HR(m_pd3dDevice->CreateUnorderedAccessView(m_pTypedBuffer1.Get(), &uavDesc,
		m_pDataUAV1.GetAddressOf()));
	HR(m_pd3dDevice->CreateUnorderedAccessView(m_pTypedBuffer2.Get(), &uavDesc,
		m_pDataUAV2.GetAddressOf()));

	// ����������ɫ��
	ComPtr<ID3DBlob> blob;
	HR(CreateShaderFromFile(L"HLSL\\BitonicSort_CS.cso",
		L"HLSL\\BitonicSort_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pBitonicSort_CS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\MatrixTranspose_CS.cso",
		L"HLSL\\MatrixTranspose_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf()));
	HR(m_pd3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pMatrixTranspose_CS.GetAddressOf()));

	return true;
}

void GameApp::SetConstants(UINT level, UINT descendMask, UINT matrixWidth, UINT matrixHeight)
{
	CB cb = { level, descendMask, matrixWidth, matrixHeight };
	m_pd3dImmediateContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_pd3dImmediateContext->CSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
}

void GameApp::GPUSort()
{
	UINT size = (UINT)m_RandomNums.size();

	m_pd3dImmediateContext->CSSetShader(m_pBitonicSort_CS.Get(), nullptr, 0);
	m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pDataUAV1.GetAddressOf(), nullptr);

	// �������ݽ�������������level <= BLOCK_SIZE ���������
	for (UINT level = 2; level <= size && level <= BITONIC_BLOCK_SIZE; level *= 2)
	{
		SetConstants(level, level, 0, 0);
		m_pd3dImmediateContext->Dispatch((size + BITONIC_BLOCK_SIZE - 1) / BITONIC_BLOCK_SIZE, 1, 1);
	}
	
	// ��������ľ�����(��>=������Ҫ��Ϊ2�Ĵ���)
	UINT matrixWidth = 2, matrixHeight = 2;
	while (matrixWidth * matrixWidth < size)
	{
		matrixWidth *= 2;
	}
	matrixHeight = size / matrixWidth;

	// ����level > BLOCK_SIZE ���������
	ComPtr<ID3D11ShaderResourceView> pNullSRV;
	for (UINT level = BITONIC_BLOCK_SIZE * 2; level <= size; level *= 2)
	{
		// ����ﵽ��ߵȼ�����Ϊȫ��������
		if (level == size)
		{
			SetConstants(level / matrixWidth, level, matrixWidth, matrixHeight);
		}
		else
		{
			SetConstants(level / matrixWidth, level / matrixWidth, matrixWidth, matrixHeight);
		}
		// �Ƚ���ת�ã��������������Buffer2
		m_pd3dImmediateContext->CSSetShader(m_pMatrixTranspose_CS.Get(), nullptr, 0);
		m_pd3dImmediateContext->CSSetShaderResources(0, 1, pNullSRV.GetAddressOf());
		m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pDataUAV2.GetAddressOf(), nullptr);
		m_pd3dImmediateContext->CSSetShaderResources(0, 1, m_pDataSRV1.GetAddressOf());
		m_pd3dImmediateContext->Dispatch(matrixWidth / TRANSPOSE_BLOCK_SIZE, 
			matrixHeight / TRANSPOSE_BLOCK_SIZE, 1);

		// ��Buffer2����������
		m_pd3dImmediateContext->CSSetShader(m_pBitonicSort_CS.Get(), nullptr, 0);
		m_pd3dImmediateContext->Dispatch(size / BITONIC_BLOCK_SIZE, 1, 1);

		// ����ת�û������������������Buffer1
		SetConstants(matrixWidth, level, matrixWidth, matrixHeight);
		m_pd3dImmediateContext->CSSetShader(m_pMatrixTranspose_CS.Get(), nullptr, 0);
		m_pd3dImmediateContext->CSSetShaderResources(0, 1, pNullSRV.GetAddressOf());
		m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, m_pDataUAV1.GetAddressOf(), nullptr);
		m_pd3dImmediateContext->CSSetShaderResources(0, 1, m_pDataSRV2.GetAddressOf());
		m_pd3dImmediateContext->Dispatch(matrixWidth / TRANSPOSE_BLOCK_SIZE,
			matrixHeight / TRANSPOSE_BLOCK_SIZE, 1);

		// ��Buffer1����ʣ��������
		m_pd3dImmediateContext->CSSetShader(m_pBitonicSort_CS.Get(), nullptr, 0);
		m_pd3dImmediateContext->Dispatch(size / BITONIC_BLOCK_SIZE, 1, 1);
	}
}



