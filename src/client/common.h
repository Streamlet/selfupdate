#include <system_error>
#include <xl/native_string>

namespace selfupdate {

enum selfupdate_error_code {
  SUE_OK = 0,
  SUE_NetworkError,
  SUE_PackageInfoFormatError,
  SUE_UnsupportedPackageFormat,
  SUE_UnsupportedHashAlgorithm,
  SUE_OpenFileError,
  SUE_PackageSizeError,
  SUE_PackageVerifyError,
  SUE_RunInstallerError,
  SUE_RunInstallerPositionError,
  SUE_PackageExtractError,
  SUE_MoveFileError,
  SUE_RunNewVersionError,

  SUE_Count,
};

struct SelfUpdateErrorMessage {
#ifdef _DEBUG
  selfupdate_error_code code;
#endif
  const char *message = nullptr;
};

extern const SelfUpdateErrorMessage selfupdate_error_message_[SUE_Count];

class selfupdate_error_category : public std::error_category {
  const char *name() const noexcept override;
  std::string message(int _Errval) const override;
};

std::error_category &selfupdate_category();

std::error_code make_selfupdate_error(selfupdate_error_code error);

#define PACKAGEINFO_PACKAGE_FORMAT_ZIP "zip"

#define PACKAGEINFO_PACKAGE_HASH_ALGO_MD5 "md5"
#define PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1 "sha1"
#define PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224 "sha224"
#define PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256 "sha256"
#define PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384 "sha384"
#define PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512 "sha512"

#define PACKAGE_NAME_VERSION_SEP "-"
#define FILE_NAME_EXT_SEP "."

#define INSTALLER_ARGUMENT_UPDATE "update"
#define INSTALLER_ARGUMENT_WAIT_PID "wait-pid"
#define INSTALLER_ARGUMENT_FORCE_UPDATE "force"
#define INSTALLER_ARGUMENT_SOURCE "source"
#define INSTALLER_ARGUMENT_TARGET "target"
#define INSTALLER_ARGUMENT_LAUNCH_FILE "launch-file"
#define INSTALLER_ARGUMENT_NEW_VERSION "new-version"

} // namespace selfupdate
