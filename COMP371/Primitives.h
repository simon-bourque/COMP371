#pragma once

#include "Types.h"

namespace cube {
	const extern uint32 numVertices;
	extern const float32 vertices[];
	extern const uint32 numIndices;
	extern const uint32 indices[];

	void fill(std::vector<float32>& vertices, std::vector<uint32>& indices);
}