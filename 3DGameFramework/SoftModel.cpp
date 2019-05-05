#include "SoftModel.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// ヘルパー関数
namespace
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) noexcept : result(hr) {}

		virtual const char* what() const override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}
}

// DirectXTKモデルから頂点データを取り出す
std::unique_ptr<SoftModel> SoftModelConverter::FromModel(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::unique_ptr<Model>& model)
{
	// 引数チェック
	if (!device || !deviceContext || !model)
		throw std::exception("Device, deviceContext and model cannot be null");

	// モデル
	auto smodel = std::make_unique<SoftModel>();

	// モデル>パラメータ
	smodel->name = model->name;

	// モデル>メッシュ処理
	for (auto& mesh : model->meshes)
	{
		// メッシュ
		assert(mesh != nullptr);
		auto smesh = std::make_shared<SoftModelMesh>();

		// メッシュ>パラメータ
		smesh->boundingSphere = mesh->boundingSphere;
		smesh->boundingBox = mesh->boundingBox;
		smesh->name = mesh->name;
		smesh->ccw = mesh->ccw;
		smesh->pmalpha = mesh->pmalpha;

		// メッシュ>パーツ処理
		for (auto& part : mesh->meshParts)
		{
			// パーツ
			assert(part != nullptr);
			auto spart = std::make_unique<SoftModelMeshPart>();

			// パーツ>パラメータ
			spart->vertexStride = part->vertexStride;
			spart->primitiveType = part->primitiveType;
			spart->indexFormat = part->indexFormat;
			spart->inputLayout = part->inputLayout;
			spart->effect = part->effect;
			spart->vbDecl = part->vbDecl;
			spart->isAlpha = part->isAlpha;

			// パーツ>インデックス
			{
				// サイズ
				auto nIndexes = part->indexCount;

				// [�AGPU上のCPU読み込み可能バッファ]を作成するための設定
				D3D11_BUFFER_DESC desc;
				part->indexBuffer->GetDesc(&desc);
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.BindFlags = 0;

				// [�AGPU上のCPU読み込み可能バッファ]を作成
				ID3D11Buffer* buf;
				ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &buf));

				// [�@GPU上のバッファ]を[�AGPU上のCPU読み込み可能バッファ]へコピー
				deviceContext->CopyResource(buf, part->indexBuffer.Get());

				// [�AGPU上のCPU読み込み可能バッファ]のメモリアドレスのマップを開く
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

				// [�BCPU上のバッファ]
				std::vector<uint16_t> inds(nIndexes);
				auto* pDstInds = &inds[0];

				// [�AGPU上のCPU読み込み可能バッファ]から[�BCPU上のバッファ]へ転送
				auto* pSrcInds = static_cast<uint16_t*>(mappedResource.pData);
				for (uint32_t i = 0; i < nIndexes; i++)
					pDstInds[i] = pSrcInds[i + part->startIndex] - part->vertexOffset;

				// [�AGPU上のCPU読み込み可能バッファ]のメモリアドレスのマップを閉じる
				deviceContext->Unmap(buf, 0);

				// [�AGPU上のCPU読み込み可能バッファ]を解放
				buf->Release();

				// パーツの頂点に[�BCPU上のバッファ]を設定
				spart->indices = std::move(inds);
			}

			// パーツ>頂点
			{
				// [�AGPU上のCPU読み込み可能バッファ]を作成するための設定
				D3D11_BUFFER_DESC desc;
				part->vertexBuffer->GetDesc(&desc);
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.BindFlags = 0;

				// サイズ
				auto nVerts = desc.ByteWidth / part->vertexStride;

				// [�AGPU上のCPU読み込み可能バッファ]を作成
				ID3D11Buffer* buf;
				ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &buf));

				// [�@GPU上のバッファ]を[�AGPU上のCPU読み込み可能バッファ]へコピー
				deviceContext->CopyResource(buf, part->vertexBuffer.Get());

				// [�AGPU上のCPU読み込み可能バッファ]のメモリアドレスのマップを開く
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

				// [�BCPU上のバッファ]
				decltype(spart->vertices) verts(nVerts);
				auto* pDstVerts = &verts[0];

				// [�AGPU上のCPU読み込み可能バッファ]から[�BCPU上のバッファ]へ転送
				auto* pSrcVerts = static_cast<char*>(mappedResource.pData);
				for (size_t i = 0; i < nVerts; i++)
					CopyMemory(&pDstVerts[i], &pSrcVerts[part->vertexOffset + part->vertexStride * i], part->vertexStride);

				// [�AGPU上のCPU読み込み可能バッファ]のメモリアドレスのマップを閉じる
				deviceContext->Unmap(buf, 0);

				// [�AGPU上のCPU読み込み可能バッファ]を解放
				buf->Release();

				// パーツの頂点に[�BCPU上のバッファ]を設定
				spart->vertices = std::move(verts);
			}

			// メッシュ>パーツ追加
			smesh->meshParts.emplace_back(std::move(spart));
		}

		// モデル>メッシュ追加
		smodel->meshes.emplace_back(std::move(smesh));
	}

	return smodel;
}

// 頂点データからDirectXTKモデルを構築
std::unique_ptr<Model> SoftModelConverter::ToModel(ID3D11Device* device, std::unique_ptr<SoftModel>& smodel)
{
	// 引数チェック
	if (!device || !smodel)
		throw std::exception("Device and softModel cannot be null");

	// モデル
	auto model = std::make_unique<Model>();

	// モデル>パラメータ
	model->name = smodel->name;

	// モデル>メッシュ処理
	for (auto& smesh : smodel->meshes)
	{
		// メッシュ
		assert(smesh != nullptr);
		auto mesh = std::make_shared<ModelMesh>();

		// メッシュ>パラメータ
		mesh->boundingSphere = smesh->boundingSphere;
		mesh->boundingBox = smesh->boundingBox;
		mesh->name = smesh->name;
		mesh->ccw = smesh->ccw;
		mesh->pmalpha = smesh->pmalpha;

		// メッシュ>パーツ処理
		for (auto& spart : smesh->meshParts)
		{
			// パーツ
			assert(spart != nullptr);
			auto part = std::make_unique<ModelMeshPart>();

			// パーツ>パラメータ
			part->vertexStride = spart->vertexStride;
			part->primitiveType = spart->primitiveType;
			part->indexFormat = spart->indexFormat;
			part->inputLayout = spart->inputLayout;
			part->effect = spart->effect;
			part->vbDecl = spart->vbDecl;
			part->isAlpha = spart->isAlpha;

			// パーツ>インデックス
			{
				// サイズ
				auto nIndexes = spart->indices.size();
				auto sizeInBytes = uint32_t(sizeof(uint16_t) * nIndexes);

				// [�@GPU上のバッファ]を作成するための設定
				D3D11_BUFFER_DESC desc = {};
				desc.ByteWidth = sizeInBytes;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				// [�BCPU上のバッファ]
				auto* pDstInds = &spart->indices[0];

				// [�BCPU上のバッファ]で[�@GPU上のバッファ]を初期化するための設定
				D3D11_SUBRESOURCE_DATA initData = {};
				initData.pSysMem = pDstInds;

				// [�@GPU上のバッファ]を作成
				ComPtr<ID3D11Buffer> buf;
				ThrowIfFailed(device->CreateBuffer(&desc, &initData, &buf));

				// パーツのバッファに[�@GPU上のバッファ]を設定
				part->startIndex = 0;
				part->indexCount = static_cast<uint32_t>(nIndexes);
				part->indexBuffer = std::move(buf);
			}

			// パーツ>頂点
			{
				// サイズ
				auto nVerts = spart->vertices.size();
				auto sizeInBytes = uint32_t(spart->vertexStride * nVerts);

				// [�@GPU上のバッファ]を作成するための設定
				D3D11_BUFFER_DESC desc = {};
				desc.ByteWidth = sizeInBytes;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				// [�BCPU上のバッファ]
				std::vector<char> verts(sizeInBytes);
				auto* pDstVerts = &verts[0];

				// [�BCPU上のバッファ]の初期化
				auto* pSrcVerts = &spart->vertices[0];
				for (size_t i = 0; i < nVerts; i++)
					CopyMemory(&pDstVerts[spart->vertexStride * i], &pSrcVerts[i], spart->vertexStride);

				// [�BCPU上のバッファ]で[�@GPU上のバッファ]を初期化するための設定
				D3D11_SUBRESOURCE_DATA initData = {};
				initData.pSysMem = pDstVerts;

				// [�@GPU上のバッファ]を作成
				ComPtr<ID3D11Buffer> buf;
				ThrowIfFailed(device->CreateBuffer(&desc, &initData, &buf));

				// パーツのバッファに[�@GPU上のバッファ]を設定
				part->vertexOffset = 0;
				part->vertexBuffer = std::move(buf);
			}

			// メッシュ>パーツ追加
			mesh->meshParts.emplace_back(std::move(part));
		}

		// モデル>メッシュ追加
		model->meshes.emplace_back(std::move(mesh));
	}

	return model;
}

// インデックスから参照がない頂点を削除
void SoftModelConverter::RemoveUnreferencedVertices(std::unique_ptr<SoftModel>& smodel)
{
	// モデル>メッシュ処理
	for (auto& smesh : smodel->meshes)
	{
		// メッシュ>パーツ処理
		for (auto& spart : smesh->meshParts)
		{
			// 頂点
			auto& verts = spart->vertices;
			auto& inds = spart->indices;
			// 使われている頂点のインデックス
			std::unordered_set<uint16_t> usedVerticesIndex(inds.begin(), inds.end());
			// 再配置前と再配置後のインデックス対応表 (要素番号: 再配置前, 値: 再配置後)
			std::vector<uint16_t> verticesIndexConvertMap(verts.size(), -1);
			// 次の再配置後のインデックス
			uint16_t indsCount = 0;
			// 使われていない頂点を削除
			verts.erase(std::remove_if(verts.begin(), verts.end(), [&](auto& value) {
				// 再配置前のインデックス
				uint16_t i = static_cast<uint16_t>(&value - &*spart->vertices.begin());
				// 使われているかどうか
				bool contains = usedVerticesIndex.find(i) != usedVerticesIndex.end();
				// 使われていたらインデックスを割り当て、1増やす
				if (contains) verticesIndexConvertMap[i] = indsCount++;
				// 使われていなかったら消す
				return !contains;
			}), verts.end());
			// インデックスを再割り当て
			for (auto& value : inds) {
				// 対応表から再割り当て
				value = verticesIndexConvertMap[value];
			}
		}
	}
}

// ポリゴンの面の順番(時計回り/反時計回り)を変換する
void SoftModelConverter::ConvertPolygonFaces(std::unique_ptr<SoftModel>& smodel, bool ccw)
{
	// モデル>メッシュ処理
	for (auto& smesh : smodel->meshes)
	{
		// ポリゴンの面の順番が違う場合のみ変換処理
		if (smesh->ccw != ccw)
		{
			// メッシュ>パーツ処理
			for (auto& spart : smesh->meshParts)
			{
				// 頂点
				auto& inds = spart->indices;
				size_t size = inds.size();
				// 三角形ABCの点Bと点Cを入れ替える
				for (size_t i = 0; i < size; i += 3)
					std::swap(inds[i + 1], inds[i + 2]);
			}
			// 設定を変更
			smesh->ccw = ccw;
		}
	}
}

// メッシュのパーツを一体化
void SoftModelConverter::MergeMeshParts(std::unique_ptr<SoftModel>& smodel)
{
	// モデル>メッシュ処理
	for (auto& smesh : smodel->meshes)
	{
		auto& sparts = smesh->meshParts;
		if (sparts.size() > 0)
		{
			// 集約頂点
			std::vector<SoftModelMeshPart::Vertex> verts;
			std::vector<SoftModelMeshPart::Index> inds;
			size_t startIndex = 0;

			// メッシュ>パーツ処理
			for (auto& spart : sparts)
			{
				verts.insert(verts.end(), spart->vertices.begin(), spart->vertices.end());
				for (auto& index : spart->indices)
					inds.push_back(index + startIndex);
				startIndex += spart->vertices.size();
			}

			// 2番目以降消去
			sparts.erase(sparts.begin() + 1, sparts.end());
			// 頂点更新
			sparts[0]->vertices = std::move(verts);
			sparts[0]->indices = std::move(inds);
		}
	}
}

// メッシュを一体化
void SoftModelConverter::MergeMesh(std::unique_ptr<SoftModel>& smodel)
{
	// 最低一個のメッシュが必要
	auto& smeshs = smodel->meshes;
	if (smeshs.size() <= 0)
		return;

	// 最低一個のパーツが必要
	auto& smesh0parts = smeshs[0]->meshParts;
	if (smesh0parts.size() <= 0)
		return;

	// マージの必要なし
	if (smeshs.size() == 1 && smesh0parts.size() == 1)
		return;

	// 集約頂点
	std::vector<SoftModelMeshPart::Vertex> verts;
	std::vector<SoftModelMeshPart::Index> inds;
	SoftModelMeshPart::Index startIndex = 0;

	// モデル>メッシュ処理
	for (auto& smesh : smeshs)
	{
		auto& sparts = smesh->meshParts;

		// メッシュ>パーツ処理
		for (auto& spart : sparts)
		{
			verts.insert(verts.end(), spart->vertices.begin(), spart->vertices.end());
			for (auto& index : spart->indices)
				inds.push_back(index + startIndex);
			startIndex += spart->vertices.size();
		}

	}

	// 2番目以降消去
	smeshs.erase(smeshs.begin() + 1, smeshs.end());
	smesh0parts.erase(smesh0parts.begin() + 1, smesh0parts.end());
	// 頂点更新
	smesh0parts[0]->vertices = std::move(verts);
	smesh0parts[0]->indices = std::move(inds);
}
