#include "SoftModel.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// �w���p�[�֐�
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

// DirectXTK���f�����璸�_�f�[�^�����o��
std::unique_ptr<SoftModel> SoftModelConverter::FromModel(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::unique_ptr<Model>& model)
{
	// �����`�F�b�N
	if (!device || !deviceContext || !model)
		throw std::exception("Device, deviceContext and model cannot be null");

	// ���f��
	auto smodel = std::make_unique<SoftModel>();

	// ���f��>�p�����[�^
	smodel->name = model->name;

	// ���f��>���b�V������
	for (auto& mesh : model->meshes)
	{
		// ���b�V��
		assert(mesh != nullptr);
		auto smesh = std::make_shared<SoftModelMesh>();

		// ���b�V��>�p�����[�^
		smesh->boundingSphere = mesh->boundingSphere;
		smesh->boundingBox = mesh->boundingBox;
		smesh->name = mesh->name;
		smesh->ccw = mesh->ccw;
		smesh->pmalpha = mesh->pmalpha;

		// ���b�V��>�p�[�c����
		for (auto& part : mesh->meshParts)
		{
			// �p�[�c
			assert(part != nullptr);
			auto spart = std::make_unique<SoftModelMeshPart>();

			// �p�[�c>�p�����[�^
			spart->vertexStride = part->vertexStride;
			spart->primitiveType = part->primitiveType;
			spart->indexFormat = part->indexFormat;
			spart->inputLayout = part->inputLayout;
			spart->effect = part->effect;
			spart->vbDecl = part->vbDecl;
			spart->isAlpha = part->isAlpha;

			// �p�[�c>�C���f�b�N�X
			{
				// �T�C�Y
				auto nIndexes = part->indexCount;

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]���쐬���邽�߂̐ݒ�
				D3D11_BUFFER_DESC desc;
				part->indexBuffer->GetDesc(&desc);
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.BindFlags = 0;

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]���쐬
				ID3D11Buffer* buf;
				ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &buf));

				// [�@GPU��̃o�b�t�@]��[�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�փR�s�[
				deviceContext->CopyResource(buf, part->indexBuffer.Get());

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�̃������A�h���X�̃}�b�v���J��
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

				// [�BCPU��̃o�b�t�@]
				std::vector<uint16_t> inds(nIndexes);
				auto* pDstInds = &inds[0];

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]����[�BCPU��̃o�b�t�@]�֓]��
				auto* pSrcInds = static_cast<uint16_t*>(mappedResource.pData);
				for (uint32_t i = 0; i < nIndexes; i++)
					pDstInds[i] = pSrcInds[i + part->startIndex] - part->vertexOffset;

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�̃������A�h���X�̃}�b�v�����
				deviceContext->Unmap(buf, 0);

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�����
				buf->Release();

				// �p�[�c�̒��_��[�BCPU��̃o�b�t�@]��ݒ�
				spart->indices = std::move(inds);
			}

			// �p�[�c>���_
			{
				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]���쐬���邽�߂̐ݒ�
				D3D11_BUFFER_DESC desc;
				part->vertexBuffer->GetDesc(&desc);
				desc.Usage = D3D11_USAGE_STAGING;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				desc.BindFlags = 0;

				// �T�C�Y
				auto nVerts = desc.ByteWidth / part->vertexStride;

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]���쐬
				ID3D11Buffer* buf;
				ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &buf));

				// [�@GPU��̃o�b�t�@]��[�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�փR�s�[
				deviceContext->CopyResource(buf, part->vertexBuffer.Get());

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�̃������A�h���X�̃}�b�v���J��
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				deviceContext->Map(buf, 0, D3D11_MAP_READ, 0, &mappedResource);

				// [�BCPU��̃o�b�t�@]
				decltype(spart->vertices) verts(nVerts);
				auto* pDstVerts = &verts[0];

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]����[�BCPU��̃o�b�t�@]�֓]��
				auto* pSrcVerts = static_cast<char*>(mappedResource.pData);
				for (size_t i = 0; i < nVerts; i++)
					CopyMemory(&pDstVerts[i], &pSrcVerts[part->vertexOffset + part->vertexStride * i], part->vertexStride);

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�̃������A�h���X�̃}�b�v�����
				deviceContext->Unmap(buf, 0);

				// [�AGPU���CPU�ǂݍ��݉\�o�b�t�@]�����
				buf->Release();

				// �p�[�c�̒��_��[�BCPU��̃o�b�t�@]��ݒ�
				spart->vertices = std::move(verts);
			}

			// ���b�V��>�p�[�c�ǉ�
			smesh->meshParts.emplace_back(std::move(spart));
		}

		// ���f��>���b�V���ǉ�
		smodel->meshes.emplace_back(std::move(smesh));
	}

	return smodel;
}

// ���_�f�[�^����DirectXTK���f�����\�z
std::unique_ptr<Model> SoftModelConverter::ToModel(ID3D11Device* device, std::unique_ptr<SoftModel>& smodel)
{
	// �����`�F�b�N
	if (!device || !smodel)
		throw std::exception("Device and softModel cannot be null");

	// ���f��
	auto model = std::make_unique<Model>();

	// ���f��>�p�����[�^
	model->name = smodel->name;

	// ���f��>���b�V������
	for (auto& smesh : smodel->meshes)
	{
		// ���b�V��
		assert(smesh != nullptr);
		auto mesh = std::make_shared<ModelMesh>();

		// ���b�V��>�p�����[�^
		mesh->boundingSphere = smesh->boundingSphere;
		mesh->boundingBox = smesh->boundingBox;
		mesh->name = smesh->name;
		mesh->ccw = smesh->ccw;
		mesh->pmalpha = smesh->pmalpha;

		// ���b�V��>�p�[�c����
		for (auto& spart : smesh->meshParts)
		{
			// �p�[�c
			assert(spart != nullptr);
			auto part = std::make_unique<ModelMeshPart>();

			// �p�[�c>�p�����[�^
			part->vertexStride = spart->vertexStride;
			part->primitiveType = spart->primitiveType;
			part->indexFormat = spart->indexFormat;
			part->inputLayout = spart->inputLayout;
			part->effect = spart->effect;
			part->vbDecl = spart->vbDecl;
			part->isAlpha = spart->isAlpha;

			// �p�[�c>�C���f�b�N�X
			{
				// �T�C�Y
				auto nIndexes = spart->indices.size();
				auto sizeInBytes = uint32_t(sizeof(uint16_t) * nIndexes);

				// [�@GPU��̃o�b�t�@]���쐬���邽�߂̐ݒ�
				D3D11_BUFFER_DESC desc = {};
				desc.ByteWidth = sizeInBytes;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

				// [�BCPU��̃o�b�t�@]
				auto* pDstInds = &spart->indices[0];

				// [�BCPU��̃o�b�t�@]��[�@GPU��̃o�b�t�@]�����������邽�߂̐ݒ�
				D3D11_SUBRESOURCE_DATA initData = {};
				initData.pSysMem = pDstInds;

				// [�@GPU��̃o�b�t�@]���쐬
				ComPtr<ID3D11Buffer> buf;
				ThrowIfFailed(device->CreateBuffer(&desc, &initData, &buf));

				// �p�[�c�̃o�b�t�@��[�@GPU��̃o�b�t�@]��ݒ�
				part->startIndex = 0;
				part->indexCount = static_cast<uint32_t>(nIndexes);
				part->indexBuffer = std::move(buf);
			}

			// �p�[�c>���_
			{
				// �T�C�Y
				auto nVerts = spart->vertices.size();
				auto sizeInBytes = uint32_t(spart->vertexStride * nVerts);

				// [�@GPU��̃o�b�t�@]���쐬���邽�߂̐ݒ�
				D3D11_BUFFER_DESC desc = {};
				desc.ByteWidth = sizeInBytes;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

				// [�BCPU��̃o�b�t�@]
				std::vector<char> verts(sizeInBytes);
				auto* pDstVerts = &verts[0];

				// [�BCPU��̃o�b�t�@]�̏�����
				auto* pSrcVerts = &spart->vertices[0];
				for (size_t i = 0; i < nVerts; i++)
					CopyMemory(&pDstVerts[spart->vertexStride * i], &pSrcVerts[i], spart->vertexStride);

				// [�BCPU��̃o�b�t�@]��[�@GPU��̃o�b�t�@]�����������邽�߂̐ݒ�
				D3D11_SUBRESOURCE_DATA initData = {};
				initData.pSysMem = pDstVerts;

				// [�@GPU��̃o�b�t�@]���쐬
				ComPtr<ID3D11Buffer> buf;
				ThrowIfFailed(device->CreateBuffer(&desc, &initData, &buf));

				// �p�[�c�̃o�b�t�@��[�@GPU��̃o�b�t�@]��ݒ�
				part->vertexOffset = 0;
				part->vertexBuffer = std::move(buf);
			}

			// ���b�V��>�p�[�c�ǉ�
			mesh->meshParts.emplace_back(std::move(part));
		}

		// ���f��>���b�V���ǉ�
		model->meshes.emplace_back(std::move(mesh));
	}

	return model;
}

// �C���f�b�N�X����Q�Ƃ��Ȃ����_���폜
void SoftModelConverter::RemoveUnreferencedVertices(std::unique_ptr<SoftModel>& smodel)
{
	// ���f��>���b�V������
	for (auto& smesh : smodel->meshes)
	{
		// ���b�V��>�p�[�c����
		for (auto& spart : smesh->meshParts)
		{
			// ���_
			auto& verts = spart->vertices;
			auto& inds = spart->indices;
			// �g���Ă��钸�_�̃C���f�b�N�X
			std::unordered_set<uint16_t> usedVerticesIndex(inds.begin(), inds.end());
			// �Ĕz�u�O�ƍĔz�u��̃C���f�b�N�X�Ή��\ (�v�f�ԍ�: �Ĕz�u�O, �l: �Ĕz�u��)
			std::vector<uint16_t> verticesIndexConvertMap(verts.size(), -1);
			// ���̍Ĕz�u��̃C���f�b�N�X
			uint16_t indsCount = 0;
			// �g���Ă��Ȃ����_���폜
			verts.erase(std::remove_if(verts.begin(), verts.end(), [&](auto& value) {
				// �Ĕz�u�O�̃C���f�b�N�X
				uint16_t i = static_cast<uint16_t>(&value - &*spart->vertices.begin());
				// �g���Ă��邩�ǂ���
				bool contains = usedVerticesIndex.find(i) != usedVerticesIndex.end();
				// �g���Ă�����C���f�b�N�X�����蓖�āA1���₷
				if (contains) verticesIndexConvertMap[i] = indsCount++;
				// �g���Ă��Ȃ����������
				return !contains;
			}), verts.end());
			// �C���f�b�N�X���Ċ��蓖��
			for (auto& value : inds) {
				// �Ή��\����Ċ��蓖��
				value = verticesIndexConvertMap[value];
			}
		}
	}
}

// �|���S���̖ʂ̏���(���v���/�����v���)��ϊ�����
void SoftModelConverter::ConvertPolygonFaces(std::unique_ptr<SoftModel>& smodel, bool ccw)
{
	// ���f��>���b�V������
	for (auto& smesh : smodel->meshes)
	{
		// �|���S���̖ʂ̏��Ԃ��Ⴄ�ꍇ�̂ݕϊ�����
		if (smesh->ccw != ccw)
		{
			// ���b�V��>�p�[�c����
			for (auto& spart : smesh->meshParts)
			{
				// ���_
				auto& inds = spart->indices;
				size_t size = inds.size();
				// �O�p�`ABC�̓_B�Ɠ_C�����ւ���
				for (size_t i = 0; i < size; i += 3)
					std::swap(inds[i + 1], inds[i + 2]);
			}
			// �ݒ��ύX
			smesh->ccw = ccw;
		}
	}
}
