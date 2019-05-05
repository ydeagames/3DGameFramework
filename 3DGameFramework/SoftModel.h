#pragma once

/*
 * �\�t�g���f��
 * ���_�����R�ɂ����邱�Ƃ��ł��邼
 */

// �\�t�g���f�����b�V���̃p�[�c
class SoftModelMeshPart
{
public:
	using Vertex = DirectX::VertexPositionNormalTangentColorTextureSkinning;
	using Index = uint16_t;
	using Collection = std::vector<std::unique_ptr<SoftModelMeshPart>>;

	uint32_t												vertexStride;
	D3D_PRIMITIVE_TOPOLOGY									primitiveType;
	DXGI_FORMAT												indexFormat;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>				inputLayout;
	std::vector<Vertex>										vertices;
	std::vector<Index>										indices;
	std::shared_ptr<DirectX::IEffect>						effect;
	std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>	vbDecl;
	bool													isAlpha;
};

// �\�t�g���f�����b�V��
class SoftModelMesh
{
public:
	using Collection = std::vector<std::shared_ptr<SoftModelMesh>>;

	DirectX::BoundingSphere									boundingSphere;
	DirectX::BoundingBox									boundingBox;
	SoftModelMeshPart::Collection							meshParts;
	std::wstring											name;
	bool													ccw;
	bool													pmalpha;
};

// �\�t�g���f�� (���_�f�[�^��ҏW�ł���N���X)
class SoftModel
{
public:
	SoftModelMesh::Collection								meshes;
	std::wstring											name;
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
	// ���b�V���̃p�[�c����̉�
	static void MergeMeshParts(std::unique_ptr<SoftModel>& smodel);
	// ���b�V������̉�
	static void MergeMesh(std::unique_ptr<SoftModel>& smodel);
};
