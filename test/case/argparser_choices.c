#include "test.h"

static const char* ut_argparser_choices_script = TEST_ALIGN_LINE
"local choices = {}" LF
"choices[\"1\"] = true" LF
"choices[\"3\"] = true" LF
"local argp = infra.argparser()" LF
"argp:add_argument(\"-a\", { choices = choices, nargs = 1 })" LF
LF
"local args = {" LF
"    \"-a\","
"    \"1\","
"}" LF
"local ret = argp:parse_args(args)" LF
"test.assert_eq(ret[\"-a\"], \"1\")" LF
;

INFRA_TEST(argparser_choices, ut_argparser_choices_script);
