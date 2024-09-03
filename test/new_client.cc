#include <selfupdate/updater.h>
#include <xl/log_setup>
#include <xl/native_string>
#include <xl/scope_exit>

int _tmain(int argc, const TCHAR *argv[]) {
  xl::log::setup(_T("new_client"));
  XL_ON_BLOCK_EXIT(xl::log::shutdown);
  XL_LOG_INFO("new_client launched.");

  if (selfupdate::IsNewVersionFirstLaunched(argc, argv)) {
    XL_LOG_INFO("This is the first launching since upgraded. Force updated: ", selfupdate::IsForceUpdated(argc, argv));
  } else {
    XL_LOG_INFO("This is an ordinary launching.");
  }

  return 0;
}
