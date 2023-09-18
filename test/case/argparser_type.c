#include "test.h"

static const char* ut_argparser_type_script = TEST_ALIGN_LINE
"local argp = infra.argparser()" LF
"argp:add_argument(\"-a\", { nargs = 1, type = \"number\" })" LF
"argp:add_argument(\"-b\", { nargs = 1, type = \"number\" })" LF
"argp:add_argument(\"-c\", { nargs = 1, type = infra.readfile })" LF
LF
"local args = {" LF
"    \"-a\"," LF
"    \"1\"," LF
"    \"-b2.0\"," LF
"    \"-c\"," LF
"    infra.exepath()," LF
"}" LF
"local ret = argp:parse_args(args)" LF
"test.assert_eq(type(ret[\"-a\"]), \"number\")" LF
"test.assert_eq(ret[\"-a\"], 1)" LF
"test.assert_eq(type(ret[\"-b\"]), \"number\")" LF
"test.assert_eq(ret[\"-b\"], 2.0)" LF
"test.assert_eq(type(ret[\"-c\"]), \"string\")" LF
;

INFRA_TEST(argparser_type, ut_argparser_type_script);
