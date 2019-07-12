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
		case BLENDFACTOR_SRC1_COLOR: return GL_SRC1_COLOR;
		case BLENDFACTOR_ONE_MINUS_SRC1_COLOR: return GL_ONE_MINUS_SRC1_COLOR;
		case BLENDFACTOR_SRC1_ALPHA: return GL_SRC1_ALPHA;
		case BLENDFACTOR_ONE_MINUS_SRC1_ALPHA: return GL_ONE_MINUS_SRC1_ALPHA;
		case BLENDFACTOR_MAX_ENUM: return 0;
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

void RenderPass::beginPass(love::graphics::Graphics *gfx)
{
	
}

void RenderPass::endPass(love::graphics::Graphics *gfx)
{

}

void RenderPass::applyState(const RenderState &state, uint32 diff)
{
	if (diff & STATEBIT_SHADER)
	{
		gl.useProgram((GLuint) state.shader->getHandle());
	}

	// TODO: built-in uniforms (should that be here, or somewhere else?)

	if (diff & STATEBIT_COLOR)
	{
		Colorf c = state.color;
		gammaCorrectColor(c);
		glVertexAttrib4f(ATTRIB_CONSTANTCOLOR, c.r, c.g, c.b, c.a);
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

		Rect r = state.scissor.rect;

		if (renderTargets.getFirstTarget().canvas.get())
			glScissor(r.x, r.y, r.w, r.h);
		else
		{
			// With no Canvas active, we need to compensate for glScissor starting
			// from the lower left of the viewport instead of the top left.
			glScissor(r.x, 100 - (r.y + r.h), r.w, r.h); // TODO
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
			glDepthFunc(OpenGL::getGLCompareMode(depth.compare));
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
		GLenum glcompare = OpenGL::getGLCompareMode(getReversedCompareMode(stencil.compare));
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
		vertex::Winding winding = state.winding;
		if (renderTargets.getFirstTarget().canvas.get())
			winding = winding == vertex::WINDING_CW ? vertex::WINDING_CCW : vertex::WINDING_CW;

		glFrontFace(winding == vertex::WINDING_CW ? GL_CW : GL_CCW);
	}

	if (diff & STATEBIT_COLORMASK)
	{
		const auto mask = state.colorChannelMask;
		glColorMask(mask.r, mask.g, mask.b, mask.a);
	}

	if (diff & STATEBIT_WIREFRAME && !GLAD_ES_VERSION_2_0)
	{
		glPolygonMode(GL_FRONT_AND_BACK, state.wireframe ? GL_LINE : GL_FILL);
	}
}

} // opengl
} // graphics
} // love
