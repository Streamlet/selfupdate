#include "../include/selfupdate/installer.h"
#include "common.h"
#include <boost/program_options.hpp>

namespace selfupdate {

namespace {
template <typename charT>
bool IsNewVersionFirstLaunched(boost::program_options::basic_command_line_parser<charT> parser) {
  boost::program_options::options_description desc;
  // clang-format off
  desc.add_options()
    (INSTALLER_ARGUMENT_NEW_VERSION, "");
  // clang-format on
  boost::program_options::variables_map vm;
  try {
    auto locale = std::locale();
    std::locale::global(std::locale(""));
    boost::program_options::store(parser.options(desc).run(), vm);
    std::locale::global(locale);
  } catch (boost::program_options::unknown_option e) {
    return false;
  } catch (boost::program_options::invalid_option_value e) {
    return false;
  }
  boost::program_options::notify(vm);

  if (vm.count(INSTALLER_ARGUMENT_NEW_VERSION) > 0) {
    return true;
  }
  return false;
}

} // namespace

bool IsNewVersionFirstLaunched(int argc, const char *argv[]) {
  return IsNewVersionFirstLaunched(boost::program_options::basic_command_line_parser<char>(argc, argv));
}

bool IsNewVersionFirstLaunched(int argc, const wchar_t *argv[]) {
  return IsNewVersionFirstLaunched(boost::program_options::basic_command_line_parser<wchar_t>(argc, argv));
}

#ifdef _WIN32
bool IsNewVersionFirstLaunched(const char *command_line) {
  return IsNewVersionFirstLaunched(
      boost::program_options::basic_command_line_parser(boost::program_options::split_winmain(command_line)));
}
bool IsNewVersionFirstLaunched(const wchar_t *command_line) {
  return IsNewVersionFirstLaunched(
      boost::program_options::basic_command_line_parser(boost::program_options::split_winmain(command_line)));
}
#endif

} // namespace selfupdate