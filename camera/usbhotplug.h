#ifndef USBHOTPLUG_H
#define USBHOTPLUG_H
#include <functional>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <map>

#define MESSAGE_BUFFER_SIZE 2048

class UsbHotplug
{
public:
    enum State {
        STATE_NONE = 0,
        STATE_RUN,
        STATE_TERMINATE
    };
    enum Action {
        ACTION_NONE = 0,
        ACTION_DEVICE_ATTACHED,
        ACTION_DEVICE_DETACHED
    };
    using FnNotify = std::function<void(int action)>;
    struct Device {
        int flag;
        std::string token;
        FnNotify notify;
    };

protected:
    int fd;
    int state;
    std::map<std::string, Device> deviceMap;
    std::mutex mutex;
    std::condition_variable condit;
    std::thread listenThread;
protected:
    void run();
public:
    UsbHotplug();
    /* vidpid: 093A:2510 */
    void registerDevice(const std::string& vidpid, const FnNotify &func);
    int start();
    void stop();
};

#endif // USBHOTPLUG_H
