#pragma once
#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_PATCH 7
#define VERSION_BUILD 0

#define VERSION_SUFFIX "-hotfix" // can be "" if no suffix

#define stringify(a) stringify_(a)
#define stringify_(a) #a

#define VERSION_STR stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) VERSION_SUFFIX
