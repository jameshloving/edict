#include <iostream>
#include <string>

#include "device_log.cpp"

int main()
{
    device_log d;
    std::string mac, model;

    mac = "mac";
    model = "model";

    d.add_device(mac, model);
    d.read_devices();
}
