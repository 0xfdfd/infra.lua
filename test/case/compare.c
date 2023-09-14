#include "test.h"

static const char* ut_compare_script = TEST_ALIGN_LINE
"local ds = {}" LF
LF
"-- Pure number compare" LF
"ds[" STRINGIFY(__LINE__) "] = { 1, 1, 0 }" LF
"ds[" STRINGIFY(__LINE__) "] = { 1, 2, -1 }" LF
"ds[" STRINGIFY(__LINE__) "] = { 2, 1, 1 }" LF
LF
"-- Pure string compare" LF
"ds[" STRINGIFY(__LINE__) "] = { \"hello\", \"hello\", 0 }" LF
"ds[" STRINGIFY(__LINE__) "] = { \"hello\", \"hello world\", -1 }" LF
"ds[" STRINGIFY(__LINE__) "] = { \"hellz\", \"hello\", 1 }" LF
LF
"-- Pure nil compare" LF
"ds[" STRINGIFY(__LINE__) "] = { nil, nil, 0 }" LF
LF
"-- Pure boolean compare" LF
"ds[" STRINGIFY(__LINE__) "] = { true, true, 0 }" LF
"ds[" STRINGIFY(__LINE__) "] = { false, false, 0 }" LF
"ds[" STRINGIFY(__LINE__) "] = { true, false, 1 }" LF
"ds[" STRINGIFY(__LINE__) "] = { false, true, -1 }" LF
LF
"-- nil always equal to nil, but less than others" LF
"ds[" STRINGIFY(__LINE__) "] = { nil, false, -1 }" LF
LF
"-- Number always less than String" LF
"ds[" STRINGIFY(__LINE__) "] = { 1, \"hello\", -1 }" LF
LF
"-- Let's do all the tests" LF
"local file_path = [[" __FILE__ "]]" LF
"for k,v in pairs(ds) do" LF
"    local ret = infra.compare(v[1], v[2])" LF
"    test.assert_eq(ret, v[3]," LF
"        \"test case at %s:%d\", file_path, k)" LF
"end" LF
"for k,v in pairs(ds) do" LF
"    local ret = infra.compare(v[2], v[1])" LF
"    test.assert_eq(ret, -v[3]," LF
"        \"test case at %s:%d\", file_path, k)" LF
"end" LF
;

INFRA_TEST(compare, ut_compare_script);
