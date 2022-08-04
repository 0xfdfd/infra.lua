local infra = require('infra')

-- json to table
do
    local data = "{ \"a\": [ \"b\", \"c\" ] }"
    io.write(infra.dump_value_as_string(infra.json_to_table(data)) .. '\n')
end

-- table to json
do
    local test_data = { z = infra.null() }

    test_data.a = "hello \"world\""
    test_data.b = 123.456
    test_data.c = true
    test_data.d = { 'a', 'b', 'c' }
    test_data.e = infra.int64(1, 2)
    test_data.f = infra.uint64(3, 4)

    io.write(infra.table_to_json(test_data) .. "\n")
end

do
    local test_table = { "hello", "world" }

    local test_list = infra.new_list({ 1, 2, 3 })
    test_list:push_back(test_table)

    local test_data = { b = infra.int64(1, 2) }
    test_data.a = test_list
    test_data.c = true

    io.write(infra.table_to_json(test_data) .. "\n")
end
