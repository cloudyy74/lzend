#pragma once

#include <expected>
#include <string>

using Error = std::string;

template <typename T>
using Result = std::expected<T, Error>;
