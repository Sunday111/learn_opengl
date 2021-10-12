#pragma once

#include <string_view>

#include "integer.hpp"

void SimpleTypeWidget(ui32 type_id, std::string_view name, void* value,
                      bool& value_changed);
void TypeIdWidget(ui32 type_id, void* base, bool& value_changed);