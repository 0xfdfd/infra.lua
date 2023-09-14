#ifndef __TEST_H__
#define __TEST_H__

#if defined(_WIN32)
#include <Windows.h>
#endif

#include "cutest.h"

#define INFRA_TEST(name, script)    \
    TEST(infra, name) {\
        test_run_script(#name, script);\
    }

#define LF  "\n"
#define STRINGIFY(x)    STRINGIFY2(x)
#define STRINGIFY2(x)   #x

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

#define TEST_ALIGN_LINE     TEST_REPLAT(LF, __LINE__)

#define TEST_REPLAT(s, x)   TEST_REPLAT_(s, x)
#define TEST_REPLAT_(s, x)  TEST_REPLAT_##x(s)

#define TEST_REPLAT_1(x)    x
#define TEST_REPLAT_2(x)    TEST_REPLAT_1(x) x
#define TEST_REPLAT_3(x)    TEST_REPLAT_2(x) x
#define TEST_REPLAT_4(x)    TEST_REPLAT_3(x) x
#define TEST_REPLAT_5(x)    TEST_REPLAT_4(x) x
#define TEST_REPLAT_6(x)    TEST_REPLAT_5(x) x
#define TEST_REPLAT_7(x)    TEST_REPLAT_6(x) x
#define TEST_REPLAT_8(x)    TEST_REPLAT_7(x) x
#define TEST_REPLAT_9(x)    TEST_REPLAT_8(x) x
#define TEST_REPLAT_10(x)   TEST_REPLAT_9(x) x
#define TEST_REPLAT_11(x)   TEST_REPLAT_10(x) x
#define TEST_REPLAT_12(x)   TEST_REPLAT_11(x) x
#define TEST_REPLAT_13(x)   TEST_REPLAT_12(x) x
#define TEST_REPLAT_14(x)   TEST_REPLAT_13(x) x
#define TEST_REPLAT_15(x)   TEST_REPLAT_14(x) x
#define TEST_REPLAT_16(x)   TEST_REPLAT_15(x) x
#define TEST_REPLAT_17(x)   TEST_REPLAT_16(x) x
#define TEST_REPLAT_18(x)   TEST_REPLAT_17(x) x
#define TEST_REPLAT_19(x)   TEST_REPLAT_18(x) x
#define TEST_REPLAT_20(x)   TEST_REPLAT_19(x) x
#define TEST_REPLAT_21(x)   TEST_REPLAT_20(x) x
#define TEST_REPLAT_22(x)   TEST_REPLAT_21(x) x
#define TEST_REPLAT_23(x)   TEST_REPLAT_22(x) x
#define TEST_REPLAT_24(x)   TEST_REPLAT_23(x) x
#define TEST_REPLAT_25(x)   TEST_REPLAT_24(x) x
#define TEST_REPLAT_26(x)   TEST_REPLAT_25(x) x
#define TEST_REPLAT_27(x)   TEST_REPLAT_26(x) x
#define TEST_REPLAT_28(x)   TEST_REPLAT_27(x) x
#define TEST_REPLAT_29(x)   TEST_REPLAT_28(x) x
#define TEST_REPLAT_30(x)   TEST_REPLAT_29(x) x
#define TEST_REPLAT_31(x)   TEST_REPLAT_30(x) x
#define TEST_REPLAT_32(x)   TEST_REPLAT_31(x) x

#endif
