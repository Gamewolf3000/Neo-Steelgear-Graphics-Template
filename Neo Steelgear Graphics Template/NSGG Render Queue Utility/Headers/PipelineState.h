#pragma once

#include <string>

#include <d3d12.h>

#include <D3DPtr.h>

class PipelineState
{
protected:
	D3DPtr<ID3D12PipelineState> pipelineState;

	D3DPtr<ID3DBlob> cachedPSO;

	ID3DBlob* LoadBlob(const std::string& filepath);

public:
	PipelineState() = default;
	~PipelineState() = default;
	PipelineState(const PipelineState& other) = delete;
	PipelineState& operator=(const PipelineState& other) = delete;
	PipelineState(PipelineState&& other) = default;
	PipelineState& operator=(PipelineState&& other) = default;

	void CachePSO(const std::string& path);

	ID3D12PipelineState* GetPipelineState();
};