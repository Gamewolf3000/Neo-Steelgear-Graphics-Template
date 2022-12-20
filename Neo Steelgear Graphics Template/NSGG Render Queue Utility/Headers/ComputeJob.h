#pragma once

#include <optional>

#include "GraphicsEngineJob.h"
#include "ComputePipelineState.h"
#include "ConstantBuffer.h"

template<FrameType Frames>
class ComputeJob : public GraphicsEngineJob<Frames>
{
protected:
	ComputePipelineState pipelineState;

	void SetupPipeline(ID3D12GraphicsCommandList* list);

	void SetShaderConstantBuffer(ID3D12GraphicsCommandList* list,
		const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
		const FrameResourceContext<Frames>& context, size_t offset = 0);
	void SetShaderStructuredBuffer(ID3D12GraphicsCommandList* list,
		const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
		const FrameResourceContext<Frames>& context);

public:
	ComputeJob() = default;
	virtual ~ComputeJob() = default;
	ComputeJob(const ComputeJob& other) = delete;
	ComputeJob& operator=(const ComputeJob& other) = delete;
	ComputeJob(ComputeJob&& other) = default;
	ComputeJob& operator=(ComputeJob&& other) = default;
};

template<FrameType Frames>
inline void ComputeJob<Frames>::SetupPipeline(
	ID3D12GraphicsCommandList* list)
{
	list->SetPipelineState(pipelineState.GetPipelineState());
	list->SetComputeRootSignature(this->rootSignature.GetRootSignature());
}

template<FrameType Frames>
inline void ComputeJob<Frames>::SetShaderConstantBuffer(
	ID3D12GraphicsCommandList* list,
	const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
	const FrameResourceContext<Frames>& context, size_t offset)
{
	LocalResourceHandle handle = context.GetLocalResource(shaderBuffers.resourceIndex);
	D3D12_GPU_VIRTUAL_ADDRESS adress = handle.resource->GetGPUVirtualAddress();
	adress += handle.offset + offset;
	list->SetComputeRootConstantBufferView(shaderBuffers.rootParameterIndex, adress);
}

template<FrameType Frames>
inline void ComputeJob<Frames>::SetShaderStructuredBuffer(
	ID3D12GraphicsCommandList* list,
	const GraphicsEngineJob<Frames>::ShaderBuffer& shaderBuffers,
	const FrameResourceContext<Frames>& context)
{
	LocalResourceHandle handle = context.GetLocalResource(shaderBuffers.resourceIndex);
	list->SetComputeRootShaderResourceView(shaderBuffers.rootParameterIndex,
		handle.resource->GetGPUVirtualAddress() + handle.offset);
}