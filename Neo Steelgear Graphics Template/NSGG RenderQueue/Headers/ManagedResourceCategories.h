#pragma once

#include <memory>

#include <d3d12.h>

#include <HeapAllocatorGPU.h>
#include <MultiHeapAllocatorGPU.h>
#include <FrameBufferComponent.h>
#include <FrameTexture2DComponent.h>
#include <ResourceUploader.h>
#include <FrameObject.h>

#include "CategoryIdentifiers.h"
#include "ManagedDescriptorHeap.h"

struct UploaderSettings
{
	size_t heapSize = 0;
	AllocationStrategy allocationStrategy = AllocationStrategy::FIRST_FIT;
};

struct CategoryResourceHandle
{
	ID3D12Resource* resource = nullptr;
	size_t offset = size_t(-1);
	size_t nrOfElements = 0;
};

struct ResourceCategoriesSettings
{
	std::shared_ptr<HeapAllocatorGPU> defaultStaticBufferAllocator = nullptr;
	std::shared_ptr<HeapAllocatorGPU> defaultDynamicBufferAllocator = nullptr;
	std::shared_ptr<HeapAllocatorGPU> defaultStaticTexture2DAllocator = nullptr;
	std::shared_ptr<HeapAllocatorGPU> defaultDynamicTexture2DAllocator = nullptr;

	UploaderSettings staticResourcesUploadSettings;
	UploaderSettings dynamicResourcesUploadSettings;
};

template<FrameType Frames>
class ManagedResourceCategories : FrameBased<Frames>
{
private:
	typedef ResourceComponent ResourceCategory;

	ID3D12Device* device;

	std::shared_ptr<HeapAllocatorGPU> staticBufferAllocator;
	std::vector<FrameBufferComponent<1>> staticBufferCategories;

	std::shared_ptr<HeapAllocatorGPU> dynamicBufferAllocator;
	std::vector<FrameBufferComponent<Frames>> dynamicBufferCategories;

	std::shared_ptr<HeapAllocatorGPU> staticTexture2DAllocator;
	std::vector<FrameTexture2DComponent<1>> staticTexture2DCategories;

	std::shared_ptr<HeapAllocatorGPU> dynamicTexture2DAllocator;
	std::vector<FrameTexture2DComponent<Frames>> dynamicTexture2DCategories;

	FrameObject<ResourceUploader, Frames> staticResourcesUploader;
	FrameObject<ResourceUploader, Frames> dynamicResourcesUploader;

	DescriptorAllocationInfo<BufferViewDesc> CreateDefaultBufferDAI(
		ViewType viewType, size_t nrOfDescriptors);
	DescriptorAllocationInfo<Texture2DViewDesc> CreateDefaultTexture2DDAI(
		ViewType viewType, size_t nrOfDescriptors);

	void UpdateDescriptorHeapHelper(bool dynamic, CategoryType CategoryType,
		size_t localIndex, ResourceCategory& Category,
		ManagedDescriptorHeap<Frames>& descriptorHeap);

public:
	ManagedResourceCategories() = default;
	~ManagedResourceCategories() = default;
	ManagedResourceCategories(const ManagedResourceCategories& other) = delete;
	ManagedResourceCategories& operator=(const ManagedResourceCategories& other) = delete;
	ManagedResourceCategories(ManagedResourceCategories&& other) = default;
	ManagedResourceCategories& operator=(ManagedResourceCategories&& other) = default;

	void Initialize(ID3D12Device* deviceToUse,
		const ResourceCategoriesSettings& heapSettings);

	template<typename T>
	CategoryIdentifier CreateBufferCategory(UpdateType categoryUpdateType,
		size_t initialMinimumHeapSize, size_t expansionMinimumSize,
		size_t nrOfStartingDescriptors, bool cbv, bool srv, bool uav);
	CategoryIdentifier CreateBufferCategory(UpdateType categoryUpdateType,
		BufferComponentInfo categoryInfo, size_t nrOfStartingDescriptors,
		bool cbv, bool srv, bool uav);
	CategoryIdentifier CreateTexture2DCategory(UpdateType categoryUpdateType,
		TextureComponentInfo categoryInfo, size_t nrOfStartingDescriptors,
		bool srv, bool uav, bool rtv, bool dsv);

	CategoryResourceIdentifier CreateBuffer(const CategoryIdentifier& category,
		size_t nrOfElements,
		const BufferReplacementViews& replacementViews = BufferReplacementViews());
	CategoryResourceIdentifier CreateTexture2D(const CategoryIdentifier& category,
		size_t width, size_t height, size_t arraySize = 1, size_t mipLevels = 1, 
		std::uint8_t sampleCount = 1, std::uint8_t sampleQuality = 0, 
		D3D12_CLEAR_VALUE* optimalClearValue = nullptr,
		const Texture2DComponentTemplate::TextureReplacementViews& replacementViews = Texture2DComponentTemplate::TextureReplacementViews());

	void RemoveResource(const CategoryResourceIdentifier& identifier);

	void SetResourceData(const CategoryResourceIdentifier& identifier, 
		void* dataAddress, std::uint8_t subresourceIndex = 0);

	void TransitionCategoryState(const CategoryIdentifier& identifier,
		std::vector<D3D12_RESOURCE_BARRIER>& barriers, D3D12_RESOURCE_STATES neededState,
		std::optional<D3D12_RESOURCE_STATES> assumedInitialState = std::nullopt);

	CategoryResourceHandle GetResourceHandle(const CategoryResourceIdentifier& identifier) const;

	void UpdateDescriptorHeap(ManagedDescriptorHeap<Frames>& descriptorHeap);
	void ActivateNewCategories(ID3D12GraphicsCommandList* list);
	void UpdateCategories(ID3D12GraphicsCommandList* list);

	void SwapFrame() override;
};

template<FrameType Frames>
inline DescriptorAllocationInfo<BufferViewDesc>
ManagedResourceCategories<Frames>::CreateDefaultBufferDAI(
	ViewType viewType, size_t nrOfDescriptors)
{
	DescriptorAllocationInfo<BufferViewDesc> toReturn(viewType,
		BufferViewDesc(viewType), nrOfDescriptors);
	return toReturn;
}

template<FrameType Frames>
inline DescriptorAllocationInfo<Texture2DViewDesc> 
ManagedResourceCategories<Frames>::CreateDefaultTexture2DDAI(
	ViewType viewType, size_t nrOfDescriptors)
{
	DescriptorAllocationInfo<Texture2DViewDesc> toReturn(viewType,
		Texture2DViewDesc(viewType), nrOfDescriptors);
	return toReturn;
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::UpdateDescriptorHeapHelper(
	bool dynamic, CategoryType categoryType, size_t localIndex,
	ResourceCategory& category, ManagedDescriptorHeap<Frames>& descriptorHeap)
{
	CategoryIdentifier identifier;
	identifier.type = categoryType;
	identifier.localIndex = localIndex;
	identifier.dynamicCategory = dynamic;

	descriptorHeap.AddCategoryDescriptors(identifier, category);
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::Initialize(ID3D12Device* deviceToUse,
	const ResourceCategoriesSettings& heapSettings)
{
	device = deviceToUse;

	std::shared_ptr<MultiHeapAllocatorGPU> defaultAllocator(new MultiHeapAllocatorGPU());
	defaultAllocator->Initialize(device);

	staticBufferAllocator = heapSettings.defaultStaticBufferAllocator != nullptr ?
		heapSettings.defaultStaticBufferAllocator : defaultAllocator;
	dynamicBufferAllocator = heapSettings.defaultDynamicBufferAllocator != nullptr ?
		heapSettings.defaultDynamicBufferAllocator : defaultAllocator;
	staticTexture2DAllocator = heapSettings.defaultStaticTexture2DAllocator != nullptr ?
		heapSettings.defaultStaticTexture2DAllocator : defaultAllocator;
	dynamicTexture2DAllocator = heapSettings.defaultDynamicTexture2DAllocator != nullptr ?
		heapSettings.defaultDynamicTexture2DAllocator : defaultAllocator;

	staticResourcesUploader.Initialize(&ResourceUploader::Initialize, device,
		heapSettings.staticResourcesUploadSettings.heapSize,
		heapSettings.staticResourcesUploadSettings.allocationStrategy);
	dynamicResourcesUploader.Initialize(&ResourceUploader::Initialize, device,
		heapSettings.dynamicResourcesUploadSettings.heapSize,
		heapSettings.dynamicResourcesUploadSettings.allocationStrategy);
}

template<FrameType Frames>
template<typename T>
inline CategoryIdentifier ManagedResourceCategories<Frames>::CreateBufferCategory(
	UpdateType categoryUpdateType, size_t initialMinimumHeapSize,
	size_t expansionMinimumSize, size_t nrOfStartingDescriptors,
	bool cbv, bool srv, bool uav)
{
	BufferComponentInfo componentInfo;
	componentInfo.mappedResource = categoryUpdateType == UpdateType::MAP_UPDATE;
	componentInfo.memoryInfo.initialMinimumHeapSize = initialMinimumHeapSize;
	componentInfo.memoryInfo.expansionMinimumSize = expansionMinimumSize;
	componentInfo.bufferInfo.alignment = alignof(T);
	componentInfo.bufferInfo.elementSize = sizeof(T);

	return CreateBufferCategory(categoryUpdateType, componentInfo, 
		nrOfStartingDescriptors, cbv, srv, uav);
}

template<FrameType Frames>
inline CategoryIdentifier ManagedResourceCategories<Frames>::CreateBufferCategory(
	UpdateType categoryUpdateType, BufferComponentInfo categoryInfo,
	size_t nrOfStartingDescriptors, bool cbv, bool srv, bool uav)
{
	CategoryIdentifier toReturn;
	bool dynamic = categoryUpdateType == UpdateType::COPY_UPDATE ||
		categoryUpdateType == UpdateType::MAP_UPDATE;
	toReturn.dynamicCategory = dynamic;
	toReturn.type = CategoryType::BUFFER;
	std::vector<DescriptorAllocationInfo<BufferViewDesc>> dai;

	if (cbv)
		dai.push_back(CreateDefaultBufferDAI(ViewType::CBV, nrOfStartingDescriptors));
	if (srv)
		dai.push_back(CreateDefaultBufferDAI(ViewType::SRV, nrOfStartingDescriptors));
	if (uav)
		dai.push_back(CreateDefaultBufferDAI(ViewType::UAV, nrOfStartingDescriptors));

	if (dynamic)
	{
		FrameBufferComponent<Frames> toAdd;

		if (categoryInfo.memoryInfo.heapAllocator == nullptr)
			categoryInfo.memoryInfo.heapAllocator = dynamicBufferAllocator.get();

		toAdd.Initialize(device, categoryUpdateType, categoryInfo, dai);
		dynamicBufferCategories.push_back(std::move(toAdd));
		toReturn.localIndex = dynamicBufferCategories.size() - 1;
	}
	else
	{
		FrameBufferComponent<1> toAdd;

		if (categoryInfo.memoryInfo.heapAllocator == nullptr)
			categoryInfo.memoryInfo.heapAllocator = staticBufferAllocator.get();

		toAdd.Initialize(device, categoryUpdateType, categoryInfo, dai);
		staticBufferCategories.push_back(std::move(toAdd));
		toReturn.localIndex = staticBufferCategories.size() - 1;
	}

	return toReturn;
}

template<FrameType Frames>
inline CategoryIdentifier ManagedResourceCategories<Frames>::CreateTexture2DCategory(
	UpdateType categoryUpdateType, TextureComponentInfo categoryInfo,
	size_t nrOfStartingDescriptors, bool srv, bool uav, bool rtv, bool dsv)
{
	CategoryIdentifier toReturn;
	bool dynamic = categoryUpdateType == UpdateType::COPY_UPDATE ||
		categoryUpdateType == UpdateType::MAP_UPDATE;
	toReturn.dynamicCategory = dynamic;
	toReturn.type = CategoryType::TEXTURE2D;
	std::vector<DescriptorAllocationInfo<Texture2DViewDesc>> dai;

	if (srv)
		dai.push_back(CreateDefaultTexture2DDAI(ViewType::SRV, nrOfStartingDescriptors));
	if (uav)
		dai.push_back(CreateDefaultTexture2DDAI(ViewType::UAV, nrOfStartingDescriptors));
	if (rtv)
		dai.push_back(CreateDefaultTexture2DDAI(ViewType::RTV, nrOfStartingDescriptors));
	if (dsv)
		dai.push_back(CreateDefaultTexture2DDAI(ViewType::DSV, nrOfStartingDescriptors));

	if (dynamic)
	{
		FrameTexture2DComponent<Frames> toAdd;

		if (categoryInfo.memoryInfo.heapAllocator == nullptr)
			categoryInfo.memoryInfo.heapAllocator = dynamicTexture2DAllocator.get();

		toAdd.Initialize(device, categoryUpdateType, categoryInfo, dai);
		dynamicTexture2DCategories.push_back(std::move(toAdd));
		toReturn.localIndex = dynamicTexture2DCategories.size() - 1;
	}
	else
	{
		FrameTexture2DComponent<1> toAdd;

		if (categoryInfo.memoryInfo.heapAllocator == nullptr)
			categoryInfo.memoryInfo.heapAllocator = staticTexture2DAllocator.get();

		toAdd.Initialize(device, categoryUpdateType, categoryInfo, dai);
		staticTexture2DCategories.push_back(std::move(toAdd));
		toReturn.localIndex = staticTexture2DCategories.size() - 1;
	}

	return toReturn;
}

template<FrameType Frames>
inline CategoryResourceIdentifier ManagedResourceCategories<Frames>::CreateBuffer(
	const CategoryIdentifier& category, size_t nrOfElements,
	const BufferReplacementViews& replacementViews)
{
	ResourceIndex internalIndex;

	if (category.dynamicCategory == true)
	{
		internalIndex = dynamicBufferCategories[category.localIndex].CreateBuffer(
			nrOfElements, replacementViews);
	}
	else
	{
		internalIndex = staticBufferCategories[category.localIndex].CreateBuffer(
			nrOfElements, replacementViews);
	}

	return { category, internalIndex };
}

template<FrameType Frames>
inline CategoryResourceIdentifier ManagedResourceCategories<Frames>::CreateTexture2D(
	const CategoryIdentifier& category, size_t width, size_t height,
	size_t arraySize, size_t mipLevels, std::uint8_t sampleCount,
	std::uint8_t sampleQuality, D3D12_CLEAR_VALUE* optimalClearValue,
	const Texture2DComponentTemplate::TextureReplacementViews& replacementViews)
{
	ResourceIndex internalIndex;

	if (category.dynamicCategory == true)
	{
		internalIndex = dynamicTexture2DCategories[category.localIndex].CreateTexture(
			width, height, arraySize, mipLevels, sampleCount, sampleQuality,
			optimalClearValue, replacementViews);
	}
	else
	{
		internalIndex = staticTexture2DCategories[category.localIndex].CreateTexture(
			width, height, arraySize, mipLevels, sampleCount, sampleQuality,
			optimalClearValue, replacementViews);
	}

	return { category, internalIndex };
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::RemoveResource(
	const CategoryResourceIdentifier& identifier)
{
	size_t localIndex = identifier.categoryIdentifier.localIndex;
	ResourceIndex internalIndex = identifier.internalIndex;

	switch (identifier.categoryIdentifier.type)
	{
	case CategoryType::BUFFER:
		if (identifier.categoryIdentifier.dynamicCategory == true)
		{
			dynamicBufferCategories[localIndex].RemoveComponent(internalIndex);
		}
		else
		{
			staticBufferCategories[localIndex].RemoveComponent(internalIndex);
		}
		break;
	case CategoryType::TEXTURE2D:
		if (identifier.categoryIdentifier.dynamicCategory == true)
		{
			dynamicTexture2DCategories[localIndex].RemoveComponent(internalIndex);
		}
		else
		{
			staticTexture2DCategories[localIndex].RemoveComponent(internalIndex);
		}
		break;
	default:
		throw std::runtime_error("Unknown category type when removing resource");
		break;
	}
}

template<FrameType Frames>
void ManagedResourceCategories<Frames>::SetResourceData(
	const CategoryResourceIdentifier& identifier, void* dataAddress,
	std::uint8_t subresourceIndex)
{
	size_t localIndex = identifier.categoryIdentifier.localIndex;
	ResourceIndex internalIndex = identifier.internalIndex;

	switch (identifier.categoryIdentifier.type)
	{
	case CategoryType::BUFFER:
		if (identifier.categoryIdentifier.dynamicCategory == true)
		{
			dynamicBufferCategories[localIndex].SetUpdateData(internalIndex,
				dataAddress);
		}
		else
		{
			staticBufferCategories[localIndex].SetUpdateData(internalIndex,
				dataAddress);
		}
		break;
	case CategoryType::TEXTURE2D:
		if (identifier.categoryIdentifier.dynamicCategory == true)
		{
			dynamicTexture2DCategories[localIndex].SetUpdateData(internalIndex,
				dataAddress, subresourceIndex);
		}
		else
		{
			staticTexture2DCategories[localIndex].SetUpdateData(internalIndex,
				dataAddress, subresourceIndex);
		}
		break;
	default:
		throw std::runtime_error("Unknown category type when setting resource data");
		break;
	}
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::TransitionCategoryState(
	const CategoryIdentifier& identifier, std::vector<D3D12_RESOURCE_BARRIER>& barriers,
	D3D12_RESOURCE_STATES neededState, std::optional<D3D12_RESOURCE_STATES> assumedInitialState)
{
	switch (identifier.type)
	{
	case CategoryType::BUFFER:
		if (identifier.dynamicCategory == true)
		{
			dynamicBufferCategories[identifier.localIndex].ChangeToState(
				barriers, neededState, assumedInitialState);
		}
		else
		{
			staticBufferCategories[identifier.localIndex].ChangeToState(
				barriers, neededState, assumedInitialState);
		}
	case CategoryType::TEXTURE2D:
		if (identifier.dynamicCategory == true)
		{
			dynamicTexture2DCategories[identifier.localIndex].TransitionAllTextures(
				barriers, neededState, assumedInitialState);
		}
		else
		{
			staticTexture2DCategories[identifier.localIndex].TransitionAllTextures(
				barriers, neededState, assumedInitialState);
		}
		break;
	default:
		throw std::runtime_error("Attempting to transition Category of unknown type");
		break;
	}
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::SwapFrame()
{
	for (auto& category : staticBufferCategories)
		category.SwapFrame();

	for (auto& category : dynamicBufferCategories)
		category.SwapFrame();

	for (auto& category : staticTexture2DCategories)
		category.SwapFrame();

	for (auto& category : dynamicTexture2DCategories)
		category.SwapFrame();

	staticResourcesUploader.SwapFrame();
	staticResourcesUploader.Active().RestoreUsedMemory();
	dynamicResourcesUploader.SwapFrame();
	dynamicResourcesUploader.Active().RestoreUsedMemory();
}

template<FrameType Frames>
inline CategoryResourceHandle ManagedResourceCategories<Frames>::GetResourceHandle(
	const CategoryResourceIdentifier& identifier) const
{
	CategoryResourceHandle toReturn;

	switch (identifier.categoryIdentifier.type)
	{
	case CategoryType::BUFFER:
		if (identifier.categoryIdentifier.dynamicCategory == true)
		{
			BufferHandle internalHandle =
				dynamicBufferCategories[identifier.categoryIdentifier.localIndex].GetBufferHandle(
					identifier.internalIndex);
			toReturn.resource = internalHandle.resource;
			toReturn.offset = internalHandle.startOffset;
			toReturn.nrOfElements = internalHandle.nrOfElements;
		}
		else
		{
			BufferHandle internalHandle =
				 staticBufferCategories[identifier.categoryIdentifier.localIndex].GetBufferHandle(
					identifier.internalIndex);
			toReturn.resource = internalHandle.resource;
			toReturn.offset = internalHandle.startOffset;
			toReturn.nrOfElements = internalHandle.nrOfElements;
		}
		break;
	case CategoryType::TEXTURE2D:
		if (identifier.categoryIdentifier.dynamicCategory == true)
		{
			TextureHandle internalHandle = 
				dynamicTexture2DCategories[identifier.categoryIdentifier.localIndex].GetTextureHandle(
					identifier.internalIndex);
			toReturn.resource = internalHandle.resource;
			toReturn.offset = 0;
			toReturn.nrOfElements = 1;
		}
		else
		{
			TextureHandle internalHandle =
				staticTexture2DCategories[identifier.categoryIdentifier.localIndex].GetTextureHandle(
					identifier.internalIndex);
			toReturn.resource = internalHandle.resource;
			toReturn.offset = 0;
			toReturn.nrOfElements = 1;
		}
		break;
	default:
		throw std::runtime_error("Unknown category type when getting resource handle");
		break;
	}

	return toReturn;
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::UpdateDescriptorHeap(
	ManagedDescriptorHeap<Frames>& descriptorHeap)
{
	for (size_t i = 0; i < staticBufferCategories.size(); ++i)
	{
		UpdateDescriptorHeapHelper(false, CategoryType::BUFFER,
			i, staticBufferCategories[i], descriptorHeap);
	}

	for (size_t i = 0; i < dynamicBufferCategories.size(); ++i)
	{
		UpdateDescriptorHeapHelper(true, CategoryType::BUFFER,
			i, dynamicBufferCategories[i], descriptorHeap);
	}

	for (size_t i = 0; i < staticTexture2DCategories.size(); ++i)
	{
		UpdateDescriptorHeapHelper(false, CategoryType::TEXTURE2D,
			i, staticTexture2DCategories[i], descriptorHeap);
	}

	for (size_t i = 0; i < dynamicTexture2DCategories.size(); ++i)
	{
		UpdateDescriptorHeapHelper(true, CategoryType::TEXTURE2D,
			i, dynamicTexture2DCategories[i], descriptorHeap);
	}
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::ActivateNewCategories(
	ID3D12GraphicsCommandList* list)
{
	static std::vector<D3D12_RESOURCE_BARRIER> barriers;

	for (auto& Category : staticBufferCategories)
		Category.GetInitializationBarriers(barriers);

	for (auto& Category : dynamicBufferCategories)
		Category.GetInitializationBarriers(barriers);

	for (auto& Category : staticTexture2DCategories)
		Category.GetInitializationBarriers(barriers);

	for (auto& Category : dynamicTexture2DCategories)
		Category.GetInitializationBarriers(barriers);

	if (barriers.size() != 0)
	{
		list->ResourceBarrier(barriers.size(), barriers.data());
		barriers.clear();
	}
}

template<FrameType Frames>
inline void ManagedResourceCategories<Frames>::UpdateCategories(
	ID3D12GraphicsCommandList* list)
{
	for (auto& category : staticBufferCategories)
		category.PerformUpdates(list, staticResourcesUploader.Active());

	for (auto& category : dynamicBufferCategories)
		category.PerformUpdates(list, dynamicResourcesUploader.Active());

	for (auto& category : staticTexture2DCategories)
		category.PerformUpdates(list, staticResourcesUploader.Active());

	for (auto& category : dynamicTexture2DCategories)
		category.PerformUpdates(list, dynamicResourcesUploader.Active());
}