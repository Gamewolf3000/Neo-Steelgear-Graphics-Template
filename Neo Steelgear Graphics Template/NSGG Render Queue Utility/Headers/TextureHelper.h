#pragma once

#include <Renderer.h>

CategoryResourceIdentifier LoadTexture(const std::string& filePath,
	Renderer<2>& renderer, const CategoryIdentifier& categoryIdentifier,
	bool generateMipMaps = true);