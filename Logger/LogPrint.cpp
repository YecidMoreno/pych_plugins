#define PLUGIN_IO_NAME LogPrint
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

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{

    struct
    {
        
    } source;



public:
    virtual ~PLUGIN_IO_NAME() {

    };

    virtual bool config(const std::string &cfg) override
    {
        configured = true;
        _cfg = json_obj::from_string(cfg);
        hh_logi(">> Log0");
        return configured;
    }

    virtual bool connect(const std::string &target) override
    {
        return true;
    }

    virtual void disconnect() override
    {
    }

    virtual ssize_t write(void *data, size_t size, const void *arg1 = nullptr) override
    {
        hh_logi(">> %f", *(double *)data);
        return -1;
    }

    virtual ssize_t read(void *buffer, size_t max_size, const void *arg1 = nullptr) override
    {
        return 0;
    }

    virtual bool command(std::string cmd, const void *arg0, const void *arg1)
    {
        return true;
    }
};

__FINISH_PLUGIN_IO;
