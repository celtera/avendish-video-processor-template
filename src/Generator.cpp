// The Generator implementation lives entirely in Generator.hpp (inline), because the Avendish
// back-ends compile the header directly and do not link this library. This
// translation unit only exists so the CMake target has a source to compile; put
// any non-inline / heavy helper code here if you ever need it.
#include "Generator.hpp"
