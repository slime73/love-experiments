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
#include "common/memory.h"

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
	data = (uint8 *) malloc(dataSize); // TODO: aligned malloc
	commands.reserve(10);
}

RenderPass::~RenderPass()
{
	reset();
	if (data != nullptr)
		free(data);
}

template <typename T>
T *RenderPass::addCommand(CommandType type, size_t size, size_t alignment)
{
	size = alignUp(size, alignment);
	currentOffset = alignUp(currentOffset, alignment);

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

	graphicsState.clear();
	graphicsState.emplace_back();

	transformState.clear();
	transformState.emplace_back();
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
	cmd->transform = transformState.back() * transform;
	inputs.emplace_back(drawable);
}

void RenderPass::drawInstanced(Graphics *gfx, Mesh *mesh, const Matrix4 &transform, int instanceCount)
{
	if (instanceCount > 1 && !gfx->getCapabilities().features[Graphics::FEATURE_INSTANCING])
		throw love::Exception("Instancing is not supported on this system.");

	auto cmd = addCommand<CommandDrawMeshInstanced>(COMMAND_DRAW_MESH_INSTANCED);
	cmd->mesh = mesh;
	cmd->instanceCount = instanceCount;
	cmd->transform = transformState.back() * transform;
	inputs.emplace_back(mesh);
}

Vector2 *RenderPass::polyline(int count)
{
	if (count <= 0)
		return nullptr;

	size_t cmdsize = sizeof(CommandDrawLine) + sizeof(Vector2) * (count - 1);
	auto cmd = addCommand<CommandDrawLine>(COMMAND_DRAW_LINE, cmdsize);

	cmd->transform = transformState.back();
	cmd->count = count;

	return cmd->positions;
}

void RenderPass::polyline(const Vector2 *vertices, int count)
{
	Vector2 *v = polyline(count);
	if (v == nullptr)
		return;
	memcpy(v, vertices, count * sizeof(Vector2));
}

void RenderPass::setColor(const Colorf &color)
{
	auto &state = graphicsState.back();
	if (state.color == color)
		return;

	auto c = addCommand<Colorf>(COMMAND_SET_COLOR);
	*c = color;

	c->r = std::min(std::max(c->r, 0.0f), 1.0f);
	c->g = std::min(std::max(c->g, 0.0f), 1.0f);
	c->b = std::min(std::max(c->b, 0.0f), 1.0f);
	c->a = std::min(std::max(c->a, 0.0f), 1.0f);

	state.color = *c;
}

void RenderPass::setShader(Shader *shader)
{
	auto &state = graphicsState.back();
	if (state.render.shader == shader)
		return;

	auto cmd = addCommand<Shader *>(COMMAND_SET_SHADER);
	*cmd = shader;

	if (shader != nullptr)
		inputs.emplace_back(shader);

	state.render.shader = shader;
}

void RenderPass::setShader()
{
	setShader(nullptr);
}

void RenderPass::setBlendMode(BlendMode mode, BlendAlpha alphaMode)
{
	setBlendState(getBlendState(mode, alphaMode));
}

void RenderPass::setBlendState(const BlendState &blend)
{
	auto &s = graphicsState.back();
	if (s.render.blend == blend)
		return;

	auto cmd = addCommand<BlendState>(COMMAND_SET_BLENDSTATE);
	*cmd = blend;

	s.render.blend = blend;
}

void RenderPass::setStencil(CompareMode compare, StencilAction action, int value, uint32 readMask, uint32 writeMask)
{
	StencilState stencil;
	stencil.compare = compare;
	stencil.action = action;
	stencil.value = value;
	stencil.readMask = readMask;
	stencil.writeMask = writeMask;

	auto &s = graphicsState.back();
	if (s.render.stencil == stencil)
		return;

	auto cmd = addCommand<StencilState>(COMMAND_SET_STENCILSTATE);
	*cmd = stencil;

	s.render.stencil = stencil;
}

void RenderPass::setStencil()
{
	setStencil(COMPARE_ALWAYS, STENCIL_KEEP);
}

void RenderPass::setDepthMode(CompareMode compare, bool write)
{
	// TODO: s.render.depth
	auto cmd = addCommand<DepthState>(COMMAND_SET_DEPTHSTATE);
	cmd->compare = compare;
	cmd->write = write;
}

void RenderPass::setDepthMode()
{
	setDepthMode(COMPARE_ALWAYS, false);
}

void RenderPass::rotate(float r)
{
	transformState.back().rotate(r);
}

void RenderPass::scale(float x, float y)
{
	transformState.back().scale(x, y);
}

void RenderPass::translate(float x, float y)
{
	transformState.back().translate(x, y);
}

void RenderPass::shear(float kx, float ky)
{
	transformState.back().shear(kx, ky);
}

void RenderPass::origin()
{
	transformState.back().setIdentity();
}

template <typename T>
static const T *read(const uint8 *buffer, size_t offset)
{
	return (const T *) (buffer + offset);
}

void RenderPass::execute(Graphics *gfx)
{
	auto &rts = renderTargets;

	bool isBackbuffer = rts.isBackbuffer();
	bool tempDepthStencil = false;

	if (!isBackbuffer && (rts.flags & (RT_TEMPORARY_DEPTH | RT_TEMPORARY_STENCIL)) != 0)
		tempDepthStencil = true;

	if (tempDepthStencil)
	{
		bool wantsDepth   = (rts.flags & RT_TEMPORARY_DEPTH) != 0;
		bool wantsStencil = (rts.flags & RT_TEMPORARY_STENCIL) != 0;

		PixelFormat dsformat = PIXELFORMAT_STENCIL8;
		if (wantsDepth && wantsStencil)
			dsformat = PIXELFORMAT_DEPTH24_STENCIL8;
		else if (wantsDepth && gfx->isCanvasFormatSupported(PIXELFORMAT_DEPTH24, false))
			dsformat = PIXELFORMAT_DEPTH24;
		else if (wantsDepth && gfx->isCanvasFormatSupported(PIXELFORMAT_DEPTH32F, false))
			dsformat = PIXELFORMAT_DEPTH32F;
		else if (wantsDepth)
			dsformat = PIXELFORMAT_DEPTH16;
		else if (wantsStencil)
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

	DrawContext context;

	context.isBackbuffer = isBackbuffer;

	if (isBackbuffer)
	{
		context.passWidth = gfx->getWidth();
		context.passHeight = gfx->getHeight();
		context.passPixelWidth = gfx->getPixelWidth();
		context.passPixelHeight = gfx->getPixelHeight();
	}
	else
	{
		auto c = rts.getFirstTarget().canvas.get();
		context.passWidth = c->getWidth();
		context.passHeight = c->getHeight();
		context.passPixelWidth = c->getPixelWidth();
		context.passPixelHeight = c->getPixelHeight();
	}

	beginPass(&context);

	for (auto cmd : commands)
	{
		switch (cmd.type)
		{
		case COMMAND_DRAW_DRAWABLE:
		{
			const auto c = read<CommandDrawDrawable>(data, cmd.offset);
			c->drawable->draw(this, &context, c->transform);
			break;
		}
		case COMMAND_DRAW_QUAD:
		{
			const auto c = read<CommandDrawQuad>(data, cmd.offset);
			c->texture->draw(gfx, c->quad, c->transform);
			break;
		}
		case COMMAND_DRAW_LAYER:
		{
			// TODO
			break;
		}
		case COMMAND_DRAW_QUAD_LAYER:
		{
			// TODO
			break;
		}
		case COMMAND_DRAW_MESH_INSTANCED:
		{
			const auto c = read<CommandDrawMeshInstanced>(data, cmd.offset);
			c->mesh->drawInstanced(gfx, c->transform, c->instanceCount);
			break;
		}
		case COMMAND_DRAW_LINE:
		{
			// TODO
			break;
		}
		case COMMAND_DRAW_POLYGON:
		{
			// TODO
			break;
		}
		case COMMAND_PRINT:
		{
			// TODO
			break;
		}
		case COMMAND_PRINTF:
		{
			// TODO
			break;
		}
		case COMMAND_SET_COLOR:
		{
			context.builtinUniforms.constantColor = *read<Colorf>(data, cmd.offset);
			gammaCorrectColor(context.builtinUniforms.constantColor);
			break;
		}
		case COMMAND_SET_SHADER:
		{
			context.state.shader = *read<Shader *>(data, cmd.offset);
			context.stateDiff |= STATEBIT_SHADER;
			break;
		}
		case COMMAND_SET_BLENDSTATE:
		{
			context.state.blend = *read<BlendState>(data, cmd.offset);
			context.stateDiff |= STATEBIT_BLEND;
			break;
		}
		case COMMAND_SET_STENCILSTATE:
		{
			context.state.stencil = *read<StencilState>(data, cmd.offset);
			context.stateDiff |= STATEBIT_STENCIL;
			break;
		}
		case COMMAND_SET_DEPTHSTATE:
		{
			context.state.depth = *read<DepthState>(data, cmd.offset);
			context.stateDiff |= STATEBIT_DEPTH;
			break;
		}
		case COMMAND_SET_SCISSOR:
		{
			context.state.scissor = *read<ScissorState>(data, cmd.offset);
			context.stateDiff |= STATEBIT_SCISSOR;
			break;
		}
		case COMMAND_SET_COLORMASK:
		{
			context.state.colorChannelMask = *read<ColorChannelMask>(data, cmd.offset);
			context.stateDiff |= STATEBIT_COLORMASK;
			break;
		}
		case COMMAND_SET_WIREFRAME:
		{
			context.state.wireframe = *read<bool>(data, cmd.offset);
			context.stateDiff |= STATEBIT_WIREFRAME;
			break;
		}
		}
	}

	gfx->flushStreamDraws();

	endPass(&context);

	if (tempDepthStencil)
		rts.depthStencil.canvas = nullptr;

	for (int i = 0; i < rts.colorCount; i++)
	{
		const auto &rt = rts.colors[i];
		if (rt.canvas->getMipmapMode() == Canvas::MIPMAPS_AUTO && rt.mipmap == 0)
			rt.canvas->generateMipmaps();
	}

	const auto &ds = rts.depthStencil;
	if (ds.canvas.get() && ds.canvas->getMipmapMode() == Canvas::MIPMAPS_AUTO && ds.mipmap == 0)
		ds.canvas->generateMipmaps();
}

void RenderPass::validateRenderTargets(Graphics *gfx, const RenderTargetSetup &rts) const
{
	const auto &caps = gfx->getCapabilities();
	RenderTarget firsttarget = rts.getFirstTarget();
	love::graphics::Canvas *firstcanvas = firsttarget.canvas;
	int ncolors = rts.colorCount;

	if (firstcanvas == nullptr)
		return;

	if (ncolors > caps.limits[Graphics::LIMIT_MULTI_CANVAS])
		throw love::Exception("This system can't simultaneously render to %d canvases.", ncolors);

	bool multiformatsupported = caps.features[Graphics::FEATURE_MULTI_CANVAS_FORMATS];

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
