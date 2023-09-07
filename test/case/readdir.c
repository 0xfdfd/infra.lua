#include "test.h"

static const char* ut_readdir_script = "\n\n\n"
"local ret = infra.readdir(\".\")" LF
"local out = infra.dump_any(ret)" LF
"test.assert_ne(out, \"\")" LF
;

INFRA_TEST(readdir, ut_readdir_script);
