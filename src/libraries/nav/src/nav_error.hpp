#ifndef _NAV_ERROR_HPP_
#define _NAV_ERROR_HPP_

#include <exception>
#include <string>

namespace nav::error
{

const char *get();
void set(const std::string &err);
inline void set(const std::exception &e)
{
	set(e.what());
}

}

#endif /* _NAV_ERROR_HPP_ */
