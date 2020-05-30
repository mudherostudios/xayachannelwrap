find_path(GAMECHANNEL_INCLUDE_DIR channelgame.hpp NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH PATHS "C:/msys64/mingw64/include/gamechannel/")
find_library(GAMECHANNEL_LIBRARY gamechannel NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH PATHS "C:/msys64/mingw64/lib/")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GAMECHANNEL DEFAULT_MSG GAMECHANNEL_LIBRARY GAMECHANNEL_INCLUDE_DIR)
