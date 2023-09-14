#include "test.h"

static const char* ut_writefile_script = TEST_ALIGN_LINE
"local data = [[" LF
"hello world" LF
"]]" LF
"local tmpfile = \"writefile.tmp\"" LF
LF
"infra.writefile(tmpfile, data)" LF
"local ret = infra.readfile(tmpfile)" LF
"os.remove(tmpfile)" LF
"test.assert_eq(ret, data)" LF
;

INFRA_TEST(writefile, ut_writefile_script);
