#pragma once

#include <cstdint>
#include <stdexcept>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <D3DPtr.h>

struct DeviceRequirements
{
	std::uint8_t adapterIndex = std::uint8_t(-1);
	D3D_FEATURE_LEVEL requiredLevel = D3D_FEATURE_LEVEL_12_0;
	D3D12_RAYTRACING_TIER rtTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	D3D_SHADER_MODEL shaderModel = D3D_SHADER_MODEL::D3D_SHADER_MODEL_6_6;
};

class ManagedDevice
{
private:
	D3DPtr<ID3D12Device> device;
	D3DPtr<IDXGIAdapter1> adapter;

	void ThrowIfFailed(HRESULT hr, const std::exception& exception);

	bool CheckSupportDXR(ID3D12Device* deviceToCheck, D3D12_RAYTRACING_TIER requiredTier);
	bool CheckSupportShaderModel(ID3D12Device* deviceToCheck, D3D_SHADER_MODEL requiredModel);
	bool CheckDevice(ID3D12Device* device, const DeviceRequirements& requirements);
	IDXGIAdapter* GetAdapter(IDXGIFactory2* factory, const DeviceRequirements& requirements);

public:
	ManagedDevice() = default;
	~ManagedDevice() = default;

	void Initialize(IDXGIFactory2* factory, const DeviceRequirements& requirements);

	ID3D12Device* GetDevice();
};