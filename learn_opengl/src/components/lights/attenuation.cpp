#include "components/lights/attenuation.hpp"

#include "reflection/predefined.hpp"

namespace reflection {
void TypeReflector<Attenuation>::ReflectType(TypeHandle handle) {
  handle->name = "Attenuation";
  handle->guid = "FB0EDDB4-7D52-41B3-89EB-7294C25ECEEB";
  handle.Add<&Attenuation::constant>("constant");
  handle.Add<&Attenuation::linear>("linear");
  handle.Add<&Attenuation::quadratic>("quadratic");
}
}  // namespace reflection