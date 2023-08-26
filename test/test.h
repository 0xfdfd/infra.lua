#ifndef __TEST_H__
#define __TEST_H__

#include "cutest.h"

#define INFRA_TEST(name, script)    \
    TEST(infra, name) {\
        test_run_script(#name, script);\
    }

#define LF  "\n"

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void test_run_script(const char* name, const char* script);

#ifdef __cplusplus
}
#endif

#endif
