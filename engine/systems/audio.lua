local log = require("engine/util/log")
local client = require("engine/client/client")

local Audio = {}
Audio.SYSTEM_NAME = "audio"
function Audio:loadAudio(filename)
    log.trace("filename=%s", filename)

    if client.state.headless then
        return true
    end

    if self.loadedAudio[filename] ~= nil then
        return true
    end

    local success, audioId = client.loadAudio({["filename"] = filename})
    if not success then
        log.error("failed to load audio")
        return false
    end

    log.assert(audioId ~= nil)
    log.assert(audioId ~= 0)

    local audio = {
        ["audioId"] = audioId,
    }
    self.loadedAudio[filename] = audio
    return true
end
function Audio:unloadAudio(filename)
    log.trace("filename=%s", filename)

    if client.state.headless then
        return true
    end

    local audio = self.loadedAudio[filename]
    if audio == nil then
        log.warning("not loaded, filename=%s", filename)
        return false
    end
    log.assert(audio.audioId ~= nil)
    log.assert(audio.audioId ~= 0)

    self.loadedAudio[filename] = nil
    return client.unloadAudio({["audioId"] = audio.audioId})
end
function Audio:playAudio(filename)
    log.trace("filename=%s", filename)

    if client.state.headless then
        return true
    end

    local audio = self.loadedAudio[filename]
    if audio == nil then
        self:loadAudio(filename)
        audio = self.loadedAudio[filename]
    end
    log.assert(audio.audioId ~= nil)

    return client.playAudio({["audioId"] = audio.audioId})
end
function Audio:clearAudio()
    log.trace("")

    if client.state.headless then
        return true
    end

    return client.clearAudio()
end
function Audio:onInit(simulation)
	self.simulation = simulation

	self.loadedAudio = {}
end
function Audio:onRunTests()
    local emptyAudio = "client/data/audio_empty.wav"
    log.assert(self:loadAudio(emptyAudio))
    log.assert(self:playAudio(emptyAudio))
    log.assert(self:clearAudio())
    log.assert(self:unloadAudio(emptyAudio))
end

return Audio
