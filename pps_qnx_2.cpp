#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <sys/neutrino.h>
#include <sys/iomsg.h>
#include <sys/iofunc.h>
#include <pps.h>

namespace PPS {

class PPSObject {
protected:
    int fd = -1;
    std::string path;
    std::atomic<bool> active{true};
    
public:
    PPSObject(const std::string& objectPath) : path(objectPath) {}
    virtual ~PPSObject() { if (fd >= 0) close(fd); }
    
    bool isValid() const { return fd >= 0; }
    const std::string& getPath() const { return path; }
};

class PPSReaderObject : public PPSObject {
private:
    struct _pulse pulse;
    int chid = -1;
    int coid = -1;
    std::thread eventThread;
    
public:
    PPSReaderObject(const std::string& objectPath) : PPSObject(objectPath) {
        fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) return;
        
        // Create channel for ionotify
        chid = ChannelCreate(0);
        if (chid == -1) return;
        
        coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
        if (coid == -1) return;
    }
    
    ~PPSReaderObject() override {
        active = false;
        if (eventThread.joinable()) {
            eventThread.join();
        }
        if (coid != -1) ConnectDetach(coid);
        if (chid != -1) ChannelDestroy(chid);
    }
    
    void startAsync(std::function<void(const std::string&, const std::string&)> callback) {
        if (!isValid()) return;
        
        // Set up ionotify for read events
        struct sigevent event;
        SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, 1, 0);
        
        if (ionotify(fd, _NOTIFY_ACTION_TRANARM, _NOTIFY_COND_INPUT, &event) == -1) {
            return;
        }
        
        eventThread = std::thread([this, callback = std::move(callback)] {
            while (active) {
                int rcvid = MsgReceivePulse(chid, &pulse, sizeof(pulse), NULL);
                if (rcvid == -1) {
                    if (errno == EINTR) continue;
                    break;
                }
                
                if (pulse.code == 1) { // Our pulse code
                    char buffer[4096];
                    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
                    if (bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        callback(path, std::string(buffer));
                        
                        // Re-arm the notification
                        struct sigevent event;
                        SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, 1, 0);
                        ionotify(fd, _NOTIFY_ACTION_TRANARM, _NOTIFY_COND_INPUT, &event);
                    }
                }
            }
        });
    }
};

class PPSWriterObject : public PPSObject {
public:
    PPSWriterObject(const std::string& objectPath) : PPSObject(objectPath) {
        fd = open(path.c_str(), O_WRONLY | O_CREAT, 0644);
    }
    
    bool writeData(const std::string& data) {
        if (!isValid()) return false;
        ssize_t written = write(fd, data.c_str(), data.size());
        return (written == static_cast<ssize_t>(data.size()));
    }
};

class PPSEngine {
private:
    std::mutex readersMutex;
    std::mutex writersMutex;
    std::unordered_map<std::string, std::unique_ptr<PPSReaderObject>> readers;
    std::unordered_map<std::string, std::unique_ptr<PPSWriterObject>> writers;
    std::unordered_map<std::string, std::vector<std::function<void(const std::string&, const std::string&)>>> handlers;
    std::atomic<bool> running{true};
    
public:
    ~PPSEngine() {
        running = false;
        // Readers will be cleaned up by their destructors
    }
    
    bool subscribe(const std::string& objectPath, std::function<void(const std::string&, const std::string&)> handler) {
        std::lock_guard<std::mutex> lock(readersMutex);
        
        auto it = readers.find(objectPath);
        if (it == readers.end()) {
            auto reader = std::make_unique<PPSReaderObject>(objectPath);
            if (!reader->isValid()) return false;
            
            // Start the async reader with a callback that forwards to all handlers
            reader->startAsync([this, objectPath](const std::string& path, const std::string& data) {
                std::lock_guard<std::mutex> hLock(this->handlersMutex);
                auto hIt = this->handlers.find(objectPath);
                if (hIt != this->handlers.end()) {
                    for (auto& h : hIt->second) {
                        h(path, data);
                    }
                }
            });
            
            readers[objectPath] = std::move(reader);
        }
        
        handlers[objectPath].push_back(std::move(handler));
        return true;
    }
    
    bool publish(const std::string& objectPath, const std::string& data) {
        std::lock_guard<std::mutex> lock(writersMutex);
        
        auto it = writers.find(objectPath);
        if (it == writers.end()) {
            auto writer = std::make_unique<PPSWriterObject>(objectPath);
            if (!writer->isValid()) return false;
            writers[objectPath] = std::move(writer);
            it = writers.find(objectPath);
        }
        
        return it->second->writeData(data);
    }
    
private:
    std::mutex handlersMutex;
};

} // namespace PPS

// Usage example for Approach 2 - More scalable for many objects
class AdvancedModel {
private:
    PPS::PPSEngine& engine;
    std::string objectPath;
    
public:
    AdvancedModel(PPS::PPSEngine& eng, const std::string& path) 
        : engine(eng), objectPath(path) {}
    
    void update(const std::string& jsonData) {
        engine.publish(objectPath, jsonData);
    }
};

class AdvancedView {
private:
    std::string name;
    std::unordered_map<std::string, std::function<void(const std::string&)>> modelHandlers;
    
public:
    AdvancedView(const std::string& viewName) : name(viewName) {}
    
    void registerModel(const std::string& modelPath, std::function<void(const std::string&)> handler) {
        modelHandlers[modelPath] = std::move(handler);
    }
    
    void handleData(const std::string& modelPath, const std::string& data) {
        auto it = modelHandlers.find(modelPath);
        if (it != modelHandlers.end()) {
            it->second(data);
        }
    }
};

int main() {
    PPS::PPSEngine engine;
    
    // Create multiple models and views
    AdvancedModel carModel(engine, "/pps/vehicle/speed");
    AdvancedModel batteryModel(engine, "/pps/vehicle/battery");
    AdvancedModel gpsModel(engine, "/pps/vehicle/gps");
    
    AdvancedView dashboardView("Dashboard");
    AdvancedView mobileAppView("MobileApp");
    
    // Register handlers for different views
    dashboardView.registerModel("/pps/vehicle/speed", [](const std::string& data) {
        std::cout << "Dashboard: Speed update - " << data << std::endl;
    });
    
    dashboardView.registerModel("/pps/vehicle/battery", [](const std::string& data) {
        std::cout << "Dashboard: Battery update - " << data << std::endl;
    });
    
    mobileAppView.registerModel("/pps/vehicle/gps", [](const std::string& data) {
        std::cout << "MobileApp: GPS update - " << data << std::endl;
    });
    
    // Subscribe views to models
    engine.subscribe("/pps/vehicle/speed", [&dashboardView](const std::string& path, const std::string& data) {
        dashboardView.handleData(path, data);
    });
    
    engine.subscribe("/pps/vehicle/battery", [&dashboardView](const std::string& path, const std::string& data) {
        dashboardView.handleData(path, data);
    });
    
    engine.subscribe("/pps/vehicle/gps", [&mobileAppView](const std::string& path, const std::string& data) {
        mobileAppView.handleData(path, data);
    });
    
    // Simulate real-time updates
    while (true) {
        carModel.update("{\"speed\": 65, \"unit\": \"mph\"}");
        batteryModel.update("{\"level\": 85, \"charging\": false}");
        gpsModel.update("{\"lat\": 37.7749, \"lng\": -122.4194}");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}