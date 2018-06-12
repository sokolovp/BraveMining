#pragma once 

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class MiningVPNDummyFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("braveShields.dummy", UNKNOWN)

 protected:
  ~MiningVPNDummyFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions
