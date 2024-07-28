#include "nav_error.hpp"

static thread_local std::string err;

const char *nav::error::get()
{
	return err.empty() ? nullptr : err.c_str();
}

void nav::error::set(const std::string &str)
{
	err = str;
}
