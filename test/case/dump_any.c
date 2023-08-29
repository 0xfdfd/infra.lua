#include "test.h"

static const char* ut_compare_script = "\n\n\n"
"local ut = {}" LF
LF
"-- Dump number" LF
"ut[" STRINGIFY(__LINE__) "] = { 1, \"1\" }" LF
LF
"-- Get current file path" LF
"local file_path = [[" __FILE__ "]]" LF
"file_path = infra.basename(file_path)" LF
LF
"-- Let's do all the tests" LF
"for k,v in pairs(ut) do" LF
"    local ret = infra.dump_any(v[1])" LF
"    test.assert_eq(ret, v[2]," LF
"        \"test case at %s:%d\", file_path, k)" LF
"end" LF
;

INFRA_TEST(dump_any, ut_compare_script);
