#include "test.h"

INFRA_TEST(execute, TEST_REPLAT(LF, 3)
"local self_path = infra.exepath()" LF
"local input = \"hello world\"" LF
"local opt = {" LF
"    args = { \"--echo-server\" }," LF
"    stdin = input," LF
"}" LF
"local ret,data = infra.execute(self_path, opt)" LF
"test.assert_eq(ret, true)" LF
"test.assert_eq(data, input)" LF
);

INFRA_TEST(execute_nonexist,
"local self_path = infra.exepath()" LF
"self_path = self_path .. \"/non_exist\"" LF
"local ret = pcall(infra.execute, self_path)" LF
"test.assert_eq(ret, false)" LF
);
