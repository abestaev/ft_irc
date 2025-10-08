#include <sstream>

std::string int_to_string(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}


