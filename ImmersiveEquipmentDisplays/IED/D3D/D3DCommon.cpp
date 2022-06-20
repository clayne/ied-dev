#include "pch.h"

#include "D3DCommon.h"

#include "D3DHelpers.h"

namespace IED
{
	D3DCommon::D3DCommon(
		ID3D11Device*               a_device,
		ID3D11DeviceContext*        a_context,
		const DXGI_SWAP_CHAIN_DESC& a_desc) noexcept(false) :
		m_device(a_device),
		m_context(a_context)
	{
		CD3D11_TEXTURE2D_DESC descDepth;
		descDepth.Width          = a_desc.BufferDesc.Width;
		descDepth.Height         = a_desc.BufferDesc.Height;
		descDepth.MipLevels      = 1;
		descDepth.ArraySize      = 1;
		descDepth.Format         = DXGI_FORMAT_D32_FLOAT;
		descDepth.SampleDesc     = a_desc.SampleDesc;
		descDepth.Usage          = D3D11_USAGE_DEFAULT;
		descDepth.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags      = 0;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;

		ThrowIfFailed(a_device->CreateTexture2D(
			std::addressof(descDepth),
			nullptr,
			depthStencilBuffer.ReleaseAndGetAddressOf()));

		CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(
			D3D11_DSV_DIMENSION_TEXTURE2D,
			descDepth.Format);

		ThrowIfFailed(a_device->CreateDepthStencilView(
			depthStencilBuffer.Get(),
			std::addressof(depthStencilViewDesc),
			m_depthStencilView.ReleaseAndGetAddressOf()));

		m_viewport = CD3D11_VIEWPORT(
			0.0f,
			0.0f,
			static_cast<float>(a_desc.BufferDesc.Width),
			static_cast<float>(a_desc.BufferDesc.Height));

		//m_states = std::make_unique<DirectX::CommonStates>(a_device);

		D3D11_DEPTH_STENCIL_DESC dsDesc = {};

		dsDesc.DepthEnable    = TRUE;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

		dsDesc.StencilEnable    = FALSE;
		dsDesc.StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK;
		dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

		dsDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
		dsDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

		dsDesc.BackFace = dsDesc.FrontFace;

		ThrowIfFailed(a_device->CreateDepthStencilState(
			std::addressof(dsDesc),
			m_depthStencilState.ReleaseAndGetAddressOf()));

		D3D11_BLEND_DESC blendDesc{};

		blendDesc.AlphaToCoverageEnable  = false;
		blendDesc.IndependentBlendEnable = true;

		for (std::uint32_t i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		{
			blendDesc.RenderTarget[i].BlendEnable           = true;
			blendDesc.RenderTarget[i].BlendOp               = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[i].DestBlendAlpha        = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[i].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[i].SrcBlendAlpha         = D3D11_BLEND_ONE;
		}

		ThrowIfFailed(a_device->CreateBlendState(
			std::addressof(blendDesc),
			m_blendState.ReleaseAndGetAddressOf()));

		D3D11_RASTERIZER_DESC rastDesc{};

		rastDesc.AntialiasedLineEnable = false;
		rastDesc.CullMode              = D3D11_CULL_FRONT;
		rastDesc.DepthBias             = 0;
		rastDesc.DepthBiasClamp        = 0.0f;
		rastDesc.DepthClipEnable       = true;
		rastDesc.FillMode              = D3D11_FILL_SOLID;
		rastDesc.FrontCounterClockwise = false;
		rastDesc.MultisampleEnable     = true;
		rastDesc.ScissorEnable         = false;
		rastDesc.SlopeScaledDepthBias  = 0.0f;

		ThrowIfFailed(a_device->CreateRasterizerState(
			std::addressof(rastDesc),
			m_rsCullClockwise.ReleaseAndGetAddressOf()));

		rastDesc.CullMode = D3D11_CULL_NONE;
		rastDesc.FillMode = D3D11_FILL_WIREFRAME;

		ThrowIfFailed(a_device->CreateRasterizerState(
			std::addressof(rastDesc),
			m_rsWireframe.ReleaseAndGetAddressOf()));
	}

	void D3DCommon::PrepareForDraw()
	{
		m_context->RSSetViewports(1, std::addressof(m_viewport));

		SetRenderTargets(m_flags.test(D3DCommonFlags::kDepth));

		m_context->ClearDepthStencilView(
			m_depthStencilView.Get(),
			D3D11_CLEAR_DEPTH,
			1.0f,
			0);

		/*if (m_flags.test(D3DCommonFlags::kAlpha))
		{*/
		/*m_context->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xFFFFFFFF);
		m_context->OMSetDepthStencilState(m_states->DepthRead(), 0);*/
		/*}
		else
		{
			m_context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
			m_context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
		}*/

		m_context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);
		m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
	}

	void D3DCommon::SetRasterizerState(D3DObjectRasterizerState a_rsstate)
	{
		ID3D11RasterizerState* rs;

		switch (a_rsstate)
		{
		case D3DObjectRasterizerState::kWireframe:
			rs = m_rsWireframe.Get();
			break;
		default:
			rs = m_rsCullClockwise.Get();
			break;
		}

		/*D3D11_RASTERIZER_DESC rastDesc;

		m_backup.RS->GetDesc(&rastDesc);

		_DMESSAGE(
			"rastDesc.AntialiasedLineEnable = %d\n"
			"rastDesc.CullMode              = %d\n"
			"rastDesc.DepthBias             = %d\n"
			"rastDesc.DepthBiasClamp        = %f\n"
			"rastDesc.DepthClipEnable       = %d\n"
			"rastDesc.FillMode              = %d\n"
			"rastDesc.FrontCounterClockwise = %d\n"
			"rastDesc.MultisampleEnable     = %d\n"
			"rastDesc.ScissorEnable         = %d\n"
			"rastDesc.SlopeScaledDepthBias  = %f\n",
			rastDesc.AntialiasedLineEnable,
			rastDesc.CullMode,
			rastDesc.DepthBias,
			rastDesc.DepthBiasClamp,
			rastDesc.DepthClipEnable,
			rastDesc.FillMode,
			rastDesc.FrontCounterClockwise,
			rastDesc.MultisampleEnable,
			rastDesc.ScissorEnable,
			rastDesc.SlopeScaledDepthBias);

		__debugbreak();*/

		m_context->RSSetState(rs);
	}

	void D3DCommon::SetRenderTargets(bool a_depth)
	{
		if (a_depth)
		{
			m_context->OMSetRenderTargets(
				m_backup.numRenderTargetViews,
				m_backup.RenderTargetViews,
				m_backup.DepthStencilView);
		}
		else
		{
			m_context->OMSetRenderTargets(
				m_backup.numRenderTargetViews,
				m_backup.RenderTargetViews,
				m_depthStencilView.Get());
		}
	}

	D3D11StateBackupImpl& D3DCommon::GetStateBackup(bool a_memzero)
	{
		if (a_memzero)
		{
			std::memset(std::addressof(m_backup), 0x0, sizeof(m_backup));
		}

		return m_backup;
	}

}