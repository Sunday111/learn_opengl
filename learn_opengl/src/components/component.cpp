#include "components/component.hpp"

#include "components/type_id_widget.hpp"

void Component::DrawDetails() {
  bool value_changed = false;
  TypeIdWidget(GetTypeGUID(), this, value_changed);
}