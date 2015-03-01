/** Copyright (C) 2013 David Braam - Released under terms of the AGPLv3 License */
#include "color.h"

using namespace Cura;

void flatColorCallback(ClipperLib::IntPoint& z1, ClipperLib::IntPoint& z2, ClipperLib::IntPoint& pt) {
    pt.Z = z1.Z;
}

