#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include <random>
// ��̲���֡(��֧��DirectX 11.2 API)
//#if defined(DEBUG) | defined(_DEBUG)
//#include <DXGItype.h>  
//#include <dxgi1_2.h>  
//#include <dxgi1_3.h>  
//#include <DXProgrammableCapture.h>  
//#endif

#define BITONIC_BLOCK_SIZE 512

#define TRANSPOSE_BLOCK_SIZE 16

class GameApp : public D3DApp
{
public:
	struct CB
	{
		UINT level;
		UINT descendMask;
		UINT matrixWidth;
		UINT matrixHeight;
	};

public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void Compute();

private:
	bool InitResource();
	void SetConstants(UINT level, UINT descendMask, UINT matrixWidth, UINT matrixHeight);
	void GPUSort();
private:
	ComPtr<ID3D11Buffer> m_pConstantBuffer;				// ����������
	ComPtr<ID3D11Buffer> m_pTypedBuffer1;				// �����ͻ�����1
	ComPtr<ID3D11Buffer> m_pTypedBuffer2;				// �����ͻ�����2
	ComPtr<ID3D11Buffer> m_pTypedBufferCopy;			// ���ڿ����������ͻ�����
	ComPtr<ID3D11UnorderedAccessView> m_pDataUAV1;		// �����ͻ�����1��Ӧ�����������ͼ
	ComPtr<ID3D11UnorderedAccessView> m_pDataUAV2;		// �����ͻ�����2��Ӧ�����������ͼ
	ComPtr<ID3D11ShaderResourceView> m_pDataSRV1;		// �����ͻ�����1��Ӧ����ɫ����Դ��ͼ
	ComPtr<ID3D11ShaderResourceView> m_pDataSRV2;		// �����ͻ�����2��Ӧ����ɫ����Դ��ͼ

	std::vector<UINT> m_RandomNums;
	UINT m_RandomNumsCount;
	ComPtr<ID3D11ComputeShader> m_pBitonicSort_CS;
	ComPtr<ID3D11ComputeShader> m_pMatrixTranspose_CS;
};


#endif