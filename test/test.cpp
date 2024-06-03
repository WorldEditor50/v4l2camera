#include <camera/usbhotplug.h>
#include <cstdio>
#include <iostream>

void test_usbhotplug()
{
    UsbHotplug hotplug;
    hotplug.registerDevice("093A:2510", [](int action) {
        if (action == UsbHotplug::ACTION_DEVICE_ATTACHED) {
            std::cout<<"device arrived."<<std::endl;
        } else if (action == UsbHotplug::ACTION_DEVICE_DETACHED) {
            std::cout<<"device remove."<<std::endl;
        }
    });

    std::cout<<"start listening."<<std::endl;
    int ret = hotplug.start();
    if (ret != 0) {
        return;
    }
    getchar();
    std::cout<<"stop listening."<<std::endl;
    hotplug.stop();
    return;
}

int main()
{
    test_usbhotplug();
    return 0;
}
