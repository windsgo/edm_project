#pragma once

#include "GCodeTask.h"

#include <memory>
#include <json.hpp>

#include <optional>

namespace edm
{

namespace task
{

class GCodeTaskConverter final {
private:
    // std::nullopt -> failed
    // nullptr -> ignored task
    static std::optional<GCodeTaskBase::ptr> _MakeGCodeTaskFromJsonObject(const json::object& jo);

public:
    static std::optional<std::vector<GCodeTaskBase::ptr>> MakeGCodeTaskListFromJson(const json::value& j);
};

} // namespace task
    
} // namespace edm
