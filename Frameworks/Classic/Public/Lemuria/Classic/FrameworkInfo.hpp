#pragma once

#include <string_view>

namespace lemuria::classic {

// The name of this framework, shown by programs that report what they are built with.
[[nodiscard]] std::string_view framework_name() noexcept;

} // namespace lemuria::classic
