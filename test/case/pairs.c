#include "test.h"

static const char* ut_pairs_script = TEST_ALIGN_LINE
"local t = {" LF
"    a = \"hello\"," LF
"    b = \"world\"," LF
"}" LF
"t[9] = \"asdf\"" LF
"t[0] = \"qwerty\"" LF
LF
"local arr_1 = {}"
"for k,v in pairs(t) do" LF
"    table.insert(arr_1, k)" LF
"end" LF
"local arr_2 = {}" LF
"for k,v in infra.pairs(t) do" LF
"    table.insert(arr_2, k)" LF
"end" LF
"for k,v in ipairs(arr_1) do" LF
"    test.assert_eq(arr_1[k], arr_2[k])" LF
"end" LF
;

INFRA_TEST(pairs, ut_pairs_script);
