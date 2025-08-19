#define PLUGIN_IO_NAME TCPDecode
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

    CommIO_plugin *comm = nullptr;
    struct
    {
        std::string comm_str;
        int length;
        int elements;

    } source;

    uint8_t recv_buffer[1024];
    float values[64]={0};

public:
    virtual ~PLUGIN_IO_NAME() {

    };

    virtual bool config(const std::string &cfg) override
    {
        configured = false;
        _cfg = json_obj::from_string(cfg);

        configured = _cfg.get("comm", &source.comm_str) && _cfg.get("elements", &source.elements);

        source.length = source.elements*sizeof(float) + 2;

        if (!configured)
            return false;

        auto &core = HH::Core::instance();
        comm = core.plugins.get_node<CommIO_plugin>(source.comm_str);

        if (!comm)
            configured = false;

        return configured;
    }

    virtual bool connect(const std::string &target) override
    {
        if (!configured)
            return false;

        if (!comm)
            return false;

        return true;
    }

    virtual void disconnect() override
    {
    }

    virtual ssize_t write(void *data, size_t size, const void *arg1 = nullptr) override
    {
        if (!comm)
            return -1;

        return -1;
    }

    virtual ssize_t read(void *buffer, size_t max_size, const void *arg1 = nullptr) override
    {
        if (!comm)
            return -1;

        std::string aux = "";
        bool use_buffer=true;

        while (comm->receive(recv_buffer, source.length) > 0)
        {
            use_buffer = false;
            
            double tmp = (double) *(float*)(recv_buffer+1);

            memcpy(buffer,&tmp,sizeof(double));
            memcpy(&values,recv_buffer+1,source.length-2);

            // hh_logn("recv_buffer: %s : tmp: %f",aux.c_str(),tmp);

        }

        if(use_buffer){
            memcpy(buffer,&values[0],sizeof(float));
        }

        return 0;
    }

    virtual bool command(std::string cmd, const void *arg0, const void *arg1)
    {
        return true;
    }
};

__FINISH_PLUGIN_IO;
