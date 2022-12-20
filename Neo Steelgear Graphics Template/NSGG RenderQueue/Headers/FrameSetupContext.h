#pragma once

#include <vector>
#include <optional>

#include <d3d12.h>

#include <FrameBased.h>
#include <DescriptorAllocator.h>

#include "TransientResourceDesc.h"
#include "LocalResourceDesc.h"
#include "Blackboard.h"
#include "ResourceIdentifiers.h"

enum class ShaderBindableDescriptorType
{
	SHADER_RESOURCE,
	UNORDERED_ACCESS
};

struct ShaderBindableDescriptorDesc
{
	ShaderBindableDescriptorType type;

	union Description
	{
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srv = std::nullopt;
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uav;
	};

	Description desc;
};

template<typename T>
struct DescriptorRequest
{
	T info;
	TransientResourceIndex index;

	DescriptorRequest(const TransientResourceIndex& index) : index(index)
	{
		// EMPTY
	}
};

typedef size_t LocalResourceIndex;

class FrameSetupContext
{
private:
	template<FrameType> friend class RenderQueue;

	std::vector<DescriptorRequest<ShaderBindableDescriptorDesc>> shaderBindableRequests;
	std::vector<DescriptorRequest<std::optional<D3D12_RENDER_TARGET_VIEW_DESC>>> rtvRequests;
	std::vector<DescriptorRequest<std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>>> dsvRequests;

	std::vector<TransientResourceDesc> transientResourceDescs;
	std::vector<LocalResourceDesc> localResourceDescs;
	size_t totalLocalMemoryNeeded = 0;

	template<FrameType Frames>
	void CreateTransientDescriptors(Blackboard<Frames>& blackboard);

	void Reset(size_t nrOfTransientResources);

public:
	FrameSetupContext() = default;
	~FrameSetupContext() = default;
	FrameSetupContext(const FrameSetupContext& other) = delete;
	FrameSetupContext& operator=(const FrameSetupContext& other) = delete;
	FrameSetupContext(FrameSetupContext&& other) = default;
	FrameSetupContext& operator=(FrameSetupContext&& other) = default;

	void SetTransientResourceDesc(const TransientResourceIndex& index, const TransientResourceDesc& desc);
	const TransientResourceDesc& GetTransientResourceDesc(const TransientResourceIndex& index);

	LocalResourceIndex CreateLocalResource(const LocalResourceDesc& desc);

	ViewIdentifier RequestTransientShaderBindable(const DescriptorRequest<ShaderBindableDescriptorDesc>& request);
	ViewIdentifier RequestTransientRTV(const DescriptorRequest<std::optional<D3D12_RENDER_TARGET_VIEW_DESC>>& request);
	ViewIdentifier RequestTransientDSV(const DescriptorRequest<std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>>& request);
};

template<FrameType Frames>
void FrameSetupContext::CreateTransientDescriptors(Blackboard<Frames>& blackboard)
{
	for (const auto& description : shaderBindableRequests)
	{
		switch (description.info.type)
		{
		case ShaderBindableDescriptorType::SHADER_RESOURCE:
			blackboard.CreateSRV(description.index, description.info.desc.srv);
			break;
		case ShaderBindableDescriptorType::UNORDERED_ACCESS:
			blackboard.CreateUAV(description.index, description.info.desc.uav);
			break;
		default:
			throw std::runtime_error("Unknown shader bindable view type");
			break;
		}
	}

	for (const auto& description : rtvRequests)
	{
		blackboard.CreateRTV(description.index, description.info);
	}

	for (const auto& description : dsvRequests)
	{
		blackboard.CreateDSV(description.index, description.info);
	}
}