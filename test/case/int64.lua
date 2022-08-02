local infra = require('infra')

-- Test_1: same value should be equal
do
    local v1 = infra.uint64(1, 2)
    local v2 = infra.uint64(1, 2)
    assert(v1 == v2)
end

-- Test_2: different value should not equal
do
    local v1 = infra.uint64(1, 2)
    local v2 = infra.uint64(3, 4)
    assert(v1 ~= v2)
    assert(v1 < v2)
    assert(v1 <= v2)
    assert(v2 > v1)
    assert(v2 >= v1)
end

-- Test_3: After math operation, value can be the same
do
    local v1 = infra.uint64(1, 2)
    local v2 = infra.uint64(1, 3)
    assert(v1 + v2 == infra.uint64(2, 5))
end

-- Test_4: mixed operation
do
    local v1 = infra.int64(1, 2)
    local v2 = infra.uint64(1, 2)
    assert(v1 + v2 == v2 + v1)
end
