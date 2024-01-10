#include "ob_type_helper.hpp"
#include "k4ainternal/logging.h"

#include <mutex>
#include <algorithm>

#if defined(_WIN32)
#include <sys/timeb.h>
#elif defined(__UNIX__) || defined(__APPLE__)
#include <time.h>
#endif


k4a_result_t check_ob_error(ob_error **error)
{
    if (*error)
    {
        auto msg = ob_error_message(*error);
        auto func = ob_error_function(*error);
        auto exception = ob_error_exception_type(*error);
        LOG_ERROR("Inner Orbbec SDK error: %s, function: %s, exception: %d", msg, func, exception);
        ob_delete_error(*error);
        *error = nullptr;
        return K4A_RESULT_FAILED;
    }
    return K4A_RESULT_SUCCEEDED;
}

void orbbec_sdk_log(ob_log_severity severity, const char *message, void *user_data)
{
    if (user_data)
    {
        // do nothing
    }
    switch (severity)
    {
    case OB_LOG_SEVERITY_DEBUG:
        LOG_TRACE("%s", message);
        break;
    case OB_LOG_SEVERITY_INFO:
        LOG_INFO("%s", message);
        break;
    case OB_LOG_SEVERITY_WARN:
        LOG_WARNING("%s", message);
        break;
    case OB_LOG_SEVERITY_ERROR:
        LOG_ERROR("%s", message);
        break;
    case OB_LOG_SEVERITY_FATAL:
        LOG_CRITICAL("%s", message);
        break;
    default:
        LOG_ERROR("Unknown severity: %d, message: %s", severity, message);
        break;
    }
}


std::mutex ob_ctx_mtx;
#ifdef CACHE_OB_CONTEXT
std::shared_ptr<ob_context_handler> ob_context_handler_instance = nullptr;
#else
std::weak_ptr<ob_context_handler> ob_context_handler_instance;
#endif

std::shared_ptr<ob_context_handler> get_ob_context_handler_instance(){
    std::lock_guard<std::mutex> lock(ob_ctx_mtx);
    
    #ifdef CACHE_OB_CONTEXT
    auto handler = ob_context_handler_instance;
    #else
    auto handler = ob_context_handler_instance.lock();
    #endif
    if (handler == nullptr)
    {
#ifdef CACHE_OB_CONTEXT
        LOG_INFO("Orbbec SDK Version:[%d.%d.%d]",
                 ob_get_major_version(),
                 ob_get_minor_version(),
                 ob_get_patch_version());

        LOG_INFO("Wrapper Version:[%d.%d.%d]", WRAPPER_VERSION_MAJOR, WRAPPER_VERSION_MINOR, WRAPPER_VERSION_PATCH);
#endif
        ob_error *error = nullptr;
        ob_set_logger_callback(OB_LOG_SEVERITY_DEBUG, orbbec_sdk_log, nullptr, &error);
        if (K4A_RESULT_FAILED == check_ob_error(&error))
        {
            return nullptr;
        }

        ob_context *context = ob_create_context(&error);
        if (K4A_RESULT_FAILED == check_ob_error(&error))
        {
            return nullptr;
        }
        handler = std::make_shared<ob_context_handler>(context);
        ob_context_handler_instance = handler;
    }

#ifdef CACHE_OB_CONTEXT
    return ob_context_handler_instance;
#else
    return ob_context_handler_instance.lock();
#endif
}

void on_device_changed_callback(ob_device_list *removed, ob_device_list *added, void *user_data)
{
    ob_error *error = nullptr;
    auto ctx_handler = (ob_context_handler *)user_data;

    auto dev_rm_count = ob_device_list_device_count(removed, &error);
    CHECK_OB_ERROR_RETURN(&error);
    for (uint32_t i = 0; i < dev_rm_count; i++)
    {
        auto uid = ob_device_list_get_device_uid(removed, i, &error);
        CHECK_OB_ERROR_RETURN(&error);

        auto it = std::find(ctx_handler->device_uid_list.begin(), ctx_handler->device_uid_list.end(), uid);
        if (it != ctx_handler->device_uid_list.end())
        {
            ctx_handler->device_uid_list.erase(it);
        }

        const char *name = ob_device_list_get_device_name(removed, i, &error);
        CHECK_OB_ERROR_RETURN(&error);

        const char *sn = ob_device_list_get_device_serial_number(removed, i, &error);
        CHECK_OB_ERROR_RETURN(&error);
        LOG_INFO("device removed: %s, sn=%s", name, sn);
    }

    auto dev_add_count = ob_device_list_device_count(added, &error);
    CHECK_OB_ERROR_RETURN(&error);
    for (uint32_t i = 0; i < dev_add_count; i++)
    {
        auto uid = ob_device_list_get_device_uid(added, i, &error);
        CHECK_OB_ERROR_RETURN(&error);

        auto it = std::find(ctx_handler->device_uid_list.begin(), ctx_handler->device_uid_list.end(), uid);
        if (it == ctx_handler->device_uid_list.end())
        {
            ctx_handler->device_uid_list.push_back(uid);
        }

        const char *name = ob_device_list_get_device_name(added, i, &error);
        CHECK_OB_ERROR_RETURN(&error);

        const char *sn = ob_device_list_get_device_serial_number(added, i, &error);
        CHECK_OB_ERROR_RETURN(&error);
        LOG_INFO("device added: %s, sn=%s", name, sn);
    }
}