#pragma once

/*
 * ソフトモデル
 * 頂点を自由にいじることができるぞ
 */

// ソフトモデルメッシュのパーツ
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

// ソフトモデルメッシュ
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

// ソフトモデル (頂点データを編集できるクラス)
class SoftModel
{
public:
	SoftModelMesh::Collection								meshes;
	std::wstring											name;
};

// ソフトモデル変換クラス
class SoftModelConverter
{
public:
	// DirectXTKモデルから頂点データを取り出す
	static std::unique_ptr<SoftModel> FromModel(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::unique_ptr<DirectX::Model>& model);
	// 頂点データからDirectXTKモデルを構築
	static std::unique_ptr<DirectX::Model> ToModel(ID3D11Device* device, std::unique_ptr<SoftModel>& smodel);
	// インデックスから参照がない頂点を削除
	static void RemoveUnreferencedVertices(std::unique_ptr<SoftModel>& smodel);
	// ポリゴンの面の順番(時計回り/反時計回り)を変換する
	static void ConvertPolygonFaces(std::unique_ptr<SoftModel>& smodel, bool ccw);
	// メッシュのパーツを一体化
	static void MergeMeshParts(std::unique_ptr<SoftModel>& smodel);
	// メッシュを一体化
	static void MergeMesh(std::unique_ptr<SoftModel>& smodel);
};
