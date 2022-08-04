local infra = require('infra')

-- Empty list have size of 0
do
    local list = infra.new_list()
    assert(list:size() == 0)
end

-- Push back
do
    local list = infra.new_list()
    list:push_back(9)
    assert(list:size() == 1)
    assert(list:pop_front() == 9)
    assert(list:size() == 0)
end

-- Pop empty list
do
    local list = infra.new_list()
    assert(list:pop_front() == nil)
end

-- Create infra list from native list
do
    local list = infra.new_list({ "1", "2", "3" })
    assert(list:size() == 3)
    assert(list:pop_front() == "1")
    assert(list:pop_front() == "2")
    assert(list:pop_front() == "3")
    assert(list:size() == 0)
end

-- pairs
do
    local list = infra.new_list({ 1, 2, 3 })
    local cnt = 1
    for k, v in pairs(list) do
        assert(v == cnt)
        cnt = cnt + 1
    end
end
