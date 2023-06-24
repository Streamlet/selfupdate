#include <system_error>

namespace selfupdate {

enum selfupdate_error_code {
  SUE_OK = 0,
  SUE_NetworkError,
  SUE_PackageInfoFormatError,
  SUE_UnsupportedPackageFormat,
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

extern const char *PACKAGEINFO_PACKAGE_FORMAT_ZIP;

extern const char *PACKAGEINFO_PACKAGE_HASH_ALGO_MD5;
extern const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA1;
extern const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA224;
extern const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA256;
extern const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA384;
extern const char *PACKAGEINFO_PACKAGE_HASH_ALGO_SHA512;

extern const char *PACKAGE_NAME_VERSION_SEP;
extern const char *FILE_NAME_EXT_SEP;

extern const char *INSTALLER_ARGUMENT_UPDATE;
extern const char *INSTALLER_ARGUMENT_WAIT_PID;
extern const char *INSTALLER_ARGUMENT_SOURCE;
extern const char *INSTALLER_ARGUMENT_TARGET;
extern const char *INSTALLER_ARGUMENT_LAUNCH_FILE;

} // namespace selfupdate
