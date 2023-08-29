#include "test.h"

static const char* ut_basename_script = "\n\n\n"
"local ut = {}" LF
LF
"ut[" STRINGIFY(__LINE__) "] = { \"/usr/bin/sort\", \"sort\" }" LF
LF
"local file_path = [[" __FILE__ "]]" LF
"for k,v in pairs(ut) do" LF
"    local ret = infra.basename(v[1])" LF
"    test.assert_eq(ret, v[2]," LF
"        \"test case at %s:%d\", file_path, k)" LF
"end" LF
;

INFRA_TEST(basename, ut_basename_script);
