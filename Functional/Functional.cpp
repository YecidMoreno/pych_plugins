#define PLUGIN_IO_NAME Functional
#define PLUGIN_IO_TYPE ControlIO_plugin

#include <iostream>
#include <string.h>

#include "commIO/commIO_interface.h"
#include "deviceIO/deviceIO_interface.h"

#include "commIO/commIO_interface.h"

#include "core/json_api.h"
#include "core/core.h"
#include "core/logger.h"

#include <core/type_utils.h>
#include <utils/eval_struct.h>

#include <array>

using namespace jsonapi;
using namespace std::chrono_literals;

struct eval_t_functional
{
    bool enable = false;
    std::string to_eval = "";
    ExprtkParser parser;
    double x[10];
    double y[10];
    double t = 0.0;
    double aux[10];
};

bool get_eval_functional_from_json(json_obj &_cfg, std::string key, eval_t_functional &eval)
{
    std::vector<std::string> eval_arr;
    if (_cfg.get(key, &eval_arr))
    {
        eval.to_eval = "";
        for (size_t i = 0; i < eval_arr.size(); i++)
        {
            eval.to_eval += eval_arr[i] + (i + 1 == eval_arr.size() ? "" : " \n ");
        }
        eval.enable = true;
    }
    else
    {
        _cfg.get(key, &eval.to_eval);
        eval.enable = true;
    }

    if (eval.enable)
    {
        std::string var;

        var = "aux";
        for (int i = 0; i < 10; i++)
        {
            eval.parser.setVariable(std::string("aux") + std::to_string(i), &eval.aux[i]);
            eval.parser.setVariable(std::string("x") + std::to_string(i), &eval.x[i]);
            eval.parser.setVariable(std::string("y") + std::to_string(i), &eval.y[i]);
        }

        eval.parser.setVariable("t", &eval.t);

        if (!eval.parser.setExpression(eval.to_eval))
        {
            hh_loge("Failed to parse expression %s", eval.to_eval.c_str());
            hh_loge(">> %s", eval.parser.error().c_str());
            return false;
        }
    }

    return true;
}

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{

    eval_t_functional eval;
    std::array<DeviceIO_plugin *, 10> _in_arr = {nullptr};
    std::array<DeviceIO_plugin *, 10> _out_arr = {nullptr};
    std::vector<std::string> _cfg_str_array;

public:
    struct
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
    } _logger;

    virtual ~PLUGIN_IO_NAME() {

    };

    virtual bool config(const std::string &cfg) override
    {
        auto &core = HH::Core::instance();
        configured = false;
        _cfg = json_obj::from_string(cfg);

        configured = get_eval_functional_from_json(_cfg, "eval", eval);
        CONTROL_IO_CONFIGURE_TASK_FROM_JSON_;

        _cfg_str_array.clear();
        if (configured && _cfg.get("in", &_cfg_str_array))
        {
            _in_arr.fill(nullptr);
            auto _in_ptr = _in_arr.begin();
            for (auto &name : _cfg_str_array)
            {
                *_in_ptr++ = core.plugins.get_node<DeviceIO_plugin>(name);
            }
        }
        else
        {
            configured = false;
        }

        _cfg_str_array.clear();
        if (configured && _cfg.get("out", &_cfg_str_array))
        {
            _out_arr.fill(nullptr);
            auto _out_ptr = _out_arr.begin();
            for (auto &name : _cfg_str_array)
            {
                *_out_ptr++ = core.plugins.get_node<DeviceIO_plugin>(name);
            }
        }
        else
        {
            configured = false;
        }

        if (_cfg.get("log", &_logger.enabled))
        {
            if (_cfg.get("filename", &_logger.file_name))
            {
                if (_logger.enabled && _logger.file_name.empty())
                {
                    _logger.enabled = false;
                }
                else
                {
                    set_logger();
                }
            }
        }

        return configured;
    }

    void set_logger()
    {
        int var_count = 0;
        if (!_logger.enabled)
            return;

        auto &core = HH::Core::instance();

        this->_logger.file.emplace_back();
        this->_logger.file.back().set_file_name(core.config().logger.path + _logger.file_name + ".log");

        _logger.file[0].vars.push_back(RecordVariable{.name = "time", .format = "%.4f", ._ptr = &eval.t});

        _cfg_str_array.clear();
        var_count = 0;
        if (_cfg.get("in", &_cfg_str_array))
        {
            for (auto &name : _cfg_str_array)
            {
                _logger.file[0].vars.push_back(RecordVariable{.name = name.c_str(), .format = "%.4f", ._ptr = &eval.x[var_count++]});
            }
        }
        _cfg_str_array.clear();
        var_count = 0;
        if (_cfg.get("out", &_cfg_str_array))
        {
            for (auto &name : _cfg_str_array)
            {
                _logger.file[0].vars.push_back(RecordVariable{.name = name.c_str(), .format = "%.4f", ._ptr = &eval.y[var_count++]});
            }
        }
    }

    virtual int loop() override
    {
        auto &core = HH::Core::instance();
        eval.t = core.runner.get_run_time_double(1s);

        for (size_t i = 0; i < _in_arr.size(); ++i)
        {
            if (_in_arr[i])
            {
                _in_arr[i]->read(&eval.x[i], 0);
            }
            else
            {
                break;
            }
        }

        eval.parser.evaluate();

        for (size_t i = 0; i < _out_arr.size(); ++i)
        {
            if (_out_arr[i])
            {
                _out_arr[i]->write(&eval.y[i], 0);
            }
            else
            {
                break;
            }
        }

        _logger.loop();

        return 0;
    }
};

__FINISH_PLUGIN_IO;
