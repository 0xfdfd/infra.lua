#include "test.h"

static const char* ut_argparser_narg_script = TEST_ALIGN_LINE
"-- Normal nargs" LF
"do" LF
"    local argp = infra.argparser()" LF
"    argp:add_argument(\"-f\", { nargs = 2 })" LF
"    local arguments = {" LF
"        \"-f\"," LF
"        \"hello\"," LF
"        \"world\"," LF
"    }" LF
"    local b,ret = pcall(argp.parse_args, argp, arguments)" LF
"    test.assert_eq(b, true)" LF
"    test.assert_eq(ret[\"-f\"], \"world\")" LF
"end" LF
LF
"-- Check for nargs mismatch." LF
"do" LF
"    local argp = infra.argparser()" LF
"    argp:add_argument(\"-b,--bar\", { nargs = 3 })" LF
"    local arguments = {" LF
"        \"--foo\"," LF
"        \"hello\"," LF
"    }" LF
"    local ret = pcall(argp.parse_args, argp, arguments)" LF
"    test.assert_eq(ret, false)" LF
"end" LF
;

INFRA_TEST(argparser_narg, ut_argparser_narg_script);
