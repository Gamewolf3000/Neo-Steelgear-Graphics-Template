#pragma once

#include "ManagedDescriptorHeap.h"
#include "CategoryIdentifiers.h"

template<FrameType Frames>
class FramePreparationContext
{
private:
	ManagedDescriptorHeap<Frames>* descriptorHeap = nullptr;

public:
	FramePreparationContext() = default;
	~FramePreparationContext() = default;
	FramePreparationContext(const FramePreparationContext& other) = delete;
	FramePreparationContext& operator=(const FramePreparationContext& other) = delete;
	FramePreparationContext(FramePreparationContext&& other) noexcept = default;
	FramePreparationContext& operator=(FramePreparationContext&& other) noexcept = default;

	void Initialize(ManagedDescriptorHeap<Frames>* descriptorHeap);

	unsigned int GetCategoryDescriptorStart(
		const CategoryIdentifier& identifier, ViewType viewType) const;
	unsigned int GetCategoryDescriptorStart(
		const CategoryResourceIdentifier& identifier, ViewType viewType) const;
	unsigned int GetCategoryResourceDescriptor(
		const CategoryResourceIdentifier& identifier, ViewType viewType) const;
};

template<FrameType Frames>
void FramePreparationContext<Frames>::Initialize(
	ManagedDescriptorHeap<Frames>* descriptorHeapToUse)
{
	descriptorHeap = descriptorHeapToUse;
}

template<FrameType Frames>
inline unsigned int FramePreparationContext<Frames>::GetCategoryDescriptorStart(
	const CategoryIdentifier& identifier, ViewType viewType) const
{
	return descriptorHeap->GetCategoryHeapOffset(identifier, viewType);
}

template<FrameType Frames>
inline unsigned int FramePreparationContext<Frames>::GetCategoryDescriptorStart(
	const CategoryResourceIdentifier& identifier, ViewType viewType) const
{
	return descriptorHeap->GetCategoryHeapOffset(identifier.categoryIdentifier,
		viewType);
}

template<FrameType Frames>
inline unsigned int FramePreparationContext<Frames>::GetCategoryResourceDescriptor(
	const CategoryResourceIdentifier& identifier, ViewType viewType) const
{
	size_t toReturn = GetCategoryDescriptorStart(
		identifier.categoryIdentifier, viewType);
	toReturn += identifier.internalIndex.descriptorIndex;

	return toReturn;
}