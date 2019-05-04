#pragma once

namespace CSG
{
	struct CSGModel
	{
		std::vector<DirectX::VertexPositionNormalTangentColorTextureSkinning> vertices;
		std::vector<uint16_t> indices;
	};

	CSGModel Union(const CSGModel& a, const CSGModel& b);
	CSGModel Intersection(const CSGModel& a, const CSGModel& b);
	CSGModel Difference(const CSGModel& a, const CSGModel& b);
}

