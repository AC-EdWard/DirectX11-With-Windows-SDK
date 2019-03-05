#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include <random>
// ��̲���֡
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
	ComPtr<ID3D11Buffer> mConstantBuffer;			// ����������
	ComPtr<ID3D11Buffer> mTypedBuffer1;				// �����ͻ�����1
	ComPtr<ID3D11Buffer> mTypedBuffer2;				// �����ͻ�����2
	ComPtr<ID3D11Buffer> mTypedBufferCopy;			// ���ڿ����������ͻ�����
	ComPtr<ID3D11UnorderedAccessView> mDataUAV1;	// �����ͻ�����1��Ӧ�����������ͼ
	ComPtr<ID3D11UnorderedAccessView> mDataUAV2;	// �����ͻ�����2��Ӧ�����������ͼ
	ComPtr<ID3D11ShaderResourceView> mDataSRV1;		// �����ͻ�����1��Ӧ����ɫ����Դ��ͼ
	ComPtr<ID3D11ShaderResourceView> mDataSRV2;		// �����ͻ�����2��Ӧ����ɫ����Դ��ͼ

	std::vector<UINT> mRandomNums;
	UINT mRandomNumsCount;
	ComPtr<ID3D11ComputeShader> mBitonicSort_CS;
	ComPtr<ID3D11ComputeShader> mMatrixTranspose_CS;
};


#endif