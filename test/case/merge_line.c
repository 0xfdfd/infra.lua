#include "test.h"

INFRA_TEST(merge_line, "\n\n\n"
"do" LF
"    local src = \"hello\\r\\nworld\"" LF
"    local tmp = infra.split_line(src)" LF
"    local dst = infra.merge_line(tmp)" LF
"    test.assert_eq(dst, \"hello\\nworld\")" LF
"end" LF
);
