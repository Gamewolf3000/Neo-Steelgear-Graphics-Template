#pragma once

#include <string>

#include <entt.hpp>
#include <FrameBased.h>

#include "FramePreparationContext.h"
#include "FrameSetupContext.h"
#include "FrameResourceContext.h"
#include "ImguiContext.h"

typedef size_t PassCost;

template<FrameType Frames>
class QueueContext;

template<FrameType Frames>
class QueueJob
{
private:

public:
	QueueJob() = default;
	~QueueJob() = default;
	QueueJob(const QueueJob& other) = delete;
	QueueJob& operator=(const QueueJob& other) = delete;
	QueueJob(QueueJob&& other) noexcept = default;
	QueueJob& operator=(QueueJob&& other) noexcept = default;

	virtual void SetupQueue(QueueContext<Frames>& context) const = 0;

	virtual void CalculateFrameCosts(const entt::registry& frameRegistry) = 0;
	virtual PassCost GetPreparationCost() const = 0; // Should return minimum of 1
	virtual PassCost GetExecutionCost() const = 0; // Should return minimum of 1

	virtual void PrepareFrame(const entt::registry& frameRegistry,
		const FramePreparationContext<Frames>& context) = 0;
	virtual void SetResourceInfo(FrameSetupContext& context) = 0;
	virtual void ExecuteFrame(ID3D12GraphicsCommandList* list, 
		FrameResourceContext<Frames>& context) = 0;
	virtual void PerformImguiOperations(ImguiContext& context);
};

template<FrameType Frames>
inline void QueueJob<Frames>::PerformImguiOperations(ImguiContext& context)
{
	(void)context; // Default function that does nothing if no override is provided
}
