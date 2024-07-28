#include <algorithm>

#include "nav_input_memory.hpp"

namespace nav::input::memory
{

static void close(void **userdata)
{
	Memory **mem = (Memory**) userdata;
	delete *mem;
	*mem = nullptr;
}

static size_t read(void *userdata, void *dest, size_t size)
{
	Memory *mem = (Memory*) userdata;
	size_t nextpos = mem->pos + size;
	size_t readed;

	if (nextpos >= mem->size)
	{
		readed = size - (mem->size - nextpos);
		nextpos = mem->size;
	}
	else
		readed = size;

	if (readed > 0)
	{
		std::copy(mem->data + mem->pos, mem->data + mem->pos + readed, (uint8_t*) dest);
		mem->pos = nextpos;
	}

	return readed;
}

static nav_bool seek(void *userdata, uint64_t pos)
{
	Memory *mem = (Memory*) userdata;

	if (pos >= mem->size)
		mem->pos = mem->size;
	else
		mem->pos = pos;

	return true;
}

static uint64_t tell(void *userdata)
{
	Memory *mem = (Memory*) userdata;
	return (uint64_t) mem->pos;
}

static uint64_t fsize(void *userdata)
{
	Memory *mem = (Memory*) userdata;
	return (uint64_t) mem->size;
}

void populate(nav_input *input, void *buf, size_t size)
{
	Memory *mem = new Memory();
	mem->data = (uint8_t*) buf;
	mem->pos = 0;
	mem->size = size;

	input->userdata = mem;
	input->close = close;
	input->read = read;
	input->seek = seek;
	input->tell = tell;
	input->size = fsize;
}

}
