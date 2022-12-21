#pragma once

#include <ResourceComponent.h>

enum class CategoryType
{
	BUFFER,
	//TEXTURE1D,
	TEXTURE2D,
	//TEXTURE3D
};

struct CategoryIdentifier
{
	CategoryType type;
	size_t localIndex = size_t(-1);
	bool dynamicCategory = true;

	bool operator==(const CategoryIdentifier& other) const
	{
		return this->type == other.type && this->localIndex == other.localIndex &&
			this->dynamicCategory == other.dynamicCategory;
	}
};

namespace std
{
	template <>
	struct hash<CategoryIdentifier>
	{
		size_t operator()(const CategoryIdentifier& identifier) const
		{
			return ((hash<CategoryType>()(identifier.type)
				^ (hash<size_t>()(identifier.localIndex) << 1)) >> 1)
				^ (hash<bool>()(identifier.dynamicCategory) << 1);
		}
	};
}

struct CategoryResourceIdentifier
{
	CategoryIdentifier categoryIdentifier;
	ResourceIndex internalIndex;

	bool operator==(const CategoryResourceIdentifier& other) const
	{
		return categoryIdentifier == other.categoryIdentifier &&
			internalIndex.allocatorIdentifier.heapChunkIndex ==
			other.internalIndex.allocatorIdentifier.heapChunkIndex &&
			internalIndex.allocatorIdentifier.internalIndex ==
			other.internalIndex.allocatorIdentifier.internalIndex &&
			internalIndex.descriptorIndex == other.internalIndex.descriptorIndex;
		// Temporary implementation, can be made default once the internal types have op== implemented
	}
};