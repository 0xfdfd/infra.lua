#include "test.h"

INFRA_TEST(exepath,
"local path = infra.exepath()" LF
"local pos = string.find(path, \"test\")" LF
"test.assert_ne(pos, nil)" LF
);
