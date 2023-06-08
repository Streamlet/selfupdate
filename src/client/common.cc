#include "common.h"
#include <system_error>

namespace selfupdate {

#ifdef _DEBUG
#define _(code, message) code, message
#else
#define _(code, message) message
#endif

const SelfUpdateErrorMessage selfupdate_error_message_[SUE_Count] = {
    {_(SUE_OK, "OK")},
    {_(SUE_PackageInfoFormatError, "package info format error")},
    {_(SUE_UnsupportedPackageFormat, "unsupported package format")},
    {_(SUE_PackageSizeError, "package size error")},
    {_(SUE_PackageVerifyError, "package verify error")},
    {_(SUE_RunInstallerError, "run installer error")},
    {_(SUE_RunInstallerPositionError, "run install position error")},
    {_(SUE_PackageExtractError, "package extract error")},
    {_(SUE_MoveFileError, "move file error")},
    {_(SUE_RunNewVersionError, "run new version error")},
};
#undef _

const char *selfupdate_error_category::name() const noexcept {
  return "selfupdate_error";
}

std::string selfupdate_error_category::message(int _Errval) const {
  if (_Errval >= 0 && _Errval < SUE_Count)
    return selfupdate_error_message_[_Errval].message;
  return "unknown error";
}

std::error_category &selfupdate_category() {
  static selfupdate_error_category instance;
  return instance;
}

std::error_code make_selfupdate_error(selfupdate_error_code error) {
  return std::error_code(error, selfupdate_category());
}

const char *PACKAGEINFO_PACKAGE_FORMAT_ZIP = "zip";

const char *PACKAGEINFO_PACKAGE_HASH_ALGO_MD5 = "md5";
const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1 = "sha1";
const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224 = "sha224";
const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256 = "sha256";
const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384 = "sha384";
const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512 = "sha512";

const char *PACKAGE_NAME_VERSION_SEP = "-";
const char *FILE_NAME_EXT_SEP = ".";

const char *INSTALLER_ARGUMENT_UPDATE = "update";
const char *INSTALLER_ARGUMENT_WAIT_PID = "wait_pid";
const char *INSTALLER_ARGUMENT_SOURCE = "source";
const char *INSTALLER_ARGUMENT_TARGET = "target";
const char *INSTALLER_ARGUMENT_LAUNCH_FILE = "launch_file";

} // namespace selfupdate
