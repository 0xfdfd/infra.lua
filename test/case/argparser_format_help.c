#include "test.h"

static const char* ut_argparser_script = TEST_ALIGN_LINE
"local argp = infra.argparser({ epilog = \"after help\", description = \"test is a test program\" })" LF
"argp:add_argument(\"-h, --help,--help2\", { action = \"help\" })" LF
"argp:add_argument(\"-f\", { required = true, default = \"1\", nargs = 1 })" LF
"local ret = argp:format_help()" LF
"test.assert_ne(ret, \"\")" LF
;

INFRA_TEST(argparser_format_help, ut_argparser_script);
