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
    std::vector<DeviceIO_plugin *> _in;
    std::vector<DeviceIO_plugin *> _out;

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
            _in.clear();

            for (auto &name : _cfg_str_array)
            {
                _in.push_back(core.pm_deviceIO.get_node(name));
            }
        }
        else
        {
            configured = false;
        }

        _cfg_str_array.clear();
        if (configured && _cfg.get("out", &_cfg_str_array))
        {
            _out.clear();

            for (auto &name : _cfg_str_array)
            {
                _out.push_back(core.pm_deviceIO.get_node(name));
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
        eval.t = core.get_run_time_double(1s);

        eval.parser.evaluate();

        if (_out.size() > 0)
        {
            _out[0]->write(&eval.y[0], 0);
        }

        hh_logi("y0: %f", eval.y[0]);
        return 0;
    }
};

__FINISH_PLUGIN_IO;
