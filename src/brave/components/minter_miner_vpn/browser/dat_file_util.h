#pragma once

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "base/files/file_path.h"

namespace mining_vpn {

bool GetDATFileData(const base::FilePath& file_path,
    std::vector<unsigned char>& buffer);

bool SetDATFileData(const base::FilePath& file_path,
    std::string& buffer);
}  // namespace mining_vpn
