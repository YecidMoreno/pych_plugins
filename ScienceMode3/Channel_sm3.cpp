#define PLUGIN_IO_NAME Channel_sm3
#define PLUGIN_IO_TYPE DeviceIO_plugin

#include <iostream>
#include <string.h>

#include "commIO/commIO_interface.h"
#include "deviceIO/deviceIO_interface.h"

#include "commIO/commIO_interface.h"


#include "core/json_api.h"
#include "core/core.h"
#include "core/logger.h"

#include <core/type_utils.h>

using namespace jsonapi;
using namespace std::chrono_literals;

#include "ScienceMode3_defs.h"

std::string mode; // Current || PulseWidth
enum
{
    CURRENT = 0,
    PULSE_WIDTH = 1
} ChannelMode_sm3;

int parseMode(const std::string &mode)
{
    if (mode == "mA")
        return CURRENT;
    if (mode == "pw")
        return PULSE_WIDTH;
    return CURRENT;
}

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{

    std::string device_str;
    DeviceIO_plugin *device = nullptr;
    ScienceMode3_write devide_arg;
    int max_mA = 10;
    int max_pw = 200;
    std::string mode;
    int mode_int = CURRENT;

public:
    virtual ~PLUGIN_IO_NAME() {

    };

    virtual bool config(const std::string &cfg) override
    {
        configured = false;
        _cfg = json_obj::from_string(cfg);

        configured = _cfg.get("channel", &devide_arg.channel) &&
                     _cfg.get("device", &device_str) &&
                     _cfg.get("mA", &devide_arg.mA) &&
                     _cfg.get("pw", &devide_arg.pw) &&
                     _cfg.get("Hz", &devide_arg.hz) &&
                     _cfg.get("max_mA", &max_mA) &&
                     _cfg.get("max_pw", &max_pw) &&
                     _cfg.get("mode", &mode) &&
                     _cfg.get("ramp", &devide_arg.ramp);

        auto &core = HH::Core::instance();
        device = core.pm_deviceIO.get_node(device_str);

        mode_int = parseMode(mode);

        if (!device)
            configured = false;

        return configured;
    }

    virtual bool connect(const std::string &target) override
    {
        if (!configured)
            return false;

        if (!device)
            return false;

        return true;
    }

    virtual void disconnect() override
    {
    }

    virtual ssize_t write(void *data, size_t size, const void *arg1 = nullptr) override
    {
        if (!device)
            return -1;

        if (!arg1)
        {
            const double &d = *static_cast<const double *>(data);
            const uint8_t dd = static_cast<const uint8_t>(d);

            if (mode_int == CURRENT)
            {
                devide_arg.mA = dd > max_mA ? max_mA : dd;
            }
            else if (mode_int == PULSE_WIDTH)
            {
                devide_arg.pw = dd > max_pw ? max_pw : dd;
            }

            return device->write(nullptr, 0, &devide_arg);
        }
        else
        {
            return device->write(nullptr, 0, arg1);
            
        }

        return -1;
    }

    virtual ssize_t read(void *buffer, size_t max_size, const void *arg1 = nullptr) override
    {
        return 0;
    }

    virtual bool command(uint32_t opcode, const void *arg = nullptr) override
    {
        return false;
    }
};

__FINISH_PLUGIN_IO;
