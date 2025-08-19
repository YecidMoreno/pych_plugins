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
    std::array<DeviceIO_plugin *,10> _in_arr = {nullptr};
    std::array<DeviceIO_plugin *,10> _out_arr = {nullptr};

public:
    virtual ~PLUGIN_IO_NAME() {

    };

    virtual bool config(const std::string &cfg) override
    {
        auto &core = HH::Core::instance();
        configured = false;
        _cfg = json_obj::from_string(cfg);

        configured = get_eval_functional_from_json(_cfg, "eval", eval);
        CONTROL_IO_CONFIGURE_TASK_FROM_JSON_;

        std::vector<std::string> _cfg_str_array;
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

        return configured;
    }

    virtual int loop() override
    {
        auto &core = HH::Core::instance();
        eval.t = core.runner.get_run_time_double(1s);

        for (size_t i = 0; i < _in_arr.size(); ++i) {
            if(_in_arr[i]){
                _in_arr[i]->read(&eval.x[i],0);
            }else{
                break;
            }
        }

        eval.parser.evaluate();

        for (size_t i = 0; i < _out_arr.size(); ++i) {
            if(_out_arr[i]){
                _out_arr[i]->write(&eval.y[i],0);
            }else{
                break;
            }
        }

        
        return 0;
    }
};

__FINISH_PLUGIN_IO;
