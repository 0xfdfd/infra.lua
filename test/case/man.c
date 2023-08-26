#include "test.h"

INFRA_TEST(man, "\n\n\n"
"-- Search for non-exist function." LF
"do" LF
"    local ret = infra.man(\"non_exist\")" LF
"    test.assert_eq(ret, nil)" LF
"end" LF
LF
"-- Search for exist function." LF
"do" LF
"    local ret = infra.man(\"man\")" LF
"    test.assert_eq(type(ret), \"string\")" LF
"end" LF
);
