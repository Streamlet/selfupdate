#pragma once

#include <xl/native_string>

namespace selfupdate {

bool InstallZipPackage(const xl::native_string &package_file, const xl::native_string &install_location);

} // namespace selfupdate
