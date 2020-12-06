#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <thread>

#include <httpserver.hpp>
#include "common.h"
#include "settings.h"
#include "MultiSync.h"
#include "Plugin.h"
#include "Plugins.h"
#include "events.h"
#include "Sequence.h"
#include "log.h"

#include "channeloutput/serialutil.h"
#include "fppversion_defines.h"

enum {
    SET_SEQUENCE_NAME = 1,
    SET_MEDIA_NAME    = 2,

    START_SEQUENCE    = 3,
    START_MEDIA       = 4,
    STOP_SEQUENCE     = 5,
    STOP_MEDIA        = 6,
    SYNC              = 7,
    
    EVENT             = 8,
    BLANK             = 9
};

class WebAudioMultiSyncPlugin : public MultiSyncPlugin, public httpserver::http_resource  {
public:
    
    WebAudioMultiSyncPlugin() {}
	
	bool Init() {
		LogDebug(VB_SYNC, "Web Audio Sync Plugin Loaded");
		return true;
	}
	
    void SendSync(uint32_t frames, float seconds) {
        int diff = frames - lastSentFrame;
        float diffT = seconds - lastSentTime;
        bool sendSync = false;
        if (diffT > 0.5) {
            sendSync = true;
        } else if (!frames) {
            // no need to send the 0 frame
        } else if (frames < 32) {
            //every 8 at the start
            if (frames % 8 == 0) {
                sendSync = true;
            }
        } else if (diff == 16) {
            sendSync = true;
        }
        
        if (sendSync) {

            lastSentFrame = frames;
            lastSentTime = seconds;
        }
        lastFrame = frames;
    }

    virtual void SendSeqOpenPacket(const std::string &filename) {
        LogDebug(VB_SYNC, "SendSeqOpenPacket");
        lastSequence = filename;
        lastFrame = -1;
        lastSentTime = -1.0f;
        lastSentFrame = -1;
    }
    virtual void SendSeqSyncStartPacket(const std::string &filename) {
        LogDebug(VB_SYNC, "SendSeqStartPacket");
        if (filename != lastSequence) {
            SendSeqOpenPacket(filename);
        }
        lastFrame = -1;
        lastSentTime = -1.0f;
        lastSentFrame = -1;
    }
    virtual void SendSeqSyncStopPacket(const std::string &filename) {
        LogDebug(VB_SYNC, "SendSeqStopPacket");
        
        lastSequence = "";
        lastFrame = -1;
        lastSentTime = -1.0f;
        lastSentFrame = -1;
    }
    virtual void SendSeqSyncPacket(const std::string &filename, int frames, float seconds) {
        LogExcess(VB_SYNC, "SendSeqSyncPacket");
        if (filename != lastSequence) {
            SendSeqSyncStartPacket(filename);
        }
        SendSync(frames, seconds);
    }
    
    virtual void SendMediaOpenPacket(const std::string &filename)  {
        LogDebug(VB_SYNC, "SendMediaOpenPacket");
        if (sendMediaSync) {
            lastMedia = filename;
            lastFrame = -1;
            lastSentTime = -1.0f;
            lastSentFrame = -1;
        }
    }
    virtual void SendMediaSyncStartPacket(const std::string &filename) {
        LogDebug(VB_SYNC, "SendMediaSyncStartPacket");
        if (sendMediaSync) {
            if (filename != lastMedia) {
                SendSeqOpenPacket(filename);
            }
            
            lastFrame = -1;
            lastSentTime = -1.0f;
            lastSentFrame = -1;
        }
    }
    virtual void SendMediaSyncStopPacket(const std::string &filename) {
        LogDebug(VB_SYNC, "SendMediaSyncStopPacket");
        if (sendMediaSync) {
            lastMedia = "";
            lastFrame = -1;
            lastSentTime = -1.0f;
            lastSentFrame = -1;
        }
    }
    virtual void SendMediaSyncPacket(const std::string &filename, float seconds) {
        LogExcess(VB_SYNC, "SendMediaSyncPacket");
        if (sendMediaSync) {
            if (filename != lastMedia) {
                SendMediaSyncStartPacket(filename);
            }
            SendSync(lastFrame > 0 ? lastFrame : 0, seconds);
        }
    }
    
    virtual void SendEventPacket(const std::string &eventID) {
        
    }
    virtual void SendBlankingDataPacket(void) {
        
    }
    
    bool fullCommandRead(int &commandSize) {
        if (curPosition == 0) {
            return false;
        }
        switch (readBuffer[0]) {
        case SET_SEQUENCE_NAME:
        case SET_MEDIA_NAME:
        case EVENT:
            //need null terminated string
            for (commandSize = 0; commandSize < curPosition; commandSize++) {
                if (readBuffer[commandSize] == 0) {
                    commandSize++;
                    return true;
                }
            }
            return false;
        case SYNC:
            commandSize = 9;
            return curPosition >= 9;
        case START_SEQUENCE:
        case START_MEDIA:
        case STOP_SEQUENCE:
        case STOP_MEDIA:
        case BLANK:
            commandSize = 1;
            break;
        default:
            commandSize = 1;
            return false;
        }
        return true;
    }
	
    bool bridgeToLocal = false;
	
    std::string lastSequence;
    std::string lastMedia;
    bool sendMediaSync = true;
    int lastFrame = -1;
    
    float lastSentTime = -1.0f;
    int lastSentFrame = -1;
    
    char readBuffer[256];
    int curPosition = 0;
};

class WebAudioFPPPlugin : public FPPPlugin {
public:
    WebAudioMultiSyncPlugin *plugin = new WebAudioMultiSyncPlugin();
    bool enabled = true;
	
    virtual ~WebAudioFPPPlugin() {}
    
    void registerApis(httpserver::webserver *m_ws) {
        //at this point, most of FPP is up and running, we can register our MultiSync plugin
        if (enabled && plugin->Init()) {
            if (getFPPmode() == MASTER_MODE) {
                //only register the sender for master mode
                multiSync->addMultiSyncPlugin(plugin);
            }
        } else {
            enabled = false;
        }
        m_ws->register_resource("/WebAudio", plugin, true);
        
    }
	
    virtual void addControlCallbacks(std::map<int, std::function<bool(int)>> &callbacks) {
        
    }
};


extern "C" {
    FPPPlugin *createPlugin() {
        return new WebAudioFPPPlugin();
    }
}
