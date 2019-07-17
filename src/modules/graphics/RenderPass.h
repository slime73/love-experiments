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
#include "common/Object.h"
#include "common/math.h"
#include "common/Vector.h"
#include "common/int.h"
#include "common/Color.h"
#include "common/Optional.h"

#include "Drawable.h"
#include "renderstate.h"
#include "vertex.h"
#include "Polyline.h"

namespace love
{
namespace graphics
{

class Graphics;
class Texture;
class Quad;
class Canvas;
class Mesh;
class Shader;
class Font;

class RenderPass : public Object
{
public:

	enum DrawMode
	{
		DRAW_LINE,
		DRAW_FILL,
		DRAW_MAX_ENUM
	};

	enum ArcMode
	{
		ARC_OPEN,
		ARC_CLOSED,
		ARC_PIE,
		ARC_MAX_ENUM
	};

	enum StackType
	{
		STACK_ALL,
		STACK_TRANSFORM,
		STACK_MAX_ENUM
	};

	enum RenderTargetFlags
	{
		RT_TEMPORARY_DEPTH   = (1 << 0),
		RT_TEMPORARY_STENCIL = (1 << 1),
	};

	enum BeginAction
	{
		BEGIN_LOAD,
		BEGIN_CLEAR,
		BEGIN_DISCARD,
		BEGIN_MAX_ENUM
	};

	enum EndAction
	{
		END_STORE,
		END_DISCARD,
		END_MAX_ENUM
	};

	struct RenderTarget
	{
		StrongRef<Canvas> canvas;
		int slice = 0;
		int mipmap = 0;

		BeginAction beginAction = BEGIN_LOAD;
		EndAction endAction = END_STORE;
		Colorf clearColor;
		double clearDepth = 1.0;
		int clearStencil = 0;
	};

	static const int MAX_COLOR_RENDER_TARGETS = 8;

	struct RenderTargetSetup
	{
		RenderTarget colors[MAX_COLOR_RENDER_TARGETS] = {};
		int colorCount = 0;

		RenderTarget depthStencil;

		uint32 flags = 0;

		const RenderTarget &getFirstTarget() const
		{
			return colorCount > 0 ? colors[0] : depthStencil;
		}

		bool isBackbuffer() const
		{
			return getFirstTarget().canvas.get() == nullptr;
		}
	};

	RenderPass(Graphics *gfx, const RenderTargetSetup &rts);
	virtual ~RenderPass();

	void reset();
	void reset(Graphics *gfx, const RenderTargetSetup &rts);

	void execute(Graphics *gfx);

	void draw(Drawable *drawable, const Matrix4 &transform);
	void drawInstanced(Graphics *gfx, Mesh *mesh, const Matrix4 &transform, int instanceCount);

	void setColor(const Colorf &color);

	void setShader(Shader *shader);
	void setShader();

	void setBlendMode(BlendMode mode, BlendAlpha alphaMode);
	void setBlendState(const BlendState &state);

	void setStencil(CompareMode compare, StencilAction action, int value = 0, uint32 readMask = 0xFFFFFFFF, uint32 writeMask = 0xFFFFFFFF);
	void setStencil();

	void setDepthMode(CompareMode compare, bool write);
	void setDepthMode();

protected:

	enum CommandType
	{
		COMMAND_DRAW_DRAWABLE,
		COMMAND_DRAW_QUAD,
		COMMAND_DRAW_LAYER,
		COMMAND_DRAW_QUAD_LAYER,
		COMMAND_DRAW_MESH_INSTANCED,
		COMMAND_DRAW_POINTS,
		COMMAND_DRAW_LINE,
		COMMAND_DRAW_POLYGON,
		COMMAND_PRINT,
		COMMAND_PRINTF,
		COMMAND_SET_COLOR,
		COMMAND_SET_SHADER,
		COMMAND_SET_BLENDSTATE,
		COMMAND_SET_STENCILSTATE,
		COMMAND_SET_DEPTHSTATE,
		COMMAND_SET_SCISSOR,
		COMMAND_SET_COLORMASK,
		COMMAND_SET_WIREFRAME,
	};

	struct Command
	{
		CommandType type;
		int offset;
	};

	struct CommandDrawDrawable
	{
		Drawable *drawable;
		Matrix4 transform;
	};

	struct CommandDrawQuad
	{
		Texture *texture;
		Quad *quad;
		Matrix4 transform;
	};

	struct CommandDrawMeshInstanced
	{
		Mesh *mesh;
		int instanceCount;
		Matrix4 transform;
	};

	struct CommandDrawPoints
	{
		int count;
		float pointSize;
		bool colors;
		Vector2 positions[1]; // Actual size determined in points().
	};

	struct CommandDrawLine
	{
		int count;
		Vector2 positions[1]; // Actual size determined in line().
	};

	struct ScissorState
	{
		Rect rect;
		bool enable;
	};

	enum StateType
	{
		STATE_COLOR = 0,
		STATE_BLEND,
		STATE_SCISSOR,
		STATE_STENCIL,
		STATE_DEPTH,
		STATE_SHADER,
		STATE_COLORMASK,
		STATE_CULLMODE,
		STATE_FACEWINDING,
		STATE_WIREFRAME,
	};

	enum StateBit
	{
		STATEBIT_COLOR = 1 << STATE_COLOR,
		STATEBIT_BLEND = 1 << STATE_BLEND,
		STATEBIT_SCISSOR = 1 << STATE_SCISSOR,
		STATEBIT_STENCIL = 1 << STATE_STENCIL,
		STATEBIT_DEPTH = 1 << STATE_DEPTH,
		STATEBIT_SHADER = 1 << STATE_SHADER,
		STATEBIT_COLORMASK = 1 << STATE_COLORMASK,
		STATEBIT_CULLMODE = 1 << STATE_CULLMODE,
		STATEBIT_FACEWINDING = 1 << STATE_FACEWINDING,
		STATEBIT_WIREFRAME = 1 << STATE_WIREFRAME,
		STATEBIT_ALL = 0xFFFFFFFF
	};

	// State that affects the graphics backend.
	struct RenderState
	{
		Colorf color = Colorf(1.0, 1.0, 1.0, 1.0);

		BlendState blend;
		ScissorState scissor;
		StencilState stencil;
		DepthState depth;

		CullMode meshCullMode = CULL_NONE;
		vertex::Winding winding = vertex::WINDING_CCW;

		Shader *shader = nullptr;

		ColorChannelMask colorChannelMask;

		bool wireframe = false;
	};

	// All state, including high-level data that backends don't know about.
	struct GraphicsState
	{
		Font *font = nullptr;

		float lineWidth = 1.0f;
		Polyline::Style lineStyle = Polyline::STYLE_SMOOTH;
		Polyline::JoinType lineJoin = Polyline::JOIN_MITER;

		float pointSize = 1.0f;

		RenderState render;
	};

	template <typename T>
	T *addCommand(CommandType type, size_t size = sizeof(T), size_t alignment = alignof(T));

	virtual void beginPass(Graphics *gfx, bool isBackbuffer) = 0;
	virtual void endPass(Graphics *gfx, bool isBackbuffer) = 0;

	void validateRenderTargets(Graphics *gfx, const RenderTargetSetup &rts) const;

	RenderTargetSetup renderTargets;

	std::vector<Command> commands;
	uint8 *data;
	size_t dataSize;
	size_t currentOffset;

	std::vector<StrongRef<Object>> inputs;

	std::vector<GraphicsState> graphicsState;

}; // RenderPass

} // graphics
} // love
