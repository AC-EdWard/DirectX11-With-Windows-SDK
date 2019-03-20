//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// ������Ϸ����
// Simple game object.
//***************************************************************************************

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Model.h"


class GameObject
{
public:
	// ʹ��ģ�����(C++11)��������
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	GameObject();

	// ��ȡλ��
	DirectX::XMFLOAT3 GetPosition() const;
	
	//
	// ��ȡ��Χ��
	//

	DirectX::BoundingBox GetLocalBoundingBox() const;
	DirectX::BoundingBox GetBoundingBox() const;
	DirectX::BoundingOrientedBox GetBoundingOrientedBox() const;

	//
	// ����ģ��
	//
	
	void SetModel(Model&& model);
	void SetModel(const Model& model);

	//
	// ���þ���
	//

	void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX world);


	// ���ƶ���
	void Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect);

private:
	Model m_Model;												// ģ��
	DirectX::XMFLOAT4X4 m_WorldMatrix;							// �������

};


#endif


