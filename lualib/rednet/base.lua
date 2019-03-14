local core = require "rednet.core"
local cmd  = require "rednet.command"

local rednet = {
    EP_TEXT = 0,
    EP_OPER = 1,
    EP_RESPONSE = 2,
    EP_ERROR = 3,
    EP_SOCKET = 4,
    EP_LUA = 5,
    EP_DEBUG = 6,
    DONT = 255,
}


local function init_all()
	local funcs = init_func
	init_func = nil
	if funcs then
		for _,f in ipairs(funcs) do
			f()
		end
	end
end

local function ret(f, ...)
	f()
	return ...
end

local function init_template(start, ...)
	init_all()
	init_func = {}
	return ret(init_all, start(...))
end

function rednet.pcall(start, ...)
	return xpcall(init_template, debug.traceback, start, ...)
end

function rednet.address(addr)
    if type(addr) == "number" then
        return string.format(":%08x", addr)
    else
        return tostring(addr)
    end
end


function rednet.self()
	return core.self()
end

function rednet.getenv(key)
	return (core.command(cmd.CC_GETENV, key))
end

function rednet.setenv(key, value)
	assert(core.command(cmd.CC_GETENV,key) == nil, "Can't setenv exist key : " .. key)
	core.command(cmd.CC_SETENT,key .. " " ..value)
end

function rednet.launch(...)
    local addr = core.command(cmd.CC_LAUNCH, table.concat({...}," "))
	if addr then
		return tonumber("0x" .. string.sub(addr , 2))
	end
end

function rednet.abort()
    core.command(cmd.CC_ABORT)
end

function rednet.name(name, handle)
    core.command(cmd.CC_NAME, name .. " " .. rednet.address(handle))
end


return rednet