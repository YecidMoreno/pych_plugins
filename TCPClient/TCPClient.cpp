#define PLUGIN_IO_NAME TCPClient
#define PLUGIN_IO_TYPE CommIO_plugin

#include "commIO/commIO_interface.h"

#include <atomic>

#include "core/core.h"
#include "core/json_api.h"
#include "core/logger.h"

#include <thread>
using namespace std::chrono_literals;

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <mutex>

#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "TCPClient_defs.h"

using namespace jsonapi;

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{
private:
    json_obj _cfg;
    bool configured = false;
    bool is_open = false;

    int sock = -1;
    struct
    {
        uint32_t port;
        std::string addr;
        std::string ack = "";

    } _config;

public:
    bool config(const std::string &cfg) override
    {
        _cfg = json_obj::from_string(cfg);
        configured = _cfg.get("port", &_config.port) && _cfg.get("addr", &_config.addr);

        _cfg.get("ack", &_config.ack);

        return configured;
    }

    bool connect(const std::string &) override
    {
        if (!configured)
            return false;

        if (is_open)
            return true;
        is_open = false;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            perror("Socket creation failed");
            return false;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(_config.port);
        inet_pton(AF_INET, _config.addr.c_str(), &server_addr.sin_addr);

        if (::connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("Connection failed");
            close(sock);
            return false;
        }
        fcntl(sock, F_SETFL, O_NONBLOCK);
        hh_logi("Connected to: %s:%d", _config.addr.c_str(), _config.port);
        is_open = true;

        if (_config.ack != "")
        {
            this->send((void *)_config.ack.c_str(), _config.ack.length(), nullptr);
        }

        return true;
    }

    ssize_t send(void *data, size_t length, const void *arg1) override
    {
        if (arg1)
        {
        }
        return ::send(sock, data, length, 0);
    }

    ssize_t receive(void *_buffer, size_t length, const void *arg) override
    {
        if (!is_open)
            return -1;

        int bytes_available = 0;
        ioctl(sock, FIONREAD, &bytes_available);

        if (arg)
        {
            const auto *arg0 = static_cast<const TCPClient_read_arg *>(arg);
            
            if (arg0->read_bytesAvailable)
                return bytes_available;
                
        }

        if (bytes_available < length)
        {
            return 0;
        }

        ssize_t received = ::recv(sock, _buffer, length, 0);

        if (received > 0)
        {
            // ((char*)_buffer)[received] = '\0';
            // hh_logn(">> %s",_buffer);
        }
        else if (received == 0)
        {
            hh_loge(">> Server closed connection");
        }
        else
        {
            hh_loge(">> Receive failed");
            perror("Receive failed");
        }

        return received;
    }
    virtual bool command(uint32_t opcode, const void *arg = nullptr)
    {
        return true;
    }

    void
    disconnect() override
    {
        if (sock != -1)
            ::close(sock);
    }

    PLUGIN_IO_NAME()
    {
    }

    ~PLUGIN_IO_NAME()
    {
        disconnect();
    }

    std::string get_type() override
    {
        return TO_STR(PLUGIN_IO_NAME);
    }
};

__FINISH_PLUGIN_IO;