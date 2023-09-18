#include "test.h"

static const char* ut_argparser_shortopt_script = TEST_ALIGN_LINE
"local argp = infra.argparser()" LF
"argp:add_argument(\"-f\", { action = \"append\", nargs = 1 })" LF
"argp:add_argument(\"-b\", { action = \"append\", nargs = 2 })" LF
LF
"local args = {" LF
"    \"-fhello\"," LF
"    \"-bhello\"," LF
"    \"world\"," LF
"}" LF
"local ret = argp:parse_args(args)" LF
"test.assert_eq(#ret[\"-f\"], 1)" LF
"test.assert_eq(ret[\"-f\"][1], \"hello\")" LF
"test.assert_eq(#ret[\"-b\"], 2)" LF
"test.assert_eq(ret[\"-b\"][1], \"hello\")" LF
"test.assert_eq(ret[\"-b\"][2], \"world\")" LF
;

INFRA_TEST(argparser_shortopt, ut_argparser_shortopt_script);
