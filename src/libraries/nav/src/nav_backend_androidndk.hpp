#ifndef _NAV_BACKEND_ANDROIDNDK_HPP_
#define _NAV_BACKEND_ANDROIDNDK_HPP_

#include "nav_config.hpp"

#ifdef NAV_BACKEND_ANDROIDNDK

#include "nav_internal.hpp"
#include "nav_backend.hpp"

namespace nav::androidndk
{

Backend *create();

}

#endif /* NAV_BACKEND_ANDROIDNDK */

#endif /* _NAV_BACKEND_ANDROIDNDK_HPP_ */