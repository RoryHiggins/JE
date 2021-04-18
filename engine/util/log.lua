local log = {}
log.LOG_LEVEL_TRACE = 0  -- logs for debugging that are happening very frequently (e.g. each step)
log.LOG_LEVEL_DEBUG = 1
log.LOG_LEVEL_LOG = 2
log.LOG_LEVEL_WARN = 3
log.LOG_LEVEL_ERR = 4
log.LOG_LEVEL_NONE = 5
log.testsLogLevel = log.LOG_LEVEL_WARN
log.logLevel = log.LOG_LEVEL_LOG
log.debugger = nil
log.protectedCall = pcall
log.assert = assert
local function noop()
end
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
	logImpl(log.LOG_LEVEL_WARN, "[WARN  %s:%d] %s() "..format, ...)
end
function log.error(format, ...)
	logImpl(log.LOG_LEVEL_ERR, "[ERROR %s:%d] %s() "..format, ...)
end
function log.enableDebugger()
	log.debugger = require("engine/lib/debugger/debugger")
	log.protectedCall = log.debugger.call
	log.assert = log.debugger.assert
end
function log.disableLogging()
	log.debugger = nil
	log.protectedCall = pcall
	log.assert = noop
	log.trace = noop
	log.debug = noop
	log.info = noop
	log.warn = noop
	log.error = noop
end
return log
