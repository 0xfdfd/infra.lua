#include "test.h"

INFRA_TEST(dirname, "\n\n\n"
"local data_set = {}" LF
"data_set[\"/usr/bin/\"] = \"/usr\"" LF
"data_set[\"stdio.h\"] = \".\"" LF
LF
"for k,v in pairs(data_set) do" LF
"    local tmp = infra.dirname(k)" LF
"    test.assert_eq(tmp, v)" LF
"end" LF
);
