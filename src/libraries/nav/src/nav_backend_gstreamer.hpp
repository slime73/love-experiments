#ifndef _NAV_BACKEND_GSTREAMER_H_
#define _NAV_BACKEND_GSTREAMER_H_

#include "nav_config.hpp"

#ifdef NAV_BACKEND_GSTREAMER

#include "nav_internal.hpp"
#include "nav_backend.hpp"

namespace nav::gstreamer
{

Backend *create();

}

#endif /* NAV_BACKEND_GSTREAMER */
#endif /* _NAV_BACKEND_GSTREAMER_H_ */
