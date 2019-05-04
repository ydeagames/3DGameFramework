#include "CSG.h"
#include "csgjs.h"

namespace CSG
{
	using namespace csgjs;
	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	namespace
	{
		namespace D2C
		{
			csgjs_vector D2C_Vector3(const Vector3& vector)
			{
				return csgjs_vector{
					vector.x,
					vector.y,
					vector.z,
				};
			}

			csgjs_vector D2C_Vector2(const Vector2& vector)
			{
				return csgjs_vector{
					vector.x,
					vector.y,
					0,
				};
			}

			csgjs_vertex D2C_Vertex(const VertexPositionNormalTangentColorTextureSkinning& vertex)
			{
				return csgjs_vertex{
					D2C_Vector3(vertex.position),
					D2C_Vector3(vertex.normal),
					D2C_Vector2(vertex.textureCoordinate),
					static_cast<const void*>(&vertex),
				};
			}

			csgjs_model D2C_Model(const CSGModel& model)
			{
				csgjs_model cmodel = {};
				for (auto& vertex : model.vertices)
					cmodel.vertices.push_back(D2C_Vertex(vertex));
				for (auto& index : model.indices)
					cmodel.indices.push_back(static_cast<int>(index));
				return cmodel;
			}
		}

		namespace C2D
		{
			Vector3 C2D_Vector3(const csgjs_vector& cvector)
			{
				return Vector3{
					cvector.x,
					cvector.y,
					cvector.z,
				};
			}

			Vector2 C2D_Vector2(const csgjs_vector& cvector)
			{
				return Vector2{
					cvector.x,
					cvector.y,
				};
			}

			VertexPositionNormalTangentColorTextureSkinning C2D_Vertex(const csgjs_vertex& cvertex)
			{
				VertexPositionNormalTangentColorTextureSkinning vertex = {};
				if (cvertex.binding != nullptr)
					vertex = *static_cast<const VertexPositionNormalTangentColorTextureSkinning*>(cvertex.binding);
				vertex.position = C2D_Vector3(cvertex.pos);
				vertex.normal = C2D_Vector3(cvertex.normal);
				vertex.textureCoordinate = C2D_Vector2(cvertex.uv);
				return vertex;
			}

			CSGModel C2D_Model(const csgjs_model& cmodel)
			{
				CSGModel model = {};
				for (auto& vertex : cmodel.vertices)
					model.vertices.push_back(C2D_Vertex(vertex));
				for (auto& index : cmodel.indices)
					model.indices.push_back(static_cast<uint16_t>(index));
				return model;
			}
		}
	}

	CSGModel Union(const CSGModel& a, const CSGModel& b)
	{
		return C2D::C2D_Model(csgjs_union(D2C::D2C_Model(a), D2C::D2C_Model(b)));
	}

	CSGModel Intersection(const CSGModel& a, const CSGModel& b)
	{
		return C2D::C2D_Model(csgjs_intersection(D2C::D2C_Model(a), D2C::D2C_Model(b)));
	}

	CSGModel Difference(const CSGModel& a, const CSGModel& b)
	{
		return C2D::C2D_Model(csgjs_difference(D2C::D2C_Model(a), D2C::D2C_Model(b)));
	}
}