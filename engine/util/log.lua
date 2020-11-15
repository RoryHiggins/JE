local log = {}
log.LOG_LEVEL_TRACE = 0
log.LOG_LEVEL_DEBUG = 1
log.LOG_LEVEL_LOG = 2
log.LOG_LEVEL_WARN = 3
log.LOG_LEVEL_ERR = 4
log.LOG_LEVEL_NONE = 5
log.testLogLevel = log.LOG_LEVEL_WARN
log.logLevel = log.LOG_LEVEL_LOG
local function logImpl(level, format, ...)
	if log.logLevel > level then
		return
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

return log
