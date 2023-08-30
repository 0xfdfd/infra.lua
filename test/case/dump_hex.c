#include "test.h"

static const char* ut_dump_hex_script = "\n\n\n"
"local ret = infra.dump_hex(\"hello world\")" LF
"test.assert_ne(ret, \"\")" LF
;

INFRA_TEST(dump_hex, ut_dump_hex_script);
