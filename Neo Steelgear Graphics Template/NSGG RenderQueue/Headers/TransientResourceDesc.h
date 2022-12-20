#pragma once

#include <optional>

#include <d3d12.h>

enum class TransientResourceBindFlag
{
	SRV,
	UAV,
	RTV,
	DSV
};

class TransientResourceDesc
{
private:
	bool hasSRV = false;

	D3D12_RESOURCE_DESC desc = D3D12_RESOURCE_DESC();
	std::optional<D3D12_CLEAR_VALUE> optimalClearValue = std::nullopt;

public:
	TransientResourceDesc() = default;
	virtual ~TransientResourceDesc() = default;
	TransientResourceDesc(const TransientResourceDesc& other) = default;
	TransientResourceDesc& operator=(const TransientResourceDesc& other) = default;
	TransientResourceDesc(TransientResourceDesc&& other) noexcept = default;
	TransientResourceDesc& operator=(TransientResourceDesc&& other) noexcept = default;

	void InitializeAsBuffer(UINT64 bufferByteSize);
	void InitializeAsTexture2D(UINT64 width, UINT height, UINT16 arraySize,
		UINT16 mipLevels, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc,
		std::optional<D3D12_CLEAR_VALUE> optimalTextureClearValue = std::nullopt);

	void AddBindFlag(TransientResourceBindFlag bindFlag);

	size_t CalculateTotalSize(ID3D12Device* device) const;

	bool HasRTV() const;
	bool HasDSV() const;

	const D3D12_RESOURCE_DESC& GetResourceDesc() const;
	const D3D12_CLEAR_VALUE* GetOptimalClearValue() const;
};