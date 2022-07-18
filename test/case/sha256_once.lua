local infra = require('infra')

local sha256_ret = infra.sha256_once("hello infra")
assert(sha256_ret == "C551B909B1E21EADFFE8F23B5D8A1AA8BB7F16D45F7A8A5B1A917E9CD4AB5C63")
