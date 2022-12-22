#pragma once

#include <optional>
#include <cstdint>

#include <FrameResourceContext.h>

#include "GraphicsEngineJob.h"
#include "VertexPipelineState.h"
#include "ConstantBuffer.h"

struct DrawCallInfo
{
	UINT nrOfInstances = 0;
	UINT startIndex = 0;
	UINT nrOfIndices = 0;
};

template<FrameType Frames>
class VertexJob : public GraphicsEngineJob<Frames>
{
private:

protected:
	struct StandardShaderBuffers
	{
		GraphicsEngineJob<Frames>::ShaderBuffer perExecution;
		GraphicsEngineJob<Frames>::ShaderBuffer perDraw;
		GraphicsEngineJob<Frames>::ShaderBuffer perEntity;

		StandardShaderBuffers() = default;
	};

	VertexPipelineState pipelineState;

	StandardShaderBuffers buffersVS;
	StandardShaderBuffers buffersHS;
	StandardShaderBuffers buffersDS;
	StandardShaderBuffers buffersGS;
	StandardShaderBuffers buffersPS;

	ViewIdentifier RequestTexture2DRTV(FrameSetupContext& context,
		const TransientResourceIndex& index,
		const std::optional<DXGI_FORMAT>& rtvFormat = std::nullopt);
	ViewIdentifier RequestTexture2DDSV(FrameSetupContext& context,
		const TransientResourceIndex& index,
		const std::optional<DXGI_FORMAT>& dsvFormat = std::nullopt);

	void SetupPipeline(ID3D12GraphicsCommandList* list,
		D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	void SetShaderConstantBuffer(ID3D12GraphicsCommandList* list,
		const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
		const FrameResourceContext<Frames>& context, size_t offset = 0);
	void SetShaderStructuredBuffer(ID3D12GraphicsCommandList* list,
		const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
		const FrameResourceContext<Frames>& context);

	void SetPerExecutionShaderBuffers(ID3D12GraphicsCommandList* list,
		const FrameResourceContext<Frames>& context);
	void SetPerEntityShaderBuffers(ID3D12GraphicsCommandList* list,
		const FrameResourceContext<Frames>& context);

	void SetIndexBufferView(ID3D12GraphicsCommandList* list, 
		DXGI_FORMAT format, const CategoryResourceIdentifier& identifier,
		const FrameResourceContext<Frames>& context);

	void DrawIndexedInstanced(const DrawCallInfo& info,
		ID3D12GraphicsCommandList* list);

public:
	VertexJob() = default;
	virtual ~VertexJob() = default;
	VertexJob(const VertexJob& other) = delete;
	VertexJob& operator=(const VertexJob& other) = delete;
	VertexJob(VertexJob&& other) = default;
	VertexJob& operator=(VertexJob&& other) = default;
};

template<FrameType Frames>
inline ViewIdentifier VertexJob<Frames>::RequestTexture2DRTV(
	FrameSetupContext& context, const TransientResourceIndex& index,
	const std::optional<DXGI_FORMAT>& rtvFormat)
{
	DescriptorRequest<std::optional<D3D12_RENDER_TARGET_VIEW_DESC>> request;
	request.index = index;

	if (rtvFormat.has_value())
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = *rtvFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		request.info = rtvDesc;
	}

	return context.RequestTransientRTV(request);
}

template<FrameType Frames>
inline ViewIdentifier VertexJob<Frames>::RequestTexture2DDSV(
	FrameSetupContext& context, const TransientResourceIndex& index,
	const std::optional<DXGI_FORMAT>& dsvFormat)
{
	DescriptorRequest<std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC>> request;
	request.index = index;

	if (dsvFormat.has_value())
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = *dsvFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		request.info = dsvDesc;
	}

	return context.RequestTransientDSV(request);
}

template<FrameType Frames>
inline void VertexJob<Frames>::SetupPipeline(
	ID3D12GraphicsCommandList* list, D3D_PRIMITIVE_TOPOLOGY topology)
{
	list->SetPipelineState(pipelineState.GetPipelineState());
	list->SetGraphicsRootSignature(this->rootSignature.GetRootSignature());
	list->IASetPrimitiveTopology(topology);
	list->RSSetScissorRects(1, &pipelineState.ScissorRect());
	list->RSSetViewports(1, &pipelineState.Viewport());
}

template<FrameType Frames>
inline void VertexJob<Frames>::SetShaderConstantBuffer(
	ID3D12GraphicsCommandList* list,
	const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
	const FrameResourceContext<Frames>& context, size_t offset)
{
	LocalResourceHandle handle = context.GetLocalResource(shaderBuffers.resourceIndex);
	D3D12_GPU_VIRTUAL_ADDRESS adress = handle.resource->GetGPUVirtualAddress();
	adress += handle.offset + offset;
	list->SetGraphicsRootConstantBufferView(shaderBuffers.rootParameterIndex, adress);
}

template<FrameType Frames>
inline void VertexJob<Frames>::SetShaderStructuredBuffer(
	ID3D12GraphicsCommandList* list,
	const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
	const FrameResourceContext<Frames>& context)
{
	LocalResourceHandle handle = context.GetLocalResource(shaderBuffers.resourceIndex);
	list->SetGraphicsRootShaderResourceView(shaderBuffers.rootParameterIndex,
		handle.resource->GetGPUVirtualAddress() + handle.offset);
}

template<FrameType Frames>
inline void VertexJob<Frames>::SetPerExecutionShaderBuffers(
	ID3D12GraphicsCommandList* list, 
	const FrameResourceContext<Frames>& context)
{
	if (buffersVS.perExecution.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderConstantBuffer(list, buffersVS.perExecution, context);
	}

	if (buffersHS.perExecution.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderConstantBuffer(list, buffersHS.perExecution, context);
	}

	if (buffersDS.perExecution.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderConstantBuffer(list, buffersDS.perExecution, context);
	}

	if (buffersGS.perExecution.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderConstantBuffer(list, buffersGS.perExecution, context);
	}

	if (buffersPS.perExecution.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderConstantBuffer(list, buffersPS.perExecution, context);
	}
}

template<FrameType Frames>
inline void VertexJob<Frames>::SetPerEntityShaderBuffers(
	ID3D12GraphicsCommandList* list, const FrameResourceContext<Frames>& context)
{
	if (buffersVS.perEntity.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderStructuredBuffer(list, buffersVS.perEntity, context);
	}

	if (buffersHS.perEntity.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderStructuredBuffer(list, buffersHS.perEntity, context);
	}

	if (buffersDS.perEntity.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderStructuredBuffer(list, buffersDS.perEntity, context);
	}

	if (buffersGS.perEntity.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderStructuredBuffer(list, buffersGS.perEntity, context);
	}

	if (buffersPS.perEntity.resourceIndex != LocalResourceIndex(-1))
	{
		this->SetShaderStructuredBuffer(list, buffersPS.perEntity, context);
	}
}

template<FrameType Frames>
inline void VertexJob<Frames>::SetIndexBufferView(
	ID3D12GraphicsCommandList* list, DXGI_FORMAT format,
	const CategoryResourceIdentifier& identifier,
	const FrameResourceContext<Frames>& context)
{
	CategoryResourceHandle handle = context.GetCategoryResource(identifier);
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = handle.resource->GetGPUVirtualAddress();
	gpuAddress += handle.offset;
	size_t indexSize = format == DXGI_FORMAT_R32_UINT ? 4 : 2;

	D3D12_INDEX_BUFFER_VIEW ibView;
	ibView.BufferLocation = gpuAddress;
	ibView.SizeInBytes = indexSize * handle.nrOfElements;
	ibView.Format = format;
	list->IASetIndexBuffer(&ibView);
}

template<FrameType Frames>
inline void VertexJob<Frames>::DrawIndexedInstanced(
	const DrawCallInfo& info, ID3D12GraphicsCommandList* list)
{
	list->DrawIndexedInstanced(info.nrOfIndices, info.nrOfInstances,
		info.startIndex, 0, 0);
}
