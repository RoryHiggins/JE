local log = {}
log.SYSTEM_NAME = "log"
log.LOG_LEVEL_TRACE = 0  -- logs for debugging that are happening very frequently (e.g. each step)
log.LOG_LEVEL_DEBUG = 1
log.LOG_LEVEL_LOG = 2
log.LOG_LEVEL_WARN = 3
log.LOG_LEVEL_ERR = 4
log.LOG_LEVEL_NONE = 5
log.testsLogLevel = log.LOG_LEVEL_WARN
log.logLevel = log.LOG_LEVEL_LOG
log.debugger = nil
log.levelStack = {}
local function logImpl(level, format, ...)
	if log.logLevel > level then
		return
	end

	if log.debugger and level >= log.LOG_LEVEL_WARN then
		log.debugger()
	end

	local callee_info = debug.getinfo(3, "Sln")
	print(string.format(format, callee_info.short_src, callee_info.currentline, callee_info.name, ...))
	io.flush()
end
function log.trace(format, ...)
	logImpl(log.LOG_LEVEL_TRACE, "[trace %s:%d] %s() "..format, ...)
end
function log.debug(format, ...)
	logImpl(log.LOG_LEVEL_DEBUG, "[debug %s:%d] %s() "..format, ...)
end
function log.info(format, ...)
	logImpl(log.LOG_LEVEL_LOG, "[info  %s:%d] %s() "..format, ...)
end
function log.warn(format, ...)
	logImpl(log.LOG_LEVEL_WARN, "[WARN  %s:%d] %s() "..format.."\n"..debug.traceback(), ...)
end
function log.error(format, ...)
	logImpl(log.LOG_LEVEL_ERR, "[ERROR %s:%d] %s() "..format.."\n"..debug.traceback(), ...)
end
function log.assert(expr)
	if expr then
		return true
	end

	log.error("Assertion failed")
	return false
end
function log.protectedCall(fn, ...)
	local args = {...}

	return xpcall(function () fn(unpack(args)) end, log.error)
end
function log.enableDebugger()
	log.debugger = require("engine/lib/debugger/debugger")
	log.protectedCall = log.debugger.call
	log.assert = log.debugger.assert
end
function log.pushLogLevel(level)
	log.levelStack[#log.levelStack + 1] = log.logLevel
	log.logLevel = level
end
function log.popLogLevel()
	if #log.levelStack == 0 then
		return
	end

	log.logLevel = log.levelStack[#log.levelStack]
	log.levelStack[#log.levelStack] = nil
end
return log
