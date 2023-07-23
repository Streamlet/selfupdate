#include "../utility/log_init.h"
#include "../utility/native_string.h"
#include <selfupdate/selfupdate.h>

int _tmain(int argc, const TCHAR *argv[]) {
  logging::setup(_T("new_client"), LOG_LEVEL_INFO);
  logging::setup_from_file(_T("log_settings.ini"));
  LOG_INFO("new_client launched.");

  if (selfupdate::IsNewVersionFirstLaunched(argc, argv))
    LOG_INFO("This is the first launching since upgraded.");
  else
    LOG_INFO("This is an ordinary launching.");

  return 0;
}
