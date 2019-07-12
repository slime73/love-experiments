/**
 * Copyright (c) 2006-2019 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#include "RenderPass.h"
#include "Texture.h"
#include "Quad.h"
#include "Graphics.h"

// C++
#include <algorithm>

namespace love
{
namespace graphics
{

RenderPass::RenderPass(Graphics *gfx, const RenderTargetSetup &rts)
	: data(nullptr)
	, dataSize(0)
	, currentOffset(0)
{
	reset(gfx, rts);

	dataSize = 1024;
	data = (uint8 *) malloc(dataSize);
	commands.reserve(10);
}

RenderPass::~RenderPass()
{
	reset();
	if (data != nullptr)
		free(data);
}

template <typename T>
T *RenderPass::addCommand(CommandType type, size_t size)
{
	// TODO: should we care about alignment?
	if (currentOffset + size > dataSize)
	{
		size_t newSize = std::max(dataSize * 2, currentOffset + size);
		uint8 *newData = (uint8 *) realloc(data, newSize);
		if (newData == nullptr)
			return nullptr;

		data = newData;
		dataSize = newSize;
	}

	Command header;
	header.type = type;
	header.offset = (int) currentOffset;

	commands.push_back(header);

	T *cmd = (T *) (data + currentOffset);
	currentOffset += size;
	return cmd;
}

void RenderPass::reset()
{
	commands.clear();
	inputs.clear();
	currentOffset = 0;
}

void RenderPass::reset(Graphics *gfx, const RenderTargetSetup &rts)
{
	validateRenderTargets(gfx, rts);
	reset();
	renderTargets = rts;
}

void RenderPass::draw(Drawable *drawable, const Matrix4 &transform)
{
	auto cmd = addCommand<CommandDrawDrawable>(COMMAND_DRAW_DRAWABLE);
	cmd->drawable = drawable;
	cmd->transform = transform;
	inputs.emplace_back(drawable);
}

void RenderPass::drawInstanced(Mesh *mesh, const Matrix4 &transform, int instanceCount)
{
	auto cmd = addCommand<CommandDrawMeshInstanced>(COMMAND_DRAW_MESH_INSTANCED);
	cmd->mesh = mesh;
	cmd->instanceCount = instanceCount;
	cmd->transform = transform;
	inputs.emplace_back(mesh);
}

void RenderPass::setColor(const Colorf &color)
{
	auto c = addCommand<Colorf>(COMMAND_SET_COLOR);
	*c = color;

	c->r = std::min(std::max(c->r, 0.0f), 1.0f);
	c->g = std::min(std::max(c->g, 0.0f), 1.0f);
	c->b = std::min(std::max(c->b, 0.0f), 1.0f);
	c->a = std::min(std::max(c->a, 0.0f), 1.0f);

	gammaCorrectColor(*c);
}

void RenderPass::setShader(Shader *shader)
{
	auto cmd = addCommand<ShaderState>(COMMAND_SET_SHADER);
	cmd->shader = shader;

	if (shader != nullptr)
		inputs.emplace_back(shader);
}

void RenderPass::setShader()
{
	setShader(nullptr);
}

void RenderPass::setBlendMode(BlendMode mode, BlendAlpha alphaMode)
{
	setBlendState(getBlendState(mode, alphaMode));
}

void RenderPass::setBlendState(const BlendState &state)
{
	auto cmd = addCommand<BlendState>(COMMAND_SET_BLENDSTATE);
	*cmd = state;
}

void RenderPass::setStencil(CompareMode compare, StencilAction action, int value, uint32 readMask, uint32 writeMask)
{
	auto cmd = addCommand<StencilState>(COMMAND_SET_STENCILSTATE);
	cmd->compare = compare;
	cmd->action = action;
	cmd->value = value;
	cmd->readMask = readMask;
	cmd->writeMask = writeMask;
}

void RenderPass::setStencil()
{
	setStencil(COMPARE_ALWAYS, STENCIL_KEEP);
}

void RenderPass::setDepthMode(CompareMode compare, bool write)
{
	auto cmd = addCommand<DepthState>(COMMAND_SET_DEPTHSTATE);
	cmd->compare = compare;
	cmd->write = write;
}

void RenderPass::setDepthMode()
{
	setDepthMode(COMPARE_ALWAYS, false);
}

void RenderPass::execute(Graphics *gfx)
{
	auto &rts = renderTargets;

	if (rts.temporaryRTFlags != 0 && rts.depthStencil.canvas.get() == nullptr)
	{
		bool wantsdepth   = (rts.temporaryRTFlags & TEMPORARY_RT_DEPTH) != 0;
		bool wantsstencil = (rts.temporaryRTFlags & TEMPORARY_RT_STENCIL) != 0;

		PixelFormat dsformat = PIXELFORMAT_STENCIL8;
		if (wantsdepth && wantsstencil)
			dsformat = PIXELFORMAT_DEPTH24_STENCIL8;
		else if (wantsdepth && gfx->isCanvasFormatSupported(PIXELFORMAT_DEPTH24, false))
			dsformat = PIXELFORMAT_DEPTH24;
		else if (wantsdepth && gfx->isCanvasFormatSupported(PIXELFORMAT_DEPTH32F, false))
			dsformat = PIXELFORMAT_DEPTH32F;
		else if (wantsdepth)
			dsformat = PIXELFORMAT_DEPTH16;
		else if (wantsstencil)
			dsformat = PIXELFORMAT_STENCIL8;

		Canvas *colorcanvas = rts.colors[0].canvas.get();
		int pixelw = colorcanvas->getPixelWidth();
		int pixelh = colorcanvas->getPixelHeight();
		int reqmsaa = colorcanvas->getRequestedMSAA();

		RenderTarget rt;
		rt.canvas = gfx->getTemporaryCanvas(dsformat, pixelw, pixelh, reqmsaa);
		rt.beginAction = BEGIN_CLEAR;
		rt.endAction = END_DISCARD;
		rts.depthStencil = rt;
	}

	beginPass(gfx);

	uint32 diff = 0xFFFFFFFF;
	GraphicsState currentState;

	for (auto cmd : commands)
	{
		switch (cmd.type)
		{
		case COMMAND_DRAW_DRAWABLE:
		{
			const auto &c = (const CommandDrawDrawable &) data[cmd.offset];
			c.drawable->draw(gfx, c.transform);
			break;
		}
		case COMMAND_DRAW_QUAD:
		{
			const auto &c = (const CommandDrawQuad &) data[cmd.offset];
			c.texture->draw(gfx, c.quad, c.transform);
			break;
		}
		case COMMAND_DRAW_LAYER:
		{

			break;
		}
		case COMMAND_DRAW_QUAD_LAYER:
		{

			break;
		}
		case COMMAND_DRAW_MESH_INSTANCED:
		{
			const auto &c = (const CommandDrawMeshInstanced &) data[cmd.offset];
			c.mesh->drawInstanced(gfx, c.transform, c.instanceCount);
			break;
		}
		case COMMAND_DRAW_POINTS:
		{

			break;
		}
		case COMMAND_DRAW_LINE:
		{

			break;
		}
		case COMMAND_DRAW_POLYGON:
		{

			break;
		}
		case COMMAND_PRINT:
		{

			break;
		}
		case COMMAND_PRINTF:
		{

			break;
		}
		case COMMAND_SET_COLOR:
		{
			currentState.color = (const Colorf &) data[cmd.offset];
			diff |= STATEBIT_COLOR;
			break;
		}
		case COMMAND_SET_SHADER:
		{
			currentState.shader = ((const ShaderState &) data[cmd.offset]).shader;
			diff |= STATEBIT_SHADER;
			break;
		}
		case COMMAND_SET_BLENDSTATE:
		{
			currentState.blend = (const BlendState &) data[cmd.offset];
			diff |= STATEBIT_BLEND;
			break;
		}
		case COMMAND_SET_STENCILSTATE:
		{
			currentState.stencil = (const StencilState &) data[cmd.offset];
			diff |= STATEBIT_STENCIL;
			break;
		}
		case COMMAND_SET_DEPTHSTATE:
		{
			currentState.depth = (const DepthState &) data[cmd.offset];
			diff |= STATEBIT_DEPTH;
			break;
		}
		case COMMAND_SET_SCISSOR:
		{
			currentState.scissor = (const ScissorState &) data[cmd.offset];
			diff |= STATEBIT_SCISSOR;
			break;
		}
		case COMMAND_SET_COLORMASK:
		{
			currentState.colorChannelMask = (const ColorChannelMask &) data[cmd.offset];
			diff |= STATEBIT_COLORMASK;
			break;
		}
		case COMMAND_SET_WIREFRAME:
		{
			currentState.wireframe = (const bool &) data[cmd.offset];
			diff |= STATEBIT_WIREFRAME;
			break;
		}
		}
	}

	gfx->flushStreamDraws();

	endPass(gfx);

	for (int i = 0; i < renderTargets.colorCount; i++)
	{
		const auto &rt = renderTargets.colors[i];
		if (rt.canvas->getMipmapMode() == Canvas::MIPMAPS_AUTO && rt.mipmap == 0)
			rt.canvas->generateMipmaps();
	}

	const auto &ds = renderTargets.depthStencil;
	if (ds.canvas.get() && ds.canvas->getMipmapMode() == Canvas::MIPMAPS_AUTO && ds.mipmap == 0)
		ds.canvas->generateMipmaps();

	if (rts.temporaryRTFlags != 0 && rts.depthStencil.canvas.get() == nullptr)
		rts.depthStencil.canvas = nullptr;
}

void RenderPass::validateRenderTargets(Graphics *gfx, const RenderTargetSetup &rts) const
{
	const auto &capabilities = gfx->getCapabilities();
	RenderTarget firsttarget = rts.getFirstTarget();
	love::graphics::Canvas *firstcanvas = firsttarget.canvas;
	int ncolors = rts.colorCount;

	if (firstcanvas == nullptr)
		return;

	if (ncolors > capabilities.limits[Graphics::LIMIT_MULTI_CANVAS])
		throw love::Exception("This system can't simultaneously render to %d canvases.", ncolors);

	bool multiformatsupported = capabilities.features[Graphics::FEATURE_MULTI_CANVAS_FORMATS];

	PixelFormat firstcolorformat = PIXELFORMAT_UNKNOWN;
	if (ncolors > 0)
		firstcolorformat = rts.colors[0].canvas->getPixelFormat();

	int pixelw = firstcanvas->getPixelWidth(firsttarget.mipmap);
	int pixelh = firstcanvas->getPixelHeight(firsttarget.mipmap);
	int reqmsaa = firstcanvas->getRequestedMSAA();

	for (int i = 0; i < ncolors; i++)
	{
		love::graphics::Canvas *c = rts.colors[i].canvas;
		PixelFormat format = c->getPixelFormat();
		int mip = rts.colors[i].mipmap;
		int slice = rts.colors[i].slice;

		if (mip < 0 || mip >= c->getMipmapCount())
			throw love::Exception("Invalid mipmap level %d.", mip + 1);

		if (!c->isValidSlice(slice))
			throw love::Exception("Invalid slice index: %d.", slice + 1);

		if (c->getPixelWidth(mip) != pixelw || c->getPixelHeight(mip) != pixelh)
			throw love::Exception("All canvases must have the same pixel dimensions.");

		if (!multiformatsupported && format != firstcolorformat)
			throw love::Exception("This system doesn't support multi-canvas rendering with different canvas formats.");

		if (c->getRequestedMSAA() != reqmsaa)
			throw love::Exception("All Canvases must have the same MSAA value.");

		if (isPixelFormatDepthStencil(format))
			throw love::Exception("Depth/stencil format Canvases must be used with the 'depthstencil' field of a render pass.");
	}

	if (rts.depthStencil.canvas != nullptr)
	{
		love::graphics::Canvas *c = rts.depthStencil.canvas;
		int mip = rts.depthStencil.mipmap;
		int slice = rts.depthStencil.slice;

		if (!isPixelFormatDepthStencil(c->getPixelFormat()))
			throw love::Exception("Only depth/stencil format Canvases can be used with the 'depthstencil' field of a render pass");

		if (c->getPixelWidth(mip) != pixelw || c->getPixelHeight(mip) != pixelh)
			throw love::Exception("All canvases must have the same pixel dimensions.");

		if (c->getRequestedMSAA() != firstcanvas->getRequestedMSAA())
			throw love::Exception("All Canvases must have the same MSAA value.");

		if (mip < 0 || mip >= c->getMipmapCount())
			throw love::Exception("Invalid mipmap level %d.", mip + 1);

		if (!c->isValidSlice(slice))
			throw love::Exception("Invalid slice index: %d.", slice + 1);
	}
}

} // graphics
} // love
