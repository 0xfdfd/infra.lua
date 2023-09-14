#include "test.h"

static const char* ut_cwd_script = TEST_ALIGN_LINE
"local ret = infra.cwd()" LF
"test.assert_ne(ret, \"\")" LF
;

INFRA_TEST(cwd, ut_cwd_script);
