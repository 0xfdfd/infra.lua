#include "test.h"

static const char* ut_readfile_nonexist_script = TEST_ALIGN_LINE
"local tmpfile = \"f6c67425-1d14-43fc-b6b2-0ced0b5bf3eb\"" LF
"local ret = pcall(infra.readfile, tmpfile)" LF
"test.assert_eq(ret, false)" LF
;

INFRA_TEST(readfile_nonexist, ut_readfile_nonexist_script);
