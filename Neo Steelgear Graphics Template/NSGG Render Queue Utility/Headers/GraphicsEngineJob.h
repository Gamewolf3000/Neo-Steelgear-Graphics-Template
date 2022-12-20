#pragma once

#include <optional>

#include <QueueContext.h>
#include <QueueJob.h>
#include <FrameSetupContext.h>
#include <ResourceIdentifiers.h>

#include "RootSignature.h"
#include "ConstantBuffer.h"

template<FrameType Frames>
class GraphicsEngineJob : public QueueJob<Frames>
{
protected:
	RootSignature rootSignature;

	struct ShaderBuffer
	{
		LocalResourceIndex resourceIndex = LocalResourceIndex(-1);
		UINT rootParameterIndex = UINT(-1);

		ShaderBuffer() = default;
	};

	template<typename T>
	LocalResourceIndex CreateLocalBuffer(FrameSetupContext& context, 
		size_t nrOfElements, size_t alignment = alignof(T));

	template<typename T>
	ViewIdentifier RequestStructuredBufferSRV(FrameSetupContext& context,
		const TransientResourceIndex& index, UINT16 nrOfElements);
	ViewIdentifier RequestTexture2DSRV(FrameSetupContext& context,
		const TransientResourceIndex& index,
		const std::optional<DXGI_FORMAT>& srvFormat = std::nullopt);
	ViewIdentifier RequestTexture2DUAV(FrameSetupContext& context,
		const TransientResourceIndex& index,
		const std::optional<DXGI_FORMAT>& uavFormat = std::nullopt);

	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressMode,
		UINT shaderRegister, D3D12_SHADER_VISIBILITY shaderVisibility);

public:
	GraphicsEngineJob() = default;
	virtual ~GraphicsEngineJob() = default;
	GraphicsEngineJob(const GraphicsEngineJob& other) = delete;
	GraphicsEngineJob& operator=(const GraphicsEngineJob& other) = delete;
	GraphicsEngineJob(GraphicsEngineJob&& other) = default;
	GraphicsEngineJob& operator=(GraphicsEngineJob&& other) = default;
};

template<FrameType Frames>
template<typename T>
inline LocalResourceIndex GraphicsEngineJob<Frames>::CreateLocalBuffer(
	FrameSetupContext& context, size_t nrOfElements, size_t alignment)
{
	LocalResourceDesc desc(sizeof(T), nrOfElements, alignment);
	return context.CreateLocalResource(desc);
}

template<FrameType Frames>
template<typename T>
inline ViewIdentifier GraphicsEngineJob<Frames>::RequestStructuredBufferSRV(
	FrameSetupContext& context, const TransientResourceIndex& index,
	UINT16 nrOfElements)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = nrOfElements;
	srvDesc.Buffer.StructureByteStride = sizeof(T);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	DescriptorRequest<ShaderBindableDescriptorDesc> request;
	request.index = index;
	request.info.type = ShaderBindableDescriptorType::SHADER_RESOURCE;
	request.info.desc.srv = srvDesc;

	return context.RequestTransientShaderBindable(request);
}

template<FrameType Frames>
inline ViewIdentifier GraphicsEngineJob<Frames>::RequestTexture2DSRV(
	FrameSetupContext& context, const TransientResourceIndex& index,
	const std::optional<DXGI_FORMAT>& srvFormat)
{
	DescriptorRequest<ShaderBindableDescriptorDesc> request;
	request.index = index;
	request.info.type = ShaderBindableDescriptorType::SHADER_RESOURCE;

	if (srvFormat.has_value())
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = *srvFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		request.info.desc.srv = srvDesc;
	}

	return context.RequestTransientShaderBindable(request);
}

template<FrameType Frames>
inline ViewIdentifier GraphicsEngineJob<Frames>::RequestTexture2DUAV(
	FrameSetupContext& context, const TransientResourceIndex& index,
	const std::optional<DXGI_FORMAT>& uavFormat)
{
	DescriptorRequest<ShaderBindableDescriptorDesc> request;
	request.index = index;
	request.info.type = ShaderBindableDescriptorType::UNORDERED_ACCESS;

	if (uavFormat.has_value())
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = *uavFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;

		request.info.desc.uav = uavDesc;
	}

	return context.RequestTransientShaderBindable(request);
}

template<FrameType Frames>
inline void GraphicsEngineJob<Frames>::AddStaticSampler(D3D12_FILTER filter,
	D3D12_TEXTURE_ADDRESS_MODE addressMode, UINT shaderRegister,
	D3D12_SHADER_VISIBILITY shaderVisibility)
{
	D3D12_STATIC_SAMPLER_DESC desc;
	desc.Filter = filter;
	desc.AddressU = desc.AddressV = desc.AddressW = addressMode;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 16;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	desc.MinLOD = 0.0f;
	desc.MaxLOD = D3D12_FLOAT32_MAX;
	desc.ShaderRegister = shaderRegister;
	desc.RegisterSpace = 0;
	desc.ShaderVisibility = shaderVisibility;

	rootSignature.AddStaticSampler(desc);
}
