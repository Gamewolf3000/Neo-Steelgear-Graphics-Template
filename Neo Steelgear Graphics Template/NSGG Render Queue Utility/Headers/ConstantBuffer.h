#pragma once

#include <cmath>

template<typename T>
class ConstantBuffer
{
private:
	T data;
	unsigned char padding[(((sizeof(T) - 1) / 256) + 1) * 256 - sizeof(T)];

public:
	ConstantBuffer() = default;
	ConstantBuffer(const T& data);
	ConstantBuffer(T&& data);
	~ConstantBuffer() = default;
	ConstantBuffer(const ConstantBuffer& other) = default;
	ConstantBuffer& operator=(const ConstantBuffer& other) = default;
	ConstantBuffer(ConstantBuffer&& other) = default;
	ConstantBuffer& operator=(ConstantBuffer&& other) = default;

	T& Data();
	const T& Data() const;
};

template<typename T>
inline ConstantBuffer<T>::ConstantBuffer(const T& data)
{
	this->data = data;
}

template<typename T>
inline ConstantBuffer<T>::ConstantBuffer(T&& data)
{
	this->data = std::move(data);
}

template<typename T>
inline T& ConstantBuffer<T>::Data()
{
	return data;
}

template<typename T>
inline const T& ConstantBuffer<T>::Data() const
{
	return data;
}
