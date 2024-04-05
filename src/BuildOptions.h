#ifndef BUILDOPTS_H
#define BUILDOPTS_H

#include <BeBuild.h>

// Uncomment DEMO_MODE to turn this into the demo package version
enum
{
	NORMAL_MODE = 0,
	PREVIEW_MODE,
	BETA_MODE
};

#define BUILD_MODE NORMAL_MODE

// Uncomment ENTER_NAVIGATION to enable using the Enter key to navigate fields.
// It copies Quicken, but it is also not consistent in its behavior, which is NOT good
// #define ENTER_NAVIGATION

#endif
