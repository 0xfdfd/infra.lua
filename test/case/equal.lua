local infra = require('infra')

-- Compare number
do
    assert(infra.equal(1, 1) == true)
    assert(infra.equal(1, 2) == false)
end

-- Compare table: same value
do
    local v1 = { "a", "b" }
    local v2 = { "a", "b" }
    assert(infra.equal(v1, v2) == true)
end

-- Compare table: one of them is empty
do
    local v1 = { }
    local v2 = { "a", "b" }
    assert(infra.equal(v1, v2) == false)
end

-- Compare table: different content
do
    local v1 = { a = "hello", b = "world" }
    local v1 = { a = "world", b = "hello" }
    assert(infra.equal(v1, v2) == false)
end

-- Compare table: same reference
do
    local v1 = { "hello", "world" }
    local v2 = v1
    assert(infra.equal(v1, v2) == true)
end
