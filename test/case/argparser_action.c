#include "test.h"

static const char* ut_argparser_action_script = TEST_ALIGN_LINE
"local function action_sum(self, oldval, newval)" LF
"    if oldval == nil then" LF
"        return newval" LF
"    end" LF
"    return oldval + newval" LF
"end" LF
LF
"local argp = infra.argparser()" LF
"argp:add_argument(\"-a\", { action = \"store\", nargs = 1 })" LF
"argp:add_argument(\"-b\", { action = \"append\", nargs = 1 })" LF
"argp:add_argument(\"-c\", { action = \"count\" })" LF
"argp:add_argument(\"-d\", { action = action_sum, nargs = '?', type = \"number\" })" LF
LF
"local function test_a()" LF
"    local args = {" LF
"        \"-a\"," LF
"        \"hello\"," LF
"        \"-a\"," LF
"        \"world\"," LF
"    }" LF
"    local ret = argp:parse_args(args)" LF
"    test.assert_eq(ret[\"-a\"], \"world\")" LF
"end" LF
LF
"local function test_b()" LF
"    local args = {" LF
"        \"-b\"," LF
"        \"hello\"," LF
"        \"-b\"," LF
"        \"world\"," LF
"    }" LF
"    local ret = argp:parse_args(args)" LF
"    test.assert_eq(ret[\"-b\"][1], args[2])" LF
"    test.assert_eq(ret[\"-b\"][2], args[4])" LF
"end" LF
LF
"local function test_c()" LF
"    local args = {" LF
"        \"-c\"," LF
"        \"-c\"," LF
"    }" LF
"    local ret = argp:parse_args(args)" LF
"    test.assert_eq(ret[\"-c\"], #args)" LF
"end" LF
LF
"do" LF
"    local args = {" LF
"        \"-d\"," LF
"        \"1\"," LF
"        \"-d2\"," LF
"    }" LF
"    local ret = argp:parse_args(args)" LF
"    test.assert_eq(ret[\"-d\"], 3)" LF
"end" LF
LF
"test_a()" LF
"test_b()" LF
"test_c()" LF
;

INFRA_TEST(argparser_action, ut_argparser_action_script);
