#include "test.h"

static const char* ut_execute_script = TEST_ALIGN_LINE
"local self_path = infra.exepath()" LF
"local input = \"hello world\"" LF
"local opt = {" LF
"    args = { \"--echo-server\" }," LF
"    stdin = input," LF
"}" LF
"local ret,data = infra.execute(self_path, opt)" LF
"test.assert_eq(ret, true)" LF
"test.assert_eq(data, input)" LF
;

INFRA_TEST(execute, ut_execute_script);

static const char* ut_execute_nonexist_script = TEST_ALIGN_LINE
"local self_path = infra.exepath()" LF
"self_path = self_path .. \"/non_exist\"" LF
"local ret = pcall(infra.execute, self_path)" LF
"test.assert_eq(ret, false)" LF
;

INFRA_TEST(execute_nonexist, ut_execute_nonexist_script);

static const char* ut_execute_stderr_script = TEST_ALIGN_LINE
"local self_path = infra.exepath()" LF
"local input = \"hello world\"" LF
"local opt = {" LF
"    args = { \"--echo-stderr-server\" }," LF
"    stdin = input," LF
"}" LF
"local ret,stdout_data,stderr_data = infra.execute(self_path, opt)" LF
"test.assert_eq(ret, true)" LF
"test.assert_eq(stdout_data, \"\")" LF
"test.assert_eq(stderr_data, input)" LF
;

INFRA_TEST(execute_stderr, ut_execute_stderr_script);
