#define PLUGIN_IO_NAME ScienceMode3
#define PLUGIN_IO_TYPE DeviceIO_plugin

#include <iostream>
#include <string.h>

#include "commIO/commIO_interface.h"
#include "deviceIO/deviceIO_interface.h"

#include "commIO/commIO_interface.h"
#include "src/plugins/commIO/CANOpenIO/CANOpenIO_defs.h"

#include <linux/can.h>

#include "core/json_api.h"
#include "core/core.h"
#include "core/logger.h"

#include <core/type_utils.h>

using namespace jsonapi;
using namespace std::chrono_literals;

#include "smpt_ml_client.h"
#include "ScienceMode3_defs.h"

class PLUGIN_IO_NAME : public PLUGIN_IO_TYPE
{

    struct
    {
        std::string iface;
        double mA[4] = {0, 0, 0, 0};
        double pw[4] = {0, 0, 0, 0};
        double hz[4] = {0, 0, 0, 0};
        double ramp[4] = {0, 0, 0, 0};

    } source;

    Smpt_device device = {0};

public:
    virtual ~PLUGIN_IO_NAME()
    {
        smpt_send_ml_stop(&device, smpt_packet_number_generator_next(&device));
        smpt_close_serial_port(&device);
    };

    virtual bool config(const std::string &cfg) override
    {
        configured = false;
        _cfg = json_obj::from_string(cfg);

        configured = _cfg.get("iface", &source.iface);

        return configured;
    }

    virtual bool connect(const std::string &target) override
    {
        if (!configured)
            return false;

        auto &core = HH::Core::instance();
        smpt_open_serial_port(&device, source.iface.c_str());

        Smpt_ml_init ml_init = {0}; /* Struct for ml_init command */
        fill_ml_init(&device, &ml_init);
        smpt_send_ml_init(&device, &ml_init); /* Send the ml_init command to the stimulation unit */

        Smpt_ml_update ml_update = {0}; /* Struct for ml_update command */
        fill_ml_update(&device, &ml_update);
        smpt_send_ml_update(&device, &ml_update);

        task_struct_t _task{0};
        _task._name = "fes0";
        _task._ts = 1s;
        _task._callback = [this]()
        { this->alive(); };

        core.add_to_task("ScienceMode3_task", _task);

        return true;
    }

    void alive()
    {
        Smpt_ml_get_current_data ml_get_current_data = {0};
        fill_ml_get_current_data(&device, &ml_get_current_data);
        smpt_send_ml_get_current_data(&device, &ml_get_current_data);
    }

    virtual void disconnect() override
    {
    }

    virtual ssize_t write(void *data, size_t size, const void *arg1 = nullptr) override
    {
        if (!arg1)
            return -1;

        const ScienceMode3_write &d = *static_cast<const ScienceMode3_write *>(arg1);
        source.mA[d.channel] = d.mA;
        source.hz[d.channel] = d.hz;
        source.pw[d.channel] = d.pw;
        source.ramp[d.channel] = d.ramp;

        Smpt_ml_update ml_update = {0};
        fill_ml_update(&device, &ml_update);
        smpt_send_ml_update(&device, &ml_update);

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
    void fill_ml_init(Smpt_device *const device, Smpt_ml_init *const ml_init)
    {
        /* Clear ml_init struct and set the data */
        smpt_clear_ml_init(ml_init);
        ml_init->packet_number = smpt_packet_number_generator_next(device);
    }

    void fill_ml_update(Smpt_device *const device, Smpt_ml_update *const ml_update)
    {
        smpt_clear_ml_update(ml_update);

        for (int idx = 0; idx < 4; idx++)
        {
            /* Clear ml_update and set the data */

            ml_update->enable_channel[idx] = source.mA[idx] > 0; /* Enable channel red */
            ml_update->packet_number = smpt_packet_number_generator_next(device);

            ml_update->channel_config[idx].number_of_points = 3;                      /* Set the number of points */
            ml_update->channel_config[idx].ramp = source.ramp[idx];                   /* Three lower pre-pulses   */
            ml_update->channel_config[idx].period = (1.0f / source.hz[idx]) * 1000.0; /* Frequency: 50 Hz */

            /* Set the stimulation pulse */
            /* First point, current: 20 mA, positive, pulse width: 200 µs */
            ml_update->channel_config[idx].points[0].current = source.mA[idx];
            ml_update->channel_config[idx].points[0].time = source.pw[idx];

            /* Second point, pause 100 µs */
            ml_update->channel_config[idx].points[1].time = source.pw[idx] / 2;

            /* Third point, current: -20 mA, negative, pulse width: 200 µs */
            ml_update->channel_config[idx].points[2].current = -source.mA[idx];
            ml_update->channel_config[idx].points[2].time = source.pw[idx];
        }
    }

    void fill_ml_get_current_data(Smpt_device *const device, Smpt_ml_get_current_data *const ml_get_current_data)
    {
        ml_get_current_data->packet_number = smpt_packet_number_generator_next(device);
        ml_get_current_data->data_selection[Smpt_Ml_Data_Stimulation] = true; /* get stimulation data */
    }
};

__FINISH_PLUGIN_IO;
