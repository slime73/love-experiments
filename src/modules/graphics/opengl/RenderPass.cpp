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
#include "Graphics.h"
#include "Texture.h"
#include "OpenGL.h"

namespace love
{
namespace graphics
{
namespace opengl
{

static GLenum getGLBlendOperation(BlendOperation op)
{
	switch (op)
	{
		case BLENDOP_ADD: return GL_FUNC_ADD;
		case BLENDOP_SUBTRACT: return GL_FUNC_SUBTRACT;
		case BLENDOP_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
		case BLENDOP_MIN: return GL_MIN;
		case BLENDOP_MAX: return GL_MAX;
		case BLENDOP_MAX_ENUM: return 0;
	}
	return 0;
}

static GLenum getGLBlendFactor(BlendFactor factor)
{
	switch (factor)
	{
		case BLENDFACTOR_ZERO: return GL_ZERO;
		case BLENDFACTOR_ONE: return GL_ONE;
		case BLENDFACTOR_SRC_COLOR: return GL_SRC_COLOR;
		case BLENDFACTOR_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
		case BLENDFACTOR_SRC_ALPHA: return GL_SRC_ALPHA;
		case BLENDFACTOR_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
		case BLENDFACTOR_DST_COLOR: return GL_DST_COLOR;
		case BLENDFACTOR_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
		case BLENDFACTOR_DST_ALPHA: return GL_DST_ALPHA;
		case BLENDFACTOR_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
		case BLENDFACTOR_SRC_ALPHA_SATURATED: return GL_SRC_ALPHA_SATURATE;
		case BLENDFACTOR_MAX_ENUM: return 0;
	}
	return 0;
}

static GLenum getGLCompareMode(CompareMode mode)
{
	switch (mode)
	{
		case COMPARE_LESS: return GL_LESS;
		case COMPARE_LEQUAL: return GL_LEQUAL;
		case COMPARE_EQUAL: return GL_EQUAL;
		case COMPARE_GEQUAL: return GL_GEQUAL;
		case COMPARE_GREATER: return GL_GREATER;
		case COMPARE_NOTEQUAL: return GL_NOTEQUAL;
		case COMPARE_ALWAYS: return GL_ALWAYS;
		case COMPARE_NEVER: return GL_NEVER;
		case COMPARE_MAX_ENUM: return 0;
	}
	return 0;
}

static GLenum getGLStencilAction(StencilAction action)
{
	switch (action)
	{
		case STENCIL_KEEP: return GL_KEEP;
		case STENCIL_ZERO: return GL_ZERO;
		case STENCIL_REPLACE: return GL_REPLACE;
		case STENCIL_INCREMENT: return GL_INCR;
		case STENCIL_DECREMENT: return GL_DECR;
		case STENCIL_INCREMENT_WRAP: return GL_INCR_WRAP;
		case STENCIL_DECREMENT_WRAP: return GL_DECR_WRAP;
		case STENCIL_INVERT: return GL_INVERT;
		case STENCIL_MAX_ENUM: return 0;
	}
	return 0;
}

static GLenum getGLPrimitiveType(PrimitiveType type)
{
	switch (type)
	{
		case PRIMITIVE_TRIANGLES: return GL_TRIANGLES;
		case PRIMITIVE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
		case PRIMITIVE_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
		case PRIMITIVE_POINTS: return GL_POINTS;
		case PRIMITIVE_MAX_ENUM: return 0;
	}
	return 0;
}

static GLenum getGLIndexDataType(IndexDataType type)
{
	switch (type)
	{
		case INDEX_UINT16: return GL_UNSIGNED_SHORT;
		case INDEX_UINT32: return GL_UNSIGNED_INT;
		case INDEX_MAX_ENUM: return 0;
	}
	return 0;
}

RenderPass::RenderPass(love::graphics::Graphics *gfx, const RenderPassAttachments &rts)
	: love::graphics::RenderPass(gfx, rts)
{
}

RenderPass::~RenderPass()
{
}

bool RenderPass::shouldDiscard(const RenderPassAttachment &rt, PassState passState) const
{
	if (passState == PASS_BEGIN)
		return rt.beginAction == RenderPassAttachment::BEGIN_DISCARD;
	else if (passState == PASS_END)
		return rt.endAction == RenderPassAttachment::END_DISCARD;
	return false;
}

void RenderPass::discardIfNeeded(DrawContext *context, PassState passState)
{
	if (!(GLAD_VERSION_4_3 || GLAD_ARB_invalidate_subdata || GLAD_ES_VERSION_3_0 || GLAD_EXT_discard_framebuffer))
		return;

	const auto &rts = context->renderTargets;

	GLenum attachments[RenderPassAttachments::MAX_COLOR_RENDER_TARGETS + 2];
	int attachmentcount = 0;

	// glDiscardFramebuffer uses different attachment enums for the default FBO.
	bool defaultFBO0 = context->isBackbuffer && gl.getDefaultFBO() == 0;

	GLenum colorname = defaultFBO0 ? GL_COLOR : GL_COLOR_ATTACHMENT0;
	GLenum depthname = defaultFBO0 ? GL_DEPTH : GL_DEPTH_ATTACHMENT;
	GLenum stencilname = defaultFBO0 ? GL_STENCIL : GL_STENCIL_ATTACHMENT;

	for (int i = 0; i < rts.colorCount; i++)
	{
		if (shouldDiscard(rts.colors[i], passState))
			attachments[attachmentcount++] = colorname + i;
	}

	if (shouldDiscard(rts.depthStencil, passState))
	{
		attachments[attachmentcount++] = depthname;
		attachments[attachmentcount++] = stencilname;
	}

	if (attachmentcount == 0)
		return;
	else if (GLAD_VERSION_4_3 || GLAD_ARB_invalidate_subdata || GLAD_ES_VERSION_3_0)
		glInvalidateFramebuffer(GL_FRAMEBUFFER, attachmentcount, attachments);
	else if (GLAD_EXT_discard_framebuffer)
		glDiscardFramebufferEXT(GL_FRAMEBUFFER, attachmentcount, attachments);
}

void RenderPass::beginPass(DrawContext *context)
{
	typedef RenderPassAttachment Attachment;

	const auto &rts = context->renderTargets;

	int w = context->passWidth;
	int h = context->passHeight;

	Matrix4 projection;
	GLuint fbo = 0;

	if (context->isBackbuffer)
	{
		fbo = gl.getDefaultFBO();

		// The projection matrix is flipped compared to rendering to a canvas,
		// due to OpenGL considering (0,0) bottom-left instead of top-left.
		projection = Matrix4::ortho(0.0, (float) w, (float) h, 0.0, -10.0f, 10.0f);
	}
	else
	{
		OpenGL::CachedRenderTargets cachedRTs;

		for (int i = 0; i < rts.colorCount; i++)
		{
			const auto &rt = rts.colors[i];
			cachedRTs.add(rt.texture.get(), rt.slice, rt.mipmap);
		}

		const auto &ds = rts.depthStencil;
		if (ds.texture.get())
			cachedRTs.add(ds.texture.get(), ds.slice, ds.mipmap);

		fbo = gl.getCachedFBO(cachedRTs);

		projection = Matrix4::ortho(0.0, (float) w, 0.0, (float) h, -10.0f, 10.0f);
	}

	context->builtinUniforms.projectionMatrix = projection;

	gl.bindFramebuffer(OpenGL::FRAMEBUFFER_ALL, fbo);
	gl.setViewport({0, 0, context->passPixelWidth, context->passPixelHeight});

	// Make sure the correct sRGB setting is used when drawing to the canvases.
	if (GLAD_VERSION_1_0 || GLAD_EXT_sRGB_write_control)
	{
		bool hasSRGBcanvas = context->isBackbuffer && isGammaCorrect();

		for (int i = 0; i < rts.colorCount; i++)
		{
			auto c = rts.colors[i].texture.get();
			if (c != nullptr && c->getPixelFormat() == PIXELFORMAT_sRGBA8_UNORM)
			{
				hasSRGBcanvas = true;
				break;
			}
		}

		if (hasSRGBcanvas != gl.isStateEnabled(OpenGL::ENABLE_FRAMEBUFFER_SRGB))
			gl.setEnableState(OpenGL::ENABLE_FRAMEBUFFER_SRGB, hasSRGBcanvas);
	}

	// glScissor affects glClear.
	if (gl.isStateEnabled(OpenGL::ENABLE_SCISSOR_TEST))
	{
		gl.setEnableState(OpenGL::ENABLE_SCISSOR_TEST, false);
		context->stateDiff |= STATEBIT_SCISSOR;
	}

	GLbitfield clearFlags = 0;

	if (rts.colorCount == 1 && rts.colors[0].beginAction == Attachment::BEGIN_CLEAR)
	{
		Colorf c = gammaCorrectColor(rts.colors[0].clearColor);
		glClearColor(c.r, c.g, c.b, c.a);
		clearFlags |= GL_COLOR_BUFFER_BIT;
	}
	else if (rts.colorCount > 0)
	{
		bool drawbuffersmodified = false;

		for (int i = 0; i < rts.colorCount; i++)
		{
			if (rts.colors[i].beginAction != Attachment::BEGIN_CLEAR)
				continue;

			Colorf c = gammaCorrectColor(rts.colors[i].clearColor);

			if (GLAD_ES_VERSION_3_0 || GLAD_VERSION_3_0)
			{
				const GLfloat carray[] = {c.r, c.g, c.b, c.a};
				glClearBufferfv(GL_COLOR, i, carray);
			}
			else
			{
				glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
				glClearColor(c.r, c.g, c.b, c.a);
				glClear(GL_COLOR_BUFFER_BIT);

				drawbuffersmodified = true;
			}
		}

		// Revert to the expected draw buffers once we're done, if glClearBuffer
		// wasn't supported.
		if (drawbuffersmodified)
		{
			GLenum bufs[MAX_COLOR_RENDER_TARGETS];
			for (int i = 0; i < rts.colorCount; i++)
				bufs[i] = GL_COLOR_ATTACHMENT0 + i;

			glDrawBuffers(rts.colorCount, bufs);
		}
	}

	if (rts.depthStencil.beginAction == Attachment::BEGIN_CLEAR)
	{
		// TODO: backbuffer depth/stencil pixel format.
		PixelFormat format = PIXELFORMAT_UNKNOWN;
		if (rts.depthStencil.texture.get())
			format = rts.depthStencil.texture->getPixelFormat();
		else if (context->isBackbuffer)
			format = PIXELFORMAT_DEPTH24_UNORM_STENCIL8;;

		if (isPixelFormatDepth(format))
		{
			// glDepthMask affects glClear.
			if (!gl.hasDepthWrites())
			{
				gl.setDepthWrites(true);
				context->stateDiff |= STATEBIT_DEPTH;
			}

			gl.clearDepth(rts.depthStencil.clearDepth);
			clearFlags |= GL_DEPTH_BUFFER_BIT;
		}

		if (isPixelFormatStencil(format))
		{
			// glStencilMask affects glClear.
			glStencilMask(0xFFFFFFFF);
			context->stateDiff |= STATEBIT_STENCIL;

			glClearStencil(rts.depthStencil.clearStencil);
			clearFlags |= GL_STENCIL_BUFFER_BIT;
		}
	}

	if (clearFlags != 0)
		glClear(clearFlags);

	discardIfNeeded(context, PASS_BEGIN);

	if (gl.bugs.clearRequiresDriverTextureStateUpdate && Shader::current)
	{
		// This seems to be enough to fix the bug for me. Other methods I've
		// tried (e.g. dummy draws) don't work in all cases.
		gl.useProgram(0);
		gl.useProgram((GLuint) Shader::current->getHandle());
	}

	// The behaviour of these depend on whether the backbuffer is active.
	context->stateDiff |= STATEBIT_SCISSOR | STATEBIT_FACEWINDING;
}

void RenderPass::endPass(DrawContext *context)
{
	const auto &rts = context->renderTargets;
	auto depthstencil = rts.depthStencil.texture.get();

	discardIfNeeded(context, PASS_END);

	// Resolve MSAA buffers. MSAA is only supported for 2D render targets so we
	// don't have to worry about resolving to slices.
	if (!context->isBackbuffer && rts.colorCount > 0 && rts.colors[0].texture->getMSAA() > 1)
	{
		int mip = rts.colors[0].mipmap;
		int w = rts.colors[0].texture->getPixelWidth(mip);
		int h = rts.colors[0].texture->getPixelHeight(mip);

		for (int i = 0; i < rts.colorCount; i++)
		{
			Texture *c = (Texture *) rts.colors[i].texture.get();

			if (!c->isReadable())
				continue;

			glReadBuffer(GL_COLOR_ATTACHMENT0 + i);

			gl.bindFramebuffer(OpenGL::FRAMEBUFFER_DRAW, c->getFBO());

			if (GLAD_APPLE_framebuffer_multisample)
				glResolveMultisampleFramebufferAPPLE();
			else
				glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

	if (depthstencil != nullptr && depthstencil->getMSAA() > 1 && depthstencil->isReadable())
	{
		gl.bindFramebuffer(OpenGL::FRAMEBUFFER_DRAW, ((Texture *) depthstencil)->getFBO());

		if (GLAD_APPLE_framebuffer_multisample)
			glResolveMultisampleFramebufferAPPLE();
		else
		{
			int mip = rts.depthStencil.mipmap;
			int w = depthstencil->getPixelWidth(mip);
			int h = depthstencil->getPixelHeight(mip);
			PixelFormat format = depthstencil->getPixelFormat();

			GLbitfield mask = 0;
			if (isPixelFormatDepth(format))
				mask |= GL_DEPTH_BUFFER_BIT;
			if (isPixelFormatStencil(format))
				mask |= GL_STENCIL_BUFFER_BIT;

			glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, mask, GL_NEAREST);
		}
	}
}

void RenderPass::applyState(DrawContext *context)
{
	// TODO: built-in uniforms (should that be here, or somewhere else?)
	// TODO: vertex buffers?
	// TODO: texture bindings?

	currentAttributes = context->vertexAttributes;

	uint32 diff = context->stateDiff;
	const RenderState &state = context->state;

	if (diff & STATEBIT_SHADER)
	{
		gl.useProgram((GLuint) state.shader->getHandle());
	}

	if (diff & STATEBIT_BLEND)
	{
		GLenum opRGB  = getGLBlendOperation(state.blend.operationRGB);
		GLenum opA    = getGLBlendOperation(state.blend.operationA);
		GLenum srcRGB = getGLBlendFactor(state.blend.srcFactorRGB);
		GLenum srcA   = getGLBlendFactor(state.blend.srcFactorA);
		GLenum dstRGB = getGLBlendFactor(state.blend.dstFactorRGB);
		GLenum dstA   = getGLBlendFactor(state.blend.dstFactorA);

		glBlendEquationSeparate(opRGB, opA);
		glBlendFuncSeparate(srcRGB, dstRGB, srcA, dstA);
	}

	if (diff & STATEBIT_SCISSOR)
	{
		if (state.scissor.enable != gl.isStateEnabled(OpenGL::ENABLE_SCISSOR_TEST))
			gl.setEnableState(OpenGL::ENABLE_SCISSOR_TEST, state.scissor.enable);

		if (state.scissor.enable)
		{
			Rect r = state.scissor.rect;

			// With no Canvas active, compensate for glScissor starting from the
			// lower left of the viewport instead of the top left.
			if (context->isBackbuffer)
				r.y = context->passPixelHeight - (r.y + r.h);

			glScissor(r.x, r.y, r.w, r.h);
		}
	}

	if (diff & STATEBIT_DEPTH)
	{
		const auto &depth = state.depth;
		bool depthenable = depth.compare != COMPARE_ALWAYS || depth.write;

		if (depthenable != gl.isStateEnabled(OpenGL::ENABLE_DEPTH_TEST))
			gl.setEnableState(OpenGL::ENABLE_DEPTH_TEST, depthenable);

		if (depthenable)
		{
			glDepthFunc(getGLCompareMode(depth.compare));
			gl.setDepthWrites(depth.write);
		}
	}

	if (diff & STATEBIT_STENCIL)
	{
		const auto &stencil = state.stencil;
		bool stencilenable = stencil.compare != COMPARE_ALWAYS || stencil.action != STENCIL_KEEP;

		// The stencil test must be enabled in order to write to the stencil buffer.
		if (stencilenable != gl.isStateEnabled(OpenGL::ENABLE_STENCIL_TEST))
			gl.setEnableState(OpenGL::ENABLE_STENCIL_TEST, stencilenable);

		/**
		 * OpenGL / GPUs do the comparison in the opposite way that makes sense
		 * for this API. For example, if the compare function is GL_GREATER then the
		 * stencil test will pass if the reference value is greater than the value
		 * in the stencil buffer. With our API it's more intuitive to assume that
		 * setStencilTest(COMPARE_GREATER, 4) will make it pass if the stencil
		 * buffer has a value greater than 4.
		 **/
		GLenum glcompare = getGLCompareMode(getReversedCompareMode(stencil.compare));
		GLenum glaction = getGLStencilAction(stencil.action);

		glStencilFunc(glcompare, stencil.value, stencil.readMask);
		glStencilOp(GL_KEEP, GL_KEEP, glaction);
		glStencilMask(stencil.writeMask);
	}

	if (diff & STATEBIT_CULLMODE)
	{
		gl.setCullMode(state.meshCullMode);
	}

	if (diff & STATEBIT_FACEWINDING)
	{
		Winding winding = state.winding;
		if (!context->isBackbuffer)
			winding = winding == WINDING_CW ? WINDING_CCW : WINDING_CW;

		glFrontFace(winding == WINDING_CW ? GL_CW : GL_CCW);
	}

	if (diff & STATEBIT_COLORMASK)
	{
		const auto mask = state.colorChannelMask;
		glColorMask(mask.r, mask.g, mask.b, mask.a);
	}

	if ((diff & STATEBIT_WIREFRAME) && !GLAD_ES_VERSION_2_0)
	{
		glPolygonMode(GL_FRONT_AND_BACK, state.wireframe ? GL_LINE : GL_FILL);
	}

	context->stateDiff = 0;
}

void RenderPass::draw(PrimitiveType primType, int firstVertex, int vertexCount, int instanceCount)
{
	gl.setVertexAttributes(currentAttributes, currentBuffers);

	GLenum glprimtype = getGLPrimitiveType(primType);

	if (instanceCount > 1)
		glDrawArraysInstanced(glprimtype, firstVertex, vertexCount, instanceCount);
	else
		glDrawArrays(glprimtype, firstVertex, vertexCount);
}

void RenderPass::draw(PrimitiveType primType, int indexCount, int instanceCount, IndexDataType indexType, Resource *indexBuffer, size_t indexOffset)
{
	gl.setVertexAttributes(currentAttributes, currentBuffers);

	const void *gloffset = BUFFER_OFFSET(indexOffset);
	GLenum glprimtype = getGLPrimitiveType(primType);
	GLenum gldatatype = getGLIndexDataType(indexType);

	gl.bindBuffer(BUFFERUSAGE_INDEX, indexBuffer->getHandle());

	if (instanceCount > 1)
		glDrawElementsInstanced(glprimtype, indexCount, gldatatype, gloffset, instanceCount);
	else
		glDrawElements(glprimtype, indexCount, gldatatype, gloffset);
}

static inline void advanceVertexOffsets(const VertexAttributes &attributes, BufferBindings &buffers, int vertexcount)
{
	uint32 touchedbuffers = 0;

	for (unsigned int i = 0; i < VertexAttributes::MAX; i++)
	{
		if (!attributes.isEnabled(i))
			continue;

		auto &attrib = attributes.attribs[i];

		uint32 bufferbit = 1u << attrib.bufferIndex;
		if ((touchedbuffers & bufferbit) == 0)
		{
			touchedbuffers |= bufferbit;
			const auto &layout = attributes.bufferLayouts[attrib.bufferIndex];
			buffers.info[attrib.bufferIndex].offset += layout.stride * vertexcount;
		}
	}
}

void RenderPass::drawQuads(int start, int count, Resource *quadIndexBuffer)
{
	const int MAX_VERTICES_PER_DRAW = LOVE_UINT16_MAX;
	const int MAX_QUADS_PER_DRAW    = MAX_VERTICES_PER_DRAW / 4;

	gl.bindBuffer(BUFFERUSAGE_INDEX, quadIndexBuffer->getHandle());

	if (gl.isBaseVertexSupported())
	{
		gl.setVertexAttributes(currentAttributes, currentBuffers);

		int basevertex = start * 4;

		for (int quadindex = 0; quadindex < count; quadindex += MAX_QUADS_PER_DRAW)
		{
			int quadcount = std::min(MAX_QUADS_PER_DRAW, count - quadindex);

			glDrawElementsBaseVertex(GL_TRIANGLES, quadcount * 6, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0), basevertex);

			basevertex += quadcount * 4;
		}
	}
	else
	{
		BufferBindings bufferscopy = currentBuffers;
		if (start > 0)
			advanceVertexOffsets(currentAttributes, bufferscopy, start * 4);

		for (int quadindex = 0; quadindex < count; quadindex += MAX_QUADS_PER_DRAW)
		{
			gl.setVertexAttributes(currentAttributes, bufferscopy);

			int quadcount = std::min(MAX_QUADS_PER_DRAW, count - quadindex);

			glDrawElements(GL_TRIANGLES, quadcount * 6, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

			if (count > MAX_QUADS_PER_DRAW)
				advanceVertexOffsets(currentAttributes, bufferscopy, quadcount * 4);
		}
	}
}

} // opengl
} // graphics
} // love
