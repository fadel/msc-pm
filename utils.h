#include <limits>
#include <stdexcept>

/*
 * Credits to:
 * http://stackoverflow.com/questions/13150449/efficient-unsigned-to-signed-cast-avoiding-implementation-defined-behavior
 */
template<typename Uint, typename Int>
Int uintToInt(Uint x)
{
    if (x <= std::numeric_limits<Int>::max())
        return static_cast<Int>(x);

    if (x >= std::numeric_limits<Int>::min())
        return static_cast<Int>(x - std::numeric_limits<Int>::min())
             + std::numeric_limits<Int>::min();

    throw std::overflow_error("given value does not fit integer type");
}
