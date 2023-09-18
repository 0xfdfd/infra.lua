#include "test.h"

static const char* ut_argparser_format_script = TEST_ALIGN_LINE
"local argp = infra.argparser()" LF
"argp:add_argument(\"--foo\", { action = \"append\", nargs = 1 })" LF
"argp:add_argument(\"--bar\", { action = \"append\", nargs = 2 })" LF
LF
"local args = {" LF
"    \"--foo=hello\"," LF
"    \"--foo\"," LF
"    \"world\"," LF
"    \"--bar=hello\"," LF
"    \"world\"," LF
"    \"--bar=hello2\"," LF
"    \"world2\"," LF
"}" LF
"local ret = argp:parse_args(args)" LF
"test.assert_eq(#ret[\"--foo\"], 2)" LF
"test.assert_eq(ret[\"--foo\"][1], \"hello\")" LF
"test.assert_eq(ret[\"--foo\"][2], \"world\")" LF
"test.assert_eq(#ret[\"--bar\"], 4)" LF
"test.assert_eq(ret[\"--bar\"][1], \"hello\")" LF
"test.assert_eq(ret[\"--bar\"][2], \"world\")" LF
"test.assert_eq(ret[\"--bar\"][3], \"hello2\")" LF
"test.assert_eq(ret[\"--bar\"][4], \"world2\")" LF
;

INFRA_TEST(argparser_longopt, ut_argparser_format_script);
