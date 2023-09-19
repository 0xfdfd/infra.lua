#include "test.h"

static const char* ut_range_script = TEST_ALIGN_LINE
"local r = infra.range(1, 100, 2)" LF
LF
"do" LF
"    test.assert_eq(r[1], true)" LF
"    test.assert_eq(r[2], false)" LF
"    test.assert_eq(r[3], true)" LF
"    test.assert_eq(r[4], false)" LF
"end" LF
LF
"do" LF
"    local v = 1" LF
"    for k in infra.pairs(r) do" LF
"        test.assert_eq(k, v)" LF
"        v = v + 2" LF
"    end" LF
"end" LF
;

INFRA_TEST(range, ut_range_script);
