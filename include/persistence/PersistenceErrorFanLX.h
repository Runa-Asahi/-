#pragma once
#include <stdexcept>
enum class PersistenceErrorCodeFanLX {
    InvalidData,
    UnsupportedSchema,
    Storage,
    Locked,
    RecoveryRequired,
    CommitIndeterminate
};
class PersistenceErrorFanLX : public std::runtime_error {
  public:
    PersistenceErrorCodeFanLX code;
    PersistenceErrorFanLX(PersistenceErrorCodeFanLX value, const std::string &message)
        : std::runtime_error(message), code(value) {}
};
