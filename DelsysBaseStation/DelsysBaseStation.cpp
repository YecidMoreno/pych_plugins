#define PLUGIN_IO_NAME DelsysBaseStation
#define PLUGIN_IO_TYPE DeviceIO_plugin

#include <iostream>
#include <string.h>

#include "commIO/commIO_interface.h"
#include "deviceIO/deviceIO_interface.h"

#include "commIO/commIO_interface.h"

#include <core/json_api.h>
#include "core/core.h"
#include "core/logger.h"

#include <core/type_utils.h>
#include <utils/utils_control.h>

#include <array>
#include <atomic>

#include <TCPClient/TCPClient_defs.h>

enum class CommandOp : uint8_t
{
    START,
    STOP,
    RESET,
    READ,
    CALIBRATE,
    INVALID
};

CommandOp parse_command(const std::string &cmd)
{
    static const std::unordered_map<std::string, CommandOp> map = {
        {"start", CommandOp::START},
        {"stop", CommandOp::STOP},
        {"reset", CommandOp::RESET},
        {"calibrate", CommandOp::CALIBRATE},
        {"read", CommandOp::READ},
    };
    auto it = map.find(cmd);
    return (it != map.end()) ? it->second : CommandOp::INVALID;
}

using namespace jsonapi;
using namespace std::chrono_literals;

struct Logger
{
    bool enabled = false;
    std::string file_name;
    std::vector<RecordVariables> file;
    inline void loop()
    {
        if (!enabled)
            return;
        for (auto &f : file)
        {
            f.loop();
        }
    }
};

std::array<LowPassFilter, 16> lp_emg;

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{

private:
    const std::string TERMINATOR = "\r\n\r\n";
    Logger _logger;
    double t_ms,t_core;
    std::atomic<bool> _started = false;

public:
    virtual ~PLUGIN_IO_NAME()
    {
        this->disconnect();
    };

    struct
    {
        std::string addr;
        uint32_t cmd_port = 50040;
        uint32_t emg_port = 50041;

        CommIO_plugin *comm_cmd = nullptr;
        CommIO_plugin *comm_emg = nullptr;

        std::vector<std::string> START_CMDs;
        std::vector<std::string> STOP_CMDs;

    } source;

    virtual bool config(const std::string &cfg) override
    {
        configured = false;
        _cfg = jsonapi::json_obj::from_string(cfg);

        configured = _cfg.get("addr", &source.addr) &&
                     _cfg.get("cmd_port", &source.cmd_port) &&
                     _cfg.get("emg_port", &source.emg_port);

        if (!configured)
        {
            hh_loge("[DelsysBaseStation] Algunos campos no fueron leidos correctamente.");
            return false;
        }

        auto &core = HH::Core::instance();

        core.plugins.add_node<CommIO_plugin>("delsys_cmd", "TCPClient");
        source.comm_cmd = core.plugins.get_node<CommIO_plugin>("delsys_cmd");

        core.plugins.add_node<CommIO_plugin>("delsys_emg", "TCPClient");
        source.comm_emg = core.plugins.get_node<CommIO_plugin>("delsys_emg");

        jsonapi::json_obj _cfg_cmd;
        _cfg_cmd.set("port", source.cmd_port);
        _cfg_cmd.set("addr", source.addr);
        if (!source.comm_cmd->config(_cfg_cmd.str()))
        {
            hh_logi("_cfg_cmd: %s", _cfg_cmd.str().c_str());
            hh_loge("No fue posibe configurar el objeto: delsys_cmds ");
            return false;
        }
        if (!source.comm_cmd->connect(""))
        {
            hh_loge("No fue posibe conectar el objeto: delsys_cmds ");
            return false;
        }

        _cfg_cmd.set("port", source.emg_port);
        _cfg_cmd.set("addr", source.addr);
        if (!source.comm_emg->config(_cfg_cmd.str()))
        {
            hh_logi("_cfg_cmd: %s", _cfg_cmd.str().c_str());
            hh_loge("No fue posibe configurar el objeto: delsys_emg ");
            return configured;
        }
        if (!source.comm_emg->connect(""))
        {
            hh_loge("No fue posibe conectar el objeto: delsys_emg ");
            return configured;
        }

        if (source.comm_cmd && source.comm_emg)
        {
            configured = true;
        }

        if (!_cfg.get("START_CMDs", &source.START_CMDs))
        {
            source.START_CMDs.clear();
            source.START_CMDs.push_back("START");
        }

        if (!_cfg.get("STOP_CMDs", &source.STOP_CMDs))
        {
            source.STOP_CMDs.clear();
            source.STOP_CMDs.push_back("STOP");
        }

        jsonapi::json_obj j_log;
        if (_cfg.get("log", &j_log))
        {
            j_log.get("enable", &_logger.enabled);
            j_log.get("file_name", &_logger.file_name);
            set_logger();
        }

        for (auto &f : lp_emg)
            f = LowPassFilter(100.0, 0.519e-3);

        return configured;
    }

    void set_logger()
    {
        if (!_logger.enabled)
            return;

        auto &core = HH::Core::instance();

        this->_logger.file.emplace_back();
        this->_logger.file.back().set_file_name(core.config().logger.path + _logger.file_name + ".log");

        _logger.file[0].vars.push_back(RecordVariable{.name = "time", .format = "%.4f", ._ptr = &t_ms});
        _logger.file[0].vars.push_back(RecordVariable{.name = "time_core", .format = "%.4f", ._ptr = &t_core});

        std::string tmp_name;
        for (int n = 0; n < 16; n++)
        {
            tmp_name = "emg" + std::to_string(n + 1);
            _logger.file[0].vars.push_back(RecordVariable{.name = tmp_name, .format = "%.4f", .fnc = [this, n]()
                                                                                              { return emg[n]; }});

            tmp_name = "emg_f" + std::to_string(n + 1);
            _logger.file[0].vars.push_back(RecordVariable{.name = tmp_name, .format = "%.4f", .fnc = [this, n]()
                                                                                              { return lp_emg[n].update(abs(emg[n])); }});
        }
    }

    virtual bool connect(const std::string &target) override
    {
        if (!configured)
            return false;

        std::string tmp_cmd;
        for (const auto &cmd : source.START_CMDs)
        {
            tmp_cmd = cmd + TERMINATOR;
            source.comm_cmd->send((void *)tmp_cmd.c_str(), tmp_cmd.length());
        }

        auto &core = HH::Core::instance();

        TCPClient_read_arg arg0;
        arg0.read_bytesAvailable = true;
        bool res = false;
        for (int i = 0; i < 40; i++)
        {
            res = source.comm_emg->receive(nullptr, 0, &arg0) > 0;
            if (res)
                break;
            std::this_thread::sleep_for(100ms);
        }

        if (!res)
        {
            hh_loge("Timeout for receive data from BaseStation");
            return false;
        }

        task_struct_t _task;
        _task._callback = [this]()
        { this->read(nullptr, 0); };
        _task._thread_name = "thr_priority_3";
        _task._ts = 13500us;
        _task._wakeup = 0s;
        _task._name = "delsys_read";
        configured = core.scheduler.add_to_task(_task._thread_name, _task);

        return true;
    }

    virtual void disconnect() override
    {
        std::string tmp_cmd;
        for (const auto &cmd : source.STOP_CMDs)
        {
            tmp_cmd = cmd + TERMINATOR;
            source.comm_cmd->send((void *)tmp_cmd.c_str(), tmp_cmd.length());
        }
    }

    virtual ssize_t write(void *data, size_t size, const void *arg1 = nullptr) override
    {
        source.comm_cmd->send(data, size);
        source.comm_cmd->send((void *)TERMINATOR.c_str(), TERMINATOR.length());
        return -1;
    }

    static constexpr int n_channels = 16;
    static constexpr int samples_per_frame = 26;
    static constexpr int sz_frame = n_channels * samples_per_frame * sizeof(float); // 1664 bytes por frame
    static constexpr int n_frames = 1;
    static constexpr int sz_final = sz_frame * n_frames;

    char tmp_buffer[sz_final + 128]; // margen extra por seguridad

    float *emg = nullptr;

    virtual ssize_t read(void *buffer, size_t max_size, const void *arg1 = nullptr) override
    {
        auto &core = HH::Core::instance();

        while (source.comm_emg->receive(tmp_buffer, sz_final) > 0)
        {
            for (int i = 0; i < n_frames; i++)
            {
                float *frame = (float *)(tmp_buffer + sz_frame * i);

                for (int s = 0; s < samples_per_frame; s++)
                {
                    emg = &frame[s * n_channels];

                    if (_started)
                    {
                        static auto &core = HH::Core::instance();
                        static double k_count = 0;
                        t_core = core.runner.get_run_time_double(1s);
                        // t_ms = (k_count++) * (1000 / 1925.925);
                        t_ms = (k_count++) * (1000 / 2000.0);
                        _logger.loop();
                    }
                }
            }
        }

        return 0;
    }

    virtual bool command(std::string cmd, const void *arg0 = nullptr, const void *arg1 = nullptr) override
    {
        switch (parse_command(cmd))
        {
        case CommandOp::START:
            _started = true;
            hh_logi("DelsysBaseStation started");
            while (source.comm_emg->receive(tmp_buffer, sz_final) > 0);
            break;
        }
        return true;
    }
};

__FINISH_PLUGIN_IO;
