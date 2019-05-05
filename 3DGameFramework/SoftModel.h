#pragma once

/*
 * �\�t�g���f��
 * ���_�����R�ɂ����邱�Ƃ��ł��邼
 */

// �\�t�g���f�����b�V���̃p�[�c
class SoftModelMeshPart
{
public:
	uint32_t																vertexStride;
	D3D_PRIMITIVE_TOPOLOGY													primitiveType;
	DXGI_FORMAT																indexFormat;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>								inputLayout;
	std::vector<DirectX::VertexPositionNormalTangentColorTextureSkinning>	vertices;
	std::vector<uint16_t>													indices;
	std::shared_ptr<DirectX::IEffect>										effect;
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>					vbDecl;
	bool																	isAlpha;

	typedef std::vector<std::unique_ptr<SoftModelMeshPart>>					Collection;
};

// �\�t�g���f�����b�V��
class SoftModelMesh
{
public:
	DirectX::BoundingSphere													boundingSphere;
	DirectX::BoundingBox													boundingBox;
	SoftModelMeshPart::Collection											meshParts;
	std::wstring															name;
	bool																	ccw;
	bool																	pmalpha;

	typedef std::vector<std::shared_ptr<SoftModelMesh>>						Collection;
};

// �\�t�g���f�� (���_�f�[�^��ҏW�ł���N���X)
class SoftModel
{
public:
	SoftModelMesh::Collection												meshes;
	std::wstring															name;
};

// �\�t�g���f���ϊ��N���X
class SoftModelConverter
{
public:
	// DirectXTK���f�����璸�_�f�[�^�����o��
	static std::unique_ptr<SoftModel> FromModel(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::unique_ptr<DirectX::Model>& model);
	// ���_�f�[�^����DirectXTK���f�����\�z
	static std::unique_ptr<DirectX::Model> ToModel(ID3D11Device* device, std::unique_ptr<SoftModel>& smodel);
	// �C���f�b�N�X����Q�Ƃ��Ȃ����_���폜
	static void RemoveUnreferencedVertices(std::unique_ptr<SoftModel>& smodel);
	// �|���S���̖ʂ̏���(���v���/�����v���)��ϊ�����
	static void ConvertPolygonFaces(std::unique_ptr<SoftModel>& smodel, bool ccw);
};
