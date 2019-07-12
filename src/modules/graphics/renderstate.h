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

#include "common/int.h"

#include <vector>
#include <string>

namespace love
{
namespace graphics
{

enum BlendMode // High level wrappers
{
	BLEND_ALPHA,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_MULTIPLY,
	BLEND_LIGHTEN,
	BLEND_DARKEN,
	BLEND_SCREEN,
	BLEND_REPLACE,
	BLEND_NONE,
	BLEND_MAX_ENUM
};

enum BlendAlpha // High level wrappers
{
	BLENDALPHA_MULTIPLY,
	BLENDALPHA_PREMULTIPLIED,
	BLENDALPHA_MAX_ENUM
};

enum BlendFactor : uint8
{
	BLENDFACTOR_ZERO,
	BLENDFACTOR_ONE,
	BLENDFACTOR_SRC_COLOR,
	BLENDFACTOR_ONE_MINUS_SRC_COLOR,
	BLENDFACTOR_SRC_ALPHA,
	BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	BLENDFACTOR_DST_COLOR,
	BLENDFACTOR_ONE_MINUS_DST_COLOR,
	BLENDFACTOR_DST_ALPHA,
	BLENDFACTOR_ONE_MINUS_DST_ALPHA,
	BLENDFACTOR_SRC_ALPHA_SATURATED,
	BLENDFACTOR_SRC1_COLOR,
	BLENDFACTOR_ONE_MINUS_SRC1_COLOR,
	BLENDFACTOR_SRC1_ALPHA,
	BLENDFACTOR_ONE_MINUS_SRC1_ALPHA,
	BLENDFACTOR_MAX_ENUM
};

enum BlendOperation : uint8
{
	BLENDOP_ADD,
	BLENDOP_SUBTRACT,
	BLENDOP_REVERSE_SUBTRACT,
	BLENDOP_MIN,
	BLENDOP_MAX,
	BLENDOP_MAX_ENUM
};

enum StencilAction
{
	STENCIL_KEEP,
	STENCIL_ZERO,
	STENCIL_REPLACE,
	STENCIL_INCREMENT,
	STENCIL_DECREMENT,
	STENCIL_INCREMENT_WRAP,
	STENCIL_DECREMENT_WRAP,
	STENCIL_INVERT,
	STENCIL_MAX_ENUM
};

enum CompareMode
{
	COMPARE_LESS,
	COMPARE_LEQUAL,
	COMPARE_EQUAL,
	COMPARE_GEQUAL,
	COMPARE_GREATER,
	COMPARE_NOTEQUAL,
	COMPARE_ALWAYS,
	COMPARE_NEVER,
	COMPARE_MAX_ENUM
};

struct BlendState
{
	bool enable = false;
	BlendOperation operationRGB = BLENDOP_ADD;
	BlendOperation operationA = BLENDOP_ADD;
	BlendFactor srcFactorRGB = BLENDFACTOR_ONE;
	BlendFactor srcFactorA = BLENDFACTOR_ONE;
	BlendFactor dstFactorRGB = BLENDFACTOR_ZERO;
	BlendFactor dstFactorA = BLENDFACTOR_ZERO;
};

struct DepthState
{
	CompareMode compare = COMPARE_ALWAYS;
	bool write = false;
};

struct StencilState
{
	CompareMode compare = COMPARE_ALWAYS;
	StencilAction action = STENCIL_KEEP;
	int value = 0;
	uint32 readMask = 0xFFFFFFFF;
	uint32 writeMask = 0xFFFFFFFF;
};

struct ColorChannelMask
{
	bool r = true;
	bool g = true;
	bool b = true;
	bool a = true;
};

BlendState getBlendState(BlendMode mode, BlendAlpha alphamode);

/**
 * GPU APIs do the comparison in the opposite way of what makes sense for some
 * of love's APIs. For example in OpenGL if the compare function is GL_GREATER,
 * then the stencil test will pass if the reference value is greater than the
 * value in the stencil buffer. With our stencil API it's more intuitive to
 * assume that setStencilTest(COMPARE_GREATER, 4) will make it pass if the
 * stencil buffer has a value greater than 4.
 **/
CompareMode getReversedCompareMode(CompareMode mode);

bool getConstant(const char *in, BlendMode &out);
bool getConstant(BlendMode in, const char *&out);
std::vector<std::string> getConstants(BlendMode);

bool getConstant(const char *in, BlendAlpha &out);
bool getConstant(BlendAlpha in, const char *&out);
std::vector<std::string> getConstants(BlendAlpha);

bool getConstant(const char *in, StencilAction &out);
bool getConstant(StencilAction in, const char *&out);
std::vector<std::string> getConstants(StencilAction);

bool getConstant(const char *in, CompareMode &out);
bool getConstant(CompareMode in, const char *&out);
std::vector<std::string> getConstants(CompareMode);

} // graphics
} // love
