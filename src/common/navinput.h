#ifndef LOVE_NAVINPUT_H
#define LOVE_NAVINPUT_H

// LOVE
#include "Stream.h"

// nav
#include "nav/nav.h"

namespace love
{

nav_input streamToNAVInput(love::Stream *stream);

}

#endif