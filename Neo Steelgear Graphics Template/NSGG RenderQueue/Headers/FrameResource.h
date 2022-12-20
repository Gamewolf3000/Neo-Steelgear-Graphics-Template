#pragma once

#include <optional>

#include <d3d12.h>

#include "FrameResourceIdentifier.h"
#include "FrameResourceBarrier.h"

class FrameResource
{
private:
	FrameResourceIdentifier identifier;
	bool initialTransitionPerformed = false;
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

	bool IsWriteState(D3D12_RESOURCE_STATES state) const;

public:
	FrameResource(const FrameResourceIdentifier& identifier);
	~FrameResource() = default;
	FrameResource(const FrameResource& other) = delete;
	FrameResource& operator=(const FrameResource& other) = delete;
	FrameResource(FrameResource&& other) noexcept = default;
	FrameResource& operator=(FrameResource&& other) noexcept = default;

	std::optional<FrameResourceBarrier> UpdateState(D3D12_RESOURCE_STATES newState);

	D3D12_RESOURCE_STATES GetInitialState() const;
	D3D12_RESOURCE_STATES GetCurrentState() const;
	bool IsInWriteState() const;
};