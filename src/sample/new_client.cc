#include "../include/selfupdate/selfupdate.h"
#include "../utility/native_string.h"
#include <iostream>

int _tmain(int argc, const TCHAR *argv[]) {
  if (selfupdate::IsNewVersionFirstLaunched(argc, argv))
    std::cout << "This is new client. Welcome!" << std::endl;
  else
    std::cout << "This is new client." << std::endl;

  return 0;
}
