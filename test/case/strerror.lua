local infra = require('infra')

local data = infra.strerror(0)
assert(data == "Operation success")
