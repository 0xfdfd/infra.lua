#include "test.h"

INFRA_TEST(strcasecmp, "\n\n\n"
"do" LF
"    local s1 = \"hello world\"" LF
"    local s2 = \"hello\"" LF
"    local ret = infra.strcasecmp(s1, s2)" LF
"    test.assert_ne(ret, 0)" LF
"end" LF
);
