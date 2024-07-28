#ifndef _NAV_INPUT_FILE_HPP_
#define _NAV_INPUT_FILE_HPP_

#include <string>

#include "nav/input.h"

namespace nav::input::file
{

bool populate(nav_input *input, const std::string &filename);

}

#endif /* _NAV_INPUT_FILE_HPP_ */
