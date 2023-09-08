#include "test.h"

INFRA_TEST(map_empty,
"do" LF
"    local size = infra.make_map():size()" LF
"    test.assert_eq(size, 0)" LF
"end" LF
);

INFRA_TEST(map_insert,
"do" LF
"    local map = infra.make_map()" LF
"    for v = 1,100 do" LF
"        local ret = map:insert(v, 0)" LF
"        test.assert_eq(ret, true, \"v=%d\", v)" LF
"    end" LF
"    test.assert_eq(map:size(), 100)" LF
"end" LF
);

INFRA_TEST(map_duplicate_insert,
"do" LF
"    local map = infra.make_map()" LF
"    local ret1 = map:insert(1, 0)" LF
"    test.assert_eq(ret1, true)" LF
"    local ret2 = map:insert(1, 1)" LF
"    test.assert_eq(ret2, false)" LF
"end" LF
);

INFRA_TEST(map_pairs,
"do" LF
"    local map = infra.make_map()" LF
"    for v = 0,9999 do" LF
"        map:insert(v, 0)" LF
"    end" LF
"    local cnt = 0" LF
"    for k,v in map:pairs() do" LF
"        test.assert_eq(cnt, k)" LF
"        cnt = cnt + 1" LF
"    end" LF
"end" LF
);

INFRA_TEST(map_replace,
"do" LF
"    local map = infra.make_map()" LF
"    map:insert(0, 1)" LF
"    test.assert_eq(map:insert(0, 0), false)" LF
"    map:replace(0, 2)" LF
"    test.assert_eq(map:size(), 1)" LF
LF
"    local ret = select(2, map:find(0))" LF
"    test.assert_eq(ret, 2)" LF
"end" LF
);

INFRA_TEST(map_make,
"local src = { \"hello\", \"world\" }" LF
"local map = infra.make_map(src)" LF
"test.assert_eq(map:size(), 2)" LF
);
