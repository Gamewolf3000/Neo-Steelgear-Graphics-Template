#pragma once

#include <string>
#include <sstream>

#include <Windows.h>
#include <d3d12.h>

#include "Dear ImGui\imgui.h"

class ImguiContext
{
private:
	ID3D12Device* device = nullptr;
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	const UINT MAX_DESCRIPTOR_COUNT = 100;
	UINT currentDescriptorCount = 1;
	UINT descriptorSize = 0;

public:
	ImguiContext() = default;
	~ImguiContext();
	ImguiContext(const ImguiContext& other) = delete;
	ImguiContext& operator=(const ImguiContext& other) = delete;
	ImguiContext(ImguiContext&& other) = default;
	ImguiContext& operator=(ImguiContext&& other) = default;

	void Initialize(HWND windowHandle, ID3D12Device* deviceToUse);

	void StartImguiFrame();
	void FinishImguiFrame(ID3D12GraphicsCommandList* list);

	template<typename T, typename... TextData>
	inline std::stringstream ToString(const T& currentElement, TextData... data);
	template<typename T>
	inline std::stringstream ToString(const T& data);

	template<typename... TextData>
	void AddText(const TextData&... data);
	void AddTextureImage(ID3D12Resource* texture);
};

template<typename T, typename ...TextData>
inline std::stringstream ImguiContext::ToString(const T& currentElement, TextData ...data)
{
	std::stringstream toReturn;
	toReturn << currentElement;
	std::string remainder = ToString(data...).str();
	toReturn << remainder;
	return toReturn;
}

template<typename T>
inline std::stringstream ImguiContext::ToString(const T& data)
{
	std::stringstream toReturn;
	toReturn << data;
	return toReturn;
}

template<typename ...TextData>
inline void ImguiContext::AddText(const TextData & ...data)
{
	std::string text;
	text = ToString(data...).str();
	ImGui::Text(text.c_str());
}