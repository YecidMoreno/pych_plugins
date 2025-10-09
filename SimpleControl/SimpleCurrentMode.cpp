#include <iostream>
#include <string.h>
#include <vector>

#include <controlIO/controlIO_interface.h>

#include <math.h>
#include <chrono>

#include <core/core.h>
#include <core/logger.h>
#include <core/CoreScheduler/tasking.h>

#include <utils/utils_control.h>

#define PLUGIN_IO_NAME SimpleCurrentMode
#define PLUGIN_IO_TYPE ControlIO_plugin

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{

    float kp, kd,ki;

    std::chrono::nanoseconds ts;
    
    double pos0 = 0;
    double pos1 = 0;

    double t_ms, u;

    VariableTrace theta_in, theta_out;

public:
    virtual ~PLUGIN_IO_NAME() = default;

    virtual bool config(const std::string &cfg) override
    {
        CONTROL_IO_INIT_CONFIG(cfg);

        CONFIG_VALIDATE(_CFG_FIELD_GET(kp));
        CONFIG_VALIDATE(_CFG_FIELD_GET(kd));
        CONFIG_VALIDATE(_CFG_FIELD_GET(ki));



        CONFIG_VALIDATE(_CFG_FIELD_GET(ts));

        set_logger();

        CONTROL_IO_FINISH_CONFIG_FULL;
    }

    void set_logger()
    {
        if (!_logger.enabled)
            return;

        auto &core = HH::Core::instance();

        this->_logger.file.emplace_back();
        this->_logger.file.back().set_file_name(core.config().logger.path + _logger.file_name + ".log");

        _logger.file[0].vars.push_back(RecordVariable{.name = "time", .format = "%.4f", ._ptr = &t_ms});
        _logger.file[0].vars.push_back(RecordVariable{.name = "theta_in", .fnc = [this]()
                                                                      { return theta_in.v(); }});
        _logger.file[0].vars.push_back(RecordVariable{.name = "dtheta_in", .fnc = [this]()
                                                                        { return theta_in.d(); }});
        _logger.file[0].vars.push_back(RecordVariable{.name = "theta_out", .fnc = [this]()
                                                                      { return theta_out.v(); }});
        _logger.file[0].vars.push_back(RecordVariable{.name = "dtheta_out", .fnc = [this]()
                                                                        { return theta_out.d(); }});
        _logger.file[0].vars.push_back(RecordVariable{.name = "u", ._ptr = &u});
    }

    void print_event(std::chrono::nanoseconds now)
    {
        static auto next_time = now;
        if (now >= next_time)
        {
            next_time += 1s;
            // hh_logi("[SimpleImpedance] pos0: %8f\t pos1: %8f\t vel: %4f", pos0, pos1, vel);
        }
    }

    void send_control_signal(double u)
    {
        // for Current mode  
        int16_t vel_int = static_cast<int16_t>(u);

        // for Velocity mode  
        // int32_t vel_int = static_cast<int32_t>(u);

        _actuators[0]->write(&vel_int, 4);
    }

    virtual int loop() override
    {
        // Do not modify — handles timing and required imports
        __CONTROL_IO_BEGIN_LOOP();

        t_ms = core.runner.get_run_time_double(1ms);

        // Read sensors
        _sensors[0]->read(&pos0, 0);
        _sensors[1]->read(&pos1, 0);

        // For continuous-time controller: compute dt with lower bound
        __CONTROL_IO_GET_DT(1e-3);

        
        // Update tracked variables and compute their derivatives and integrals
        
        theta_in.update(pos0 *M_PI/180.0, dt);
        theta_out.update(pos1*M_PI/180.0, dt);

        /* P.Y.C.H. — Put Your Controller (Probably Overengineered) Here */
        u = sin(2*M_PI*0.1*t_ms/1000.0)*1000.0;

        // Apply control signal to actuator
        send_control_signal(u);
        

        // Finalize loop timing and logging
        __CONTROL_IO_END_LOOP();

        // print_event(now);
        _logger.loop();

        return 0;
    };
};

__FINISH_PLUGIN_IO;
