#pragma once

#include <vector>

#include <d3d12.h>

#include <D3DPtr.h>

struct RootParameter
{
	D3D12_SHADER_VISIBILITY visibility;
	D3D12_ROOT_PARAMETER_TYPE viewType;
	UINT shaderRegister = UINT(-1);
	UINT registerSpace = UINT(-1);
};

class RootSignature
{
protected:
	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	D3DPtr<ID3D12RootSignature> rootSignature;

public:
	RootSignature() = default;
	~RootSignature() = default;
	RootSignature(const RootSignature& other) = delete;
	RootSignature& operator=(const RootSignature& other) = delete;
	RootSignature(RootSignature&& other) = default;
	RootSignature& operator=(RootSignature&& other) = default;

	UINT AddRootBuffer(const RootParameter& rootParameter);
	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc);

	void Finalize(ID3D12Device* deviceToUse);

	ID3D12RootSignature* GetRootSignature() const;
};