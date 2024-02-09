#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include <json.hpp>

namespace edm {

namespace interpreter {

class GCodeCommandBase {
public:
    using ptr = std::shared_ptr<GCodeCommandBase>;

public:
    GCodeCommandBase() = default;
    GCodeCommandBase(const json::value& jv);

    GCodeCommandBase(const GCodeCommandBase&) = default;
    GCodeCommandBase& operator=(const GCodeCommandBase&) = default;
    GCodeCommandBase(GCodeCommandBase&&) = default;
    GCodeCommandBase& operator=(GCodeCommandBase&&) = default;

    virtual ~GCodeCommandBase() noexcept = default;

public:

protected:

};

} // namespace interpreter

} // namespace edm