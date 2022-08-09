local infra = require('infra')

-- map.new_map
do
    local map = infra.new_map({ "h1", "h2", "h3" })
    assert(map:size() == 3)

    map = infra.new_map()
    assert(map:size() == 0)

    local src = { "hello", "world" }
    map:copy(src)
    assert(map:size() == 2)

    for k, v in pairs(map) do
        assert(src[k] == v)
    end
end

-- map.clone()
do
    local map = infra.new_map({ "h1", "h2", "h3" })
    local map2 = map:clone()
    assert(map == map2)
end

-- map.__eq
do
    local t1 = infra.new_map({ "hello", "world" })
    assert(t1:equal({ "hello", "world" }))
end

-- map.v
do
    local t = infra.new_map({ "h1", "h2", "h3" })
    t:v()["hello"] = "world"
    assert(t:v()["hello"] == "world")
end
