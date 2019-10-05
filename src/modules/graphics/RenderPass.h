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

namespace math
{
class Transform;
}

namespace graphics
{

class Graphics;
class Texture;
class Quad;
class Canvas;
class Mesh;
class Shader;
class Font;

// The members in here must respect uniform buffer alignment/padding rules.
struct BuiltinUniformData
{
	Matrix4 transformMatrix;
	Matrix4 projectionMatrix;
	Vector4 screenSizeParams;
	Colorf constantColor;
};

struct DrawContext
{
	uint32 stateDiff = 0xFFFFFFFF;
	RenderState state;

	vertex::Attributes vertexAttributes;

	BuiltinUniformData builtinUniforms;

	bool isBackbuffer = false;
	int passWidth = 0;
	int passHeight = 0;
	int passPixelWidth = 0;
	int passPixelHeight = 0;
};

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
		RT_AUTO_DEPTH   = (1 << 0),
		RT_AUTO_STENCIL = (1 << 1),
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
		StrongRef<love::graphics::Canvas> canvas;
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

	Vector2 *polyline(int count);
	void polyline(const Vector2 *vertices, int count);

	void setColor(const Colorf &color);

	void setShader(Shader *shader);
	void setShader();

	void setBlendMode(BlendMode mode, BlendAlpha alphaMode);
	void setBlendState(const BlendState &state);

	void setStencil(CompareMode compare, StencilAction action, int value = 0, uint32 readMask = 0xFFFFFFFF, uint32 writeMask = 0xFFFFFFFF);
	void setStencil();

	void setDepthMode(CompareMode compare, bool write);
	void setDepthMode();

	void rotate(float r);
	void scale(float x, float y);
	void translate(float x, float y);
	void shear(float kx, float ky);
	void origin();

	void applyTransform(love::math::Transform *transform);
	void replaceTransform(love::math::Transform *transform);

	Vector2 transformPoint(Vector2 point);
	Vector2 inverseTransformPoint(Vector2 point);

	// Functions called by Drawable objects during execute().
	virtual void applyState(DrawContext *context) = 0;

	virtual void draw(PrimitiveType primType, int firstVertex, int vertexCount, int instanceCount) = 0;
	virtual void draw(PrimitiveType primType, int indexCount, int instanceCount, IndexDataType indexType, Resource *indexBuffer, size_t indexOffset) = 0;
	virtual void drawQuads(int start, int count, Resource *quadIndexBuffer) = 0;

protected:

	enum StateType
	{
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

	virtual void beginPass(DrawContext *context) = 0;
	virtual void endPass(DrawContext *context) = 0;

	RenderTargetSetup renderTargets;

private:

	enum CommandType
	{
		// Draw commands.
		COMMAND_DRAW_DRAWABLE,
		COMMAND_DRAW_QUAD,
		COMMAND_DRAW_LAYER,
		COMMAND_DRAW_QUAD_LAYER,
		COMMAND_DRAW_MESH_INSTANCED,
		COMMAND_DRAW_LINE,
		COMMAND_DRAW_POLYGON,
		COMMAND_PRINT,
		COMMAND_PRINTF,

		// Uniform data update commands.
		COMMAND_SET_COLOR,

		// Render state update commands.
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

	struct CommandDrawLine
	{
		Matrix4 transform;
		int count;
		Vector2 positions[1]; // Actual size determined in line().
	};

	struct CommandDrawPolygon
	{
		Matrix4 transform;
		int count;
		Vector2 positions[1]; // Actual size determined in polygon().
	};

	// All state, including high-level data that backends don't know about.
	struct GraphicsState
	{
		Font *font = nullptr;

		float lineWidth = 1.0f;
		Polyline::Style lineStyle = Polyline::STYLE_SMOOTH;
		Polyline::JoinType lineJoin = Polyline::JOIN_MITER;

		Colorf color;

		RenderState render;
	};

	template <typename T>
	T *addCommand(CommandType type, size_t size = sizeof(T), size_t alignment = alignof(T));

	void validateRenderTargets(Graphics *gfx, const RenderTargetSetup &rts) const;

	std::vector<Command> commands;
	uint8 *data;
	size_t dataSize;
	size_t currentOffset;

	std::vector<StrongRef<Object>> inputs;

	std::vector<GraphicsState> graphicsState;
	std::vector<Matrix4> transformState;

}; // RenderPass

} // graphics
} // love
