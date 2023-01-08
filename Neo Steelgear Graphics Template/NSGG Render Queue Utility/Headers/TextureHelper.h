#pragma once

#include <Renderer.h>

CategoryResourceIdentifier LoadTextureStandard(const std::string& filePath,
	Renderer<2>& renderer, const CategoryIdentifier& categoryIdentifier,
	bool generateMipMaps = true);

CategoryResourceIdentifier LoadTextureHDR(const std::string& filePath,
	Renderer<2>& renderer, const CategoryIdentifier& categoryIdentifier,
	bool generateMipMaps = true);

CategoryResourceIdentifier LoadTextureCubeDDS(const std::string& filePath,
	Renderer<2>& renderer, const CategoryIdentifier& categoryIdentifier);