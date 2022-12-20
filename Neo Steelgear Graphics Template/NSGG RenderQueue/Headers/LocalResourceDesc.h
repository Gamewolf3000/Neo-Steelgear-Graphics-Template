#pragma once


class LocalResourceDesc
{
private:
	size_t elementSize = 0;
	size_t nrOfElements = 0;
	size_t resourceAlignment = 0;

public:
	LocalResourceDesc(size_t elementSize, size_t nrOfElements,
		size_t resourceAlignment);
	virtual ~LocalResourceDesc() = default;
	LocalResourceDesc(const LocalResourceDesc& other) = default;
	LocalResourceDesc& operator=(const LocalResourceDesc& other) = default;
	LocalResourceDesc(LocalResourceDesc&& other) noexcept = default;
	LocalResourceDesc& operator=(LocalResourceDesc&& other) noexcept = default;

	size_t GetAlignment() const;
	size_t GetSize() const;
};