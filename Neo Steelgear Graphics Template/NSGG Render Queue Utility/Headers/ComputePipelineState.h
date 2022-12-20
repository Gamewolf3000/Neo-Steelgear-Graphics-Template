#pragma once

#include "PipelineState.h"

class ComputePipelineState : public PipelineState
{
private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
	D3DPtr<ID3DBlob> shaderBlob;

public:
	ComputePipelineState();
	~ComputePipelineState() = default;
	ComputePipelineState(const ComputePipelineState& other) = delete;
	ComputePipelineState& operator=(const ComputePipelineState& other) = delete;
	ComputePipelineState(ComputePipelineState&& other) = default;
	ComputePipelineState& operator=(ComputePipelineState&& other) = default;

	void SetRootSignature(ID3D12RootSignature* rootSignature);
	void SetShader(const std::string& path);
	void SetCachedPSO(const std::string& path);

	void Finalize(ID3D12Device* deviceToUse);
};