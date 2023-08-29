#include "test.h"

static const char* ut_compare_script = "\n\n\n"
"local ds = {}" LF
LF
"-- Compare number" LF
"ds[" STRINGIFY(__LINE__) "] = { 1, 1, 0 }" LF
"ds[" STRINGIFY(__LINE__) "] = { 1, 2, -1 }" LF
LF
"-- Compare string" LF
"ds[" STRINGIFY(__LINE__) "] = { \"hello\", \"hello\", 0 }" LF
"ds[" STRINGIFY(__LINE__) "] = { \"hello\", \"hello world\", -1 }" LF
"ds[" STRINGIFY(__LINE__) "] = { \"hellz\", \"hello\", 1 }" LF
LF
"-- nil always equal to each other" LF
"ds[" STRINGIFY(__LINE__) "] = { nil, nil, 0 }" LF
LF
"-- nil always less than boolean" LF
"ds[" STRINGIFY(__LINE__) "] = { nil, false, -1 }" LF
LF
"-- Number always less than String" LF
"ds[" STRINGIFY(__LINE__) "] = { 1, \"hello\", -1 }" LF
LF
"-- Let's do all the tests" LF
"for k,v in pairs(ds) do" LF
"    local file_path = [[" __FILE__ "]]" LF
"    test.assert_eq(" LF
"        infra.compare(v[1], v[2])," LF
"        v[3]," LF
"        \"locate at %s:%d\", file_path, k)" LF
"end" LF
;

INFRA_TEST(compare, ut_compare_script);
