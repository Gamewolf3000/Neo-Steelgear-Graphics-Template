#pragma once

typedef size_t TransientResourceIndex;

enum class FrameViewType
{
	SHADER_BINDABLE,
	RTV,
	DSV
};

typedef size_t TransientResourceViewIndex;

struct ViewIdentifier
{
	FrameViewType type;
	TransientResourceViewIndex internalIndex;
};

typedef size_t LocalResourceIndex;