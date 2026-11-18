#include "CustomFramework/CustomFramework.hpp"

namespace custom_framework {

lemuria::core::Result<lemuria::core::Version> read_required_engine(std::string_view text) {
    return lemuria::core::parse_version(text);
}

} // namespace custom_framework
