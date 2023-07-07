#include "../include/selfupdate/selfupdate.h"
#include <iostream>

int main(int argc, const char *argv[]) {
  if (selfupdate::IsNewVersionFirstLaunched(argc, argv))
    std::cout << "This is new client. Welcome!" << std::endl;
  else
    std::cout << "This is new client." << std::endl;

  return 0;
}
