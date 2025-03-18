#include "usbhotplug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <chrono>
#include <iostream>
#include "strings.hpp"

std::string shellExecute(const std::string& command)
{
    std::string result = "";
    FILE *fpRead = popen(command.c_str(), "r");
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
    while (fgets(buf, 1024-1, fpRead)!=NULL) {
        result = buf;
    }
    if (fpRead != NULL) {
        pclose(fpRead);
    }
    auto it = result.find('\n');
    result.erase(it);
    return result;
}

int checkVideo(const std::string &message, std::string &vidpid)
{
    std::string key = "video4linux/video";
    std::string::size_type pos = message.find(key);
    if (pos == std::string::npos) {
        return -1;
    }
    std::string strNum = message.substr(pos + key.size());
    int num = std::atoi(strNum.c_str());
    if (num%2 != 0) {
        return 1;
    }
    std::string videoName = "video" + strNum;
    std::string vid;
    {
        std::string cmd = Strings::format(1024, "cat /sys/class/video4linux/%s/device/modalias", videoName.c_str());
        std::string result = shellExecute(cmd);
        int i = result.find('v');
        vid = Strings::upper(result.substr(i + 1, 4));
    }
    std::string pid;
    {
        std::string cmd = Strings::format(1024, "cat /sys/class/video4linux/%s/device/modalias", videoName.c_str());
        std::string result = shellExecute(cmd);
        int i = result.find('p');
        pid = Strings::upper(result.substr(i + 1, 4));
    }
    vidpid = vid + ":" + pid;
    return 0;
}

void UsbHotplug::run()
{
    std::cout<<"enter listen thread."<<std::endl;
    while (1) {
        {
            std::unique_lock<std::mutex> locker(mutex);
            condit.wait_for(locker, std::chrono::microseconds(500), [=]()->bool{
                    return  state == STATE_RUN || state == STATE_TERMINATE;
                    });
            if (state == STATE_TERMINATE) {
                state = STATE_NONE;
                break;
            }
        }

        /* recv message*/
        char buf[MESSAGE_BUFFER_SIZE] = {0};
        ssize_t len = recv(fd, &buf, sizeof(buf), 0);
        if (len <= 0) {
            continue;
        }
        /* parse message */
#if 0
        printf("%s\n", buf);
#endif
        std::string message(buf);
        int action = ACTION_NONE;
        if (message.find("remove@") != std::string::npos) {
            std::string::size_type pos = message.find("remove@");
            std::string token = message.substr(7);
            for (auto& dev : deviceMap) {
                if (dev.second.token == token) {
                    action = ACTION_DEVICE_DETACHED;
                    dev.second.flag = action;
                    dev.second.notify(action);
                    break;
                }
            }
        } else if (message.find("add@") != std::string::npos) {
            std::string vidpid;
            std::string::size_type pos = message.find("add@");
            std::string token = message.substr(4);
            action = ACTION_DEVICE_ATTACHED;
            pos = message.find("video4linux/video");
            if (pos != std::string::npos) {
                int ret = checkVideo(message, vidpid);
                if (ret != 0) {
                    continue;
                }
                auto it = deviceMap.find(vidpid);
                if (it != deviceMap.end()) {
                    it->second.token = token;
                    if (it->second.flag != action) {
                        it->second.flag = action;
                        it->second.notify(action);
                    }
                }
            } else {
                for (auto& dev : deviceMap) {
                    if (message.find(dev.first) != std::string::npos) {
                        dev.second.token = token;
                        if (dev.second.flag != action) {
                            dev.second.flag = action;
                            dev.second.notify(action);
                        }
                        break;
                    }
                }
            }
        }
    }
    std::cout<<"leave listen thread."<<std::endl;
    return;
}

UsbHotplug::UsbHotplug()
    :fd(-1), state(STATE_NONE)
{

}

void UsbHotplug::registerDevice(const std::string &vidpid, const UsbHotplug::FnNotify &func)
{
    /* vidpid: 093A:2510 */
    Device dev;
    dev.flag = ACTION_NONE;
    dev.notify = func;
    deviceMap.insert(std::pair<std::string, Device>(vidpid, dev));
    return;
}

int UsbHotplug::start()
{
    if (state != STATE_NONE) {
        return 0;
    }
    const int buffersize = 1024;
    fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (fd == -1) {
        perror("socket");
        return -1;
    }
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
    /* set nonblock */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL)");
        return -2;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        perror("fcntl(F_SETFL)");
        return -2;
    }

    struct sockaddr_nl snl;
    bzero(&snl, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;
    int ret = bind(fd, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if (ret < 0) {
        perror("bind");
        close(fd);
        return -3;
    }
    state = STATE_RUN;
    listenThread = std::thread(&UsbHotplug::run, this);
    return 0;
}

void UsbHotplug::stop()
{
    if (state == STATE_NONE) {
        return;
    }

    while (state != STATE_NONE) {
        std::unique_lock<std::mutex> locker(mutex);
        state = STATE_TERMINATE;
        condit.notify_all();
        condit.wait_for(locker, std::chrono::milliseconds(500), [=]()->bool{
                return state == STATE_NONE;
                });
    }
    listenThread.join();
    if (fd != -1) {
        close(fd);
    }
    return;
}
