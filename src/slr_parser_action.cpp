#include "../include/slr_parser.hpp"

namespace slr {

std::string Action::to_string() const {
  switch (type) {
  case ActionType::SHIFT:
    return "s" + std::to_string(value);
  case ActionType::REDUCE:
    return "r" + std::to_string(value);
  case ActionType::ACCEPT:
    return "acc";
  case ActionType::ERROR:
    return "err";
  default:
    return "unknown";
  }
}

} // namespace slr