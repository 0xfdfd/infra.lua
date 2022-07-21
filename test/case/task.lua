local infra = require('infra')

local sum = 0

local function thread_1()
    for i = 1, 10, 1 do
        sum = sum + 1
        coroutine.yield()
    end
end

infra.task_spawn(coroutine.create(thread_1))
infra.task_spawn(coroutine.create(thread_1))
infra.task_run()

assert(sum == 20)
