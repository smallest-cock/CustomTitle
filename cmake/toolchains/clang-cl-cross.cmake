set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

if(NOT DEFINED MSVC_WINE_ROOT)
    if(DEFINED ENV{MSVC_WINE_ROOT})
        set(MSVC_WINE_ROOT "$ENV{MSVC_WINE_ROOT}")
    else()
        set(MSVC_WINE_ROOT "$ENV{HOME}/msvc")
    endif()
endif()

file(GLOB _msvc_versions LIST_DIRECTORIES true "${MSVC_WINE_ROOT}/vc/tools/msvc/*")
if(NOT _msvc_versions)
    message(FATAL_ERROR "No MSVC toolset found under ${MSVC_WINE_ROOT}/vc/tools/msvc/")
endif()
list(SORT _msvc_versions COMPARE NATURAL ORDER DESCENDING)
list(GET _msvc_versions 0 _msvc_latest)
get_filename_component(MSVC_TOOLSET_VERSION "${_msvc_latest}" NAME)

file(GLOB _sdk_versions LIST_DIRECTORIES true "${MSVC_WINE_ROOT}/kits/10/include/*")
if(NOT _sdk_versions)
    message(FATAL_ERROR "No Windows SDK found under ${MSVC_WINE_ROOT}/kits/10/include/")
endif()
list(SORT _sdk_versions COMPARE NATURAL ORDER DESCENDING)
list(GET _sdk_versions 0 _sdk_latest)
get_filename_component(WINSDK_VERSION "${_sdk_latest}" NAME)

message(STATUS "clang-cl toolchain: MSVC ${MSVC_TOOLSET_VERSION}, SDK ${WINSDK_VERSION} (root: ${MSVC_WINE_ROOT})")

set(_msvc_inc "${MSVC_WINE_ROOT}/vc/tools/msvc/${MSVC_TOOLSET_VERSION}/include")
set(_msvc_lib "${MSVC_WINE_ROOT}/vc/tools/msvc/${MSVC_TOOLSET_VERSION}/lib/x64")
set(_sdk_inc  "${MSVC_WINE_ROOT}/kits/10/include/${WINSDK_VERSION}")
set(_sdk_lib  "${MSVC_WINE_ROOT}/kits/10/lib/${WINSDK_VERSION}")


# --- Fix known case-mismatched libs from the Windows SDK ---
# Some SDK libs ship under inconsistent casing (e.g. ShLwApi.lib /
# shlwapi.lib) that NTFS tolerates but a case-sensitive filesystem
# doesn't. Object files reference these by exact name via embedded
# #pragma comment(lib, "...") directives, so create a symlink with
# the exact requested casing next to the real file.
set(_sdk_lib_x64 "${_sdk_lib}/um/x64")
set(_known_lib_casing_fixes
    "shlwapi.lib=Shlwapi.lib"
)
foreach(_fix ${_known_lib_casing_fixes})
    string(REPLACE "=" ";" _fix_pair "${_fix}")
    list(GET _fix_pair 0 _real_name)
    list(GET _fix_pair 1 _wanted_name)
    set(_real_path "${_sdk_lib_x64}/${_real_name}")
    set(_wanted_path "${_sdk_lib_x64}/${_wanted_name}")
    if(EXISTS "${_real_path}" AND NOT EXISTS "${_wanted_path}")
        message(STATUS "Symlinking ${_wanted_name} -> ${_real_name} (SDK lib casing fix)")
        file(CREATE_LINK "${_real_path}" "${_wanted_path}" SYMBOLIC)
    endif()
endforeach()


# Bake include/lib paths into flags rather than process environment —
# env vars set here only apply to this configure-time CMake process,
# not to the separate ninja process tree spawned later by `cmake --build`.
set(_sysinclude_flags
    "-imsvc\"${_msvc_inc}\" -imsvc\"${_sdk_inc}/ucrt\" -imsvc\"${_sdk_inc}/shared\" -imsvc\"${_sdk_inc}/um\" -imsvc\"${_sdk_inc}/winrt\""
)
set(CMAKE_C_FLAGS_INIT   "${_sysinclude_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_sysinclude_flags}")

# llvm-rc's preprocessing pass (via clang-cl) needs the same include
# paths — CMake's cmake_llvm_rc wrapper doesn't inherit CXX flags.
set(CMAKE_RC_FLAGS_INIT
    "-I\"${_msvc_inc}\" -I\"${_sdk_inc}/ucrt\" -I\"${_sdk_inc}/shared\" -I\"${_sdk_inc}/um\" -I\"${_sdk_inc}/winrt\""
)

set(_libpath_flags
    "/libpath:\"${_msvc_lib}\" /libpath:\"${_sdk_lib}/ucrt/x64\" /libpath:\"${_sdk_lib}/um/x64\""
)
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${_libpath_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_libpath_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_libpath_flags}")

set(CMAKE_C_COMPILER   clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_LINKER   lld-link)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT       llvm-mt)
