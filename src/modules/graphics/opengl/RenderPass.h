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

#pragma once

// LOVE
#include "graphics/RenderPass.h"

namespace love
{
namespace graphics
{
namespace opengl
{

class RenderPass : public love::graphics::RenderPass
{
public:

	RenderPass(Graphics *gfx, const RenderTargetSetup &rts);
	virtual ~RenderPass();

	void applyState(DrawContext *context) override;

	void draw(PrimitiveType primType, int firstVertex, int vertexCount, int instanceCount) override;
	void draw(PrimitiveType primType, int indexCount, int instanceCount, IndexDataType indexType, Resource *indexBuffer, size_t indexOffset) override;
	void drawQuads(int start, int count, Resource *quadIndexBuffer) override;

private:

	enum PassState
	{
		PASS_BEGIN,
		PASS_END,
	};

	void beginPass(DrawContext *context) override;
	void endPass(DrawContext *context) override;

	void discardIfNeeded(PassState passState, bool isBackbuffer);

	vertex::Attributes currentAttributes;
	vertex::BufferBindings currentBuffers;

}; // RenderPass

} // opengl
} // graphics
} // love
