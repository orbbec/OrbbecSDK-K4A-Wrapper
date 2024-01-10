#pragma once

#include "k4a/k4a.h"
#include <libobsensor/h/Context.h>
#include <libobsensor/h/Error.h>
#include <libobsensor/h/Version.h>
#include <libobsensor/h/Device.h>
#include <memory>
#include <vector>
#include <string>

k4a_result_t check_ob_error(ob_error **error);

#define CHECK_OB_ERROR(ob_err) check_ob_error(ob_err)
#define OB_SUCCEEDED(error) (K4A_RESULT_SUCCEEDED == check_ob_error(error))
#define OB_FAILED(error) (K4A_RESULT_FAILED == check_ob_error(error))

#define CHECK_OB_ERROR_CONTINUE(error)                                                                                 \
    if (K4A_RESULT_FAILED == check_ob_error(error))                                                                    \
    {                                                                                                                  \
        continue;                                                                                                      \
    }

#define CHECK_OB_ERROR_BREAK(error)                                                                                    \
    if (K4A_RESULT_FAILED == check_ob_error(error))                                                                    \
    {                                                                                                                  \
        break;                                                                                                         \
    }

#define CHECK_OB_ERROR_RETURN(error)                                                                                   \
    if (K4A_RESULT_FAILED == check_ob_error(error))                                                                    \
    {                                                                                                                  \
        return;                                                                                                        \
    }

#define CHECK_OB_ERROR_RETURN_K4A_RESULT(ob_err)                                                                       \
    if (K4A_RESULT_FAILED == check_ob_error(ob_err))                                                                   \
    {                                                                                                                  \
        return K4A_RESULT_FAILED;                                                                                      \
    }

#define CHECK_OB_ERROR_RETURN_VALUE(ob_err, value)                                                                     \
    if (K4A_RESULT_FAILED == check_ob_error(ob_err))                                                                   \
    {                                                                                                                  \
        return value;                                                                                                  \
    }

ob_context *get_ob_context_instance();

void on_device_changed_callback(ob_device_list *removed, ob_device_list *added, void *user_data);
struct ob_context_handler
{
    ob_context_handler(ob_context *ctx) : context(ctx)
    {
        ob_error *error = nullptr;
        auto device_list = ob_query_device_list(context, &error);
        CHECK_OB_ERROR_RETURN(&error);

        auto device_count = ob_device_list_device_count(device_list, &error);
        CHECK_OB_ERROR_RETURN(&error);

        for (uint32_t i = 0; i < device_count; i++)
        {
            auto uid = ob_device_list_get_device_uid(device_list, i, &error);
            device_uid_list.push_back(uid);
        }
        ob_delete_device_list(device_list, &error);
        CHECK_OB_ERROR_RETURN(&error);

        ob_set_device_changed_callback(context, on_device_changed_callback, this, &error);
        CHECK_OB_ERROR_RETURN(&error);
    };

    ~ob_context_handler()
    {
        if (context != nullptr)
        {
            ob_error *error = nullptr;
            ob_delete_context(context, &error);
            check_ob_error(&error);
        }
    }
    ob_context *context;
    std::vector<std::string> device_uid_list;
};
std::shared_ptr<ob_context_handler> get_ob_context_handler_instance();
