#ifndef _NAV_INPUT_MEMORY_HPP_
#define _NAV_INPUT_MEMORY_HPP_

#include "nav/input.h"

namespace nav::input::memory
{

struct Memory
{
	uint8_t *data;
	size_t size, pos;
};

void populate(nav_input *input, void *buf, size_t size);

}

#endif /* _NAV_INPUT_MEMORY_HPP_ */
