#pragma once

#include <array>

#include "PipelineState.h"

enum class ShaderStage
{
	VS,
	HS,
	DS,
	GS,
	PS
};

struct RenderTarget
{
	DXGI_FORMAT format;
	D3D12_RENDER_TARGET_BLEND_DESC blendDesc;

	RenderTarget()
	{
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		blendDesc.BlendEnable = false;
		blendDesc.LogicOpEnable = false;
		blendDesc.SrcBlend = D3D12_BLEND_ONE;
		blendDesc.DestBlend = D3D12_BLEND_ZERO;
		blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
};

class VertexPipelineState : public PipelineState
{
private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	std::array<D3DPtr<ID3DBlob>, 5> shaderBlobs;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

public:
	VertexPipelineState();
	~VertexPipelineState() = default;
	VertexPipelineState(const VertexPipelineState& other) = delete;
	VertexPipelineState& operator=(const VertexPipelineState& other) = delete;
	VertexPipelineState(VertexPipelineState&& other) = default;
	VertexPipelineState& operator=(VertexPipelineState&& other) = default;

	void SetBasedOnWindow(unsigned int width, unsigned int height);

	void SetRootSignature(ID3D12RootSignature* rootSignature);
	void SetShader(ShaderStage stage, const std::string& path);
	void SetFormatDSV(DXGI_FORMAT format);
	void AddRenderTarget(RenderTarget& renderTarget);
	void SetCachedPSO(const std::string& path);

	D3D12_RASTERIZER_DESC& RasterizerDesc();
	D3D12_BLEND_DESC& BlendDesc();
	D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc();
	D3D12_STREAM_OUTPUT_DESC& StreamOutputDesc();

	D3D12_VIEWPORT& Viewport();
	D3D12_RECT& ScissorRect();
	D3D_PRIMITIVE_TOPOLOGY& PrimitiveTopology();

	void Finalize(ID3D12Device* deviceToUse);
};