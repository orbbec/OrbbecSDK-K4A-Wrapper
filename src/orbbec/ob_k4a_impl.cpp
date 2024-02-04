// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4a/k4a.h>
#include "libobsensor/h/Context.h"
#include "libobsensor/h/Device.h"
#include "libobsensor/h/Error.h"
#include "libobsensor/h/Frame.h"
#include "libobsensor/h/ObTypes.h"
#include "libobsensor/h/Pipeline.h"
#include "libobsensor/h/Sensor.h"
#include "libobsensor/h/StreamProfile.h"
#include "libobsensor/h/Version.h"
#include "libobsensor/h/Filter.h"

// Dependent libraries
#include <k4ainternal/handle.h>
#include <k4ainternal/common.h>
#include <k4ainternal/calibration.h>
#include <k4ainternal/transformation.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/transformation.h>

#include <imusync.h>
#include <frame_queue.h>

#include "obmetadata.h"
#include "ob_type_helper.hpp"


// System dependencies
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// cpp
#include <memory>
#include <vector>

#define ORBBEC_MEGA_PID 0x0669
#define ORBBEC_BOLT_PID 0x066B

std::vector<int> get_effective_device(ob_context* &context){
    ob_error *ob_err = NULL;
    std::vector<int> effective_devices;
    ob_device_list *ob_dev_list = NULL;
    do{
        ob_dev_list = ob_query_device_list(context, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        uint32_t device_count = ob_device_list_device_count(ob_dev_list, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        int pid;
        for(uint32_t index=0; index < device_count; index++){
            pid = ob_device_list_get_device_pid(ob_dev_list, index, &ob_err);
            CHECK_OB_ERROR_BREAK(&ob_err);

            if(!(pid == ORBBEC_MEGA_PID || pid == ORBBEC_BOLT_PID)){
                const char * name = ob_device_list_get_device_name(ob_dev_list, index, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);

                const char *sn = ob_device_list_get_device_serial_number(ob_dev_list, index, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);

                LOG_INFO("Current device not supported, name = %s, sn = %s, pid = %d", name, sn, pid);
            }else{
                effective_devices.push_back(index);
            }
        }
    }while(0);

    if(ob_dev_list!=NULL){
        ob_delete_device_list(ob_dev_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
    }

    return effective_devices;
}
#include <string>
#include <sstream>

#ifdef __cplusplus
extern "C" {
#endif

k4a_device_clock_sync_mode_t current_device_clock_sync_mode;
char K4A_ENV_VAR_LOG_TO_A_FILE[] = K4A_ENABLE_LOG_TO_A_FILE;
// char K4A_ENV_VAR_LOG_TO_A_FILE[] = "";
#define MAX_FIREWARE_VERSION_LEN 64
#define UNREFERENCED_VALUE(P) ((void)P)

#define MAX_JSON_FILE_SIZE 1024 * 10
#define ORBBEC_MEGA_PID 0x0669
#define ORBBEC_BOLT_PID 0x066B
#define MAX_DELAY_TIME 33333
#define MIN_DELAY_TIME -33333

k4a_result_t k4a_device_get_calibration_from_json(k4a_device_t device_handle,
                                                  const k4a_depth_mode_t depth_mode,
                                                  const k4a_color_resolution_t color_resolution,
                                                  k4a_calibration_t *calibration);

typedef enum _ir_mode
{
    ACTIVE_IR = 0,
    PASSIVE_IR = 1,
} ir_mode_t;

typedef enum _json_status
{
    JSON_FILE_INVALID = 0,
    JSON_FILE_VALID = 1,
} json_staus_t;

typedef struct _calibration_json
{
    char *calibration_json;
    uint32_t json_max_size;
    uint32_t json_actual_size;
    uint32_t status;
} calibration_json_t;

typedef struct _k4a_device_context_t
{
    char serial_number[16];
    bool have_been_init_once;

    ob_device *device;
    ob_pipeline *pipe;
    frame_queue_t frameset_queue; // Queue for storing captures in

    imusync_t imusync;
    ob_sensor *accel_sensor;
    ob_sensor *gyro_sensor;

    calibration_json_t *json;
    k4a_transformation_t transformation;

    bool is_streaming;

#ifndef CACHE_OB_CONTEXT
    std::shared_ptr<ob_context_handler> ob_context_handler;
#endif
} k4a_device_context_t;

K4A_DECLARE_CONTEXT(k4a_device_t, k4a_device_context_t);

typedef struct _k4a_depthengine_instance_helper_t
{
    std::shared_ptr<depthengine_context> depthengine_instance_helper;
} k4a_depthengine_instance_helper_t;
K4A_DECLARE_CONTEXT(k4a_depthengine_t, k4a_depthengine_instance_helper_t);

#define DEPTH_CAPTURE (false)
#define COLOR_CAPTURE (true)
#define TRANSFORM_ENABLE_GPU_OPTIMIZATION (true)
#define K4A_DEPTH_MODE_TO_STRING_CASE(depth_mode)                                                                      \
    case depth_mode:                                                                                                   \
        return #depth_mode
#define K4A_COLOR_RESOLUTION_TO_STRING_CASE(color_resolution)                                                          \
    case color_resolution:                                                                                             \
        return #color_resolution
#define K4A_IMAGE_FORMAT_TO_STRING_CASE(image_format)                                                                  \
    case image_format:                                                                                                 \
        return #image_format
#define K4A_FPS_TO_STRING_CASE(fps)                                                                                    \
    case fps:                                                                                                          \
        return #fps

k4a_result_t k4a_depth_engine_helper_create(k4a_depthengine_t* handle){
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, handle == NULL);
    auto ob_depth_engine_handler = depthengine_instance_create();
    k4a_depthengine_t depthengine_handle = NULL;
    k4a_depthengine_instance_helper_t *depthengine_ctx = k4a_depthengine_t_create(&depthengine_handle);
    depthengine_ctx->depthengine_instance_helper = ob_depth_engine_handler;

    k4a_result_t result = K4A_RESULT_FROM_BOOL(depthengine_ctx != NULL);
    if (K4A_FAILED(result))
    {
        k4a_depthengine_t_destroy(depthengine_handle);
        depthengine_handle = NULL;
        return result;
    }
    *handle = depthengine_handle;
    return result;
}
void k4a_depth_engine_helper_release(k4a_depthengine_t handle){
    k4a_depthengine_instance_helper_t *depthengine_ctx = k4a_depthengine_t_get_context(handle);
    depthengine_ctx->depthengine_instance_helper.reset();
    k4a_depthengine_t_destroy(handle);
}

uint32_t k4a_device_get_installed_count(void)
{

    auto ob_context_handler = get_ob_context_handler_instance();
    ob_context *context = ob_context_handler->context;
    std::vector<int> effective_devices = get_effective_device(context);

    return (uint32_t)effective_devices.size();
}

k4a_result_t k4a_set_debug_message_handler(k4a_logging_message_cb_t *message_cb,
                                           void *message_cb_context,
                                           k4a_log_level_t min_level)
{
    return logger_register_message_callback(message_cb, message_cb_context, min_level);
}

k4a_result_t k4a_set_allocator(k4a_memory_allocate_cb_t allocate, k4a_memory_destroy_cb_t free)
{
    k4a_result_t result = K4A_RESULT_FAILED;

    LOG_WARNING("unsupported api [allocate=%d,free=%d]", allocate, free);
    return result;
}

void ob_frame_set_ready(ob_frame *frame_set, void *user_data)
{
    if (frame_set == NULL)
    {
        LOG_ERROR("frame_set is null", 0);
        return;
    }

    k4a_device_t device_handle = (k4a_device_t)user_data;
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    ob_error *ob_err = NULL;
    if (device_ctx == NULL)
    {
        ob_delete_frame(frame_set, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
        LOG_ERROR("device_ctx is null ", 0);
        return;
    }

    frame_queue_push(device_ctx->frameset_queue, (k4a_capture_t)frame_set);
}

void ob_get_json_callback(ob_data_tran_state state, ob_data_chunk *data_chunk, void *user_data)
{
    if (data_chunk == NULL)
    {
        LOG_ERROR("data_chunk is null", 0);
        return;
    }

    k4a_device_context_t* device_ctx = (k4a_device_context_t*)user_data;
    if (device_ctx->json == NULL || device_ctx->json->calibration_json == NULL)
    {
        LOG_ERROR("json memory is null", 0);
        return;
    }

    if (device_ctx->json->status == JSON_FILE_VALID)
    {
        return;
    }

    if (state == DATA_TRAN_STAT_TRANSFERRING)
    {
        device_ctx->json->json_actual_size = data_chunk->fullDataSize;
        if (device_ctx->json->json_actual_size > device_ctx->json->json_max_size)
        {
            LOG_ERROR("json memory is too small", 0);
            return;
        }
        memcpy(device_ctx->json->calibration_json + data_chunk->offset, data_chunk->data, data_chunk->size);
    }
    else if (state == DATA_TRAN_STAT_DONE)
    {
        if (device_ctx->json->json_actual_size % 2 == 1 &&
            device_ctx->json->calibration_json[device_ctx->json->json_actual_size - 1] != 0x7d)
        {
            device_ctx->json->calibration_json[device_ctx->json->json_actual_size - 1] = 0x7d;
        }
        device_ctx->json->status = JSON_FILE_VALID;
        device_ctx->json->calibration_json[device_ctx->json->json_actual_size] = '\0';
    }
    else
    {
        device_ctx->json->status = JSON_FILE_INVALID;
    }
}

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_handle == NULL);

    auto ob_context_handler = get_ob_context_handler_instance();
    ob_context *ob_ctx = ob_context_handler->context;
    std::vector<int> effective_device = get_effective_device(ob_ctx);
    if(index >= effective_device.size()){
        LOG_INFO("index is out of range", 0);
        return K4A_RESULT_FAILED;
    }
    index = effective_device[index];

    k4a_device_context_t *device_ctx = NULL;
    k4a_device_t handle = NULL;
    device_ctx = k4a_device_t_create(&handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(device_ctx != NULL);

    if (K4A_FAILED(result))
    {
        k4a_device_t_destroy(handle);
        handle = NULL;
        return result;
    }

    // clear context
    device_ctx->have_been_init_once = false;
    device_ctx->device = NULL;
    device_ctx->pipe = NULL;
    device_ctx->frameset_queue = NULL;
    device_ctx->json = NULL;
    device_ctx->accel_sensor = NULL;
    device_ctx->gyro_sensor = NULL;
    device_ctx->imusync = NULL;
    device_ctx->is_streaming = false;

#ifndef CACHE_OB_CONTEXT
    device_ctx->ob_context_handler = ob_context_handler;
#endif
    ob_device_list *dev_list = NULL;
    ob_error *ob_err = NULL;

    result = K4A_RESULT_FAILED;
    do
    {
        dev_list = ob_query_device_list(ob_ctx, &ob_err);

        CHECK_OB_ERROR_BREAK(&ob_err);

        const char *sn = ob_device_list_get_device_serial_number(dev_list, index, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);
        if (strlen(sn) > 16)
        {
            LOG_ERROR("length of device serial number error: %s", sn);
        }
        memcpy(device_ctx->serial_number, sn, 16);
        *device_handle = handle;

        result = K4A_RESULT_SUCCEEDED;
    } while (0);

    // release
    if (dev_list != NULL)
    {
        ob_delete_device_list(dev_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
    }

    return result;
}

k4a_result_t update_imu_raw_calibration_data_from_orbbec_sdk(k4a_device_context_t *device_ctx){
    k4a_result_t result = K4A_RESULT_FAILED;
    if(device_ctx->json == NULL|| device_ctx->json->status == JSON_FILE_INVALID){
        return result;
    }

    ob_error *ob_err = NULL;
    ob_calibration_param calibration_param = ob_pipeline_get_calibration_param(device_ctx->pipe, nullptr, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    std::string calibration_json_str = device_ctx->json->calibration_json;
    // gyro
    size_t offset = calibration_json_str.find("CALIBRATION_InertialSensorId_LSM6DSM", 0);
    size_t begin = calibration_json_str.find("\"Rt\": {\"Rotation\": [", offset);
    size_t end = calibration_json_str.find("]},", begin) +3;

    k4a_calibration_extrinsics_t *depth_to_gyro_extrinsics  = (k4a_calibration_extrinsics_t*)&calibration_param.extrinsics[OB_SENSOR_DEPTH][OB_SENSOR_ACCEL];

    std::stringstream ss;
    ss << "\"Rt\": {\"Rotation\": [" << depth_to_gyro_extrinsics->rotation[0] << ","
        << depth_to_gyro_extrinsics->rotation[1] << "," << depth_to_gyro_extrinsics->rotation[2] << ","
        << depth_to_gyro_extrinsics->rotation[3] << "," << depth_to_gyro_extrinsics->rotation[4] << ","
        << depth_to_gyro_extrinsics->rotation[5] << "," << depth_to_gyro_extrinsics->rotation[6] << ","
        << depth_to_gyro_extrinsics->rotation[7] << "," << depth_to_gyro_extrinsics->rotation[8] << "], \"Translation\": ["
        << depth_to_gyro_extrinsics->translation[0] / 1000.f << "," << depth_to_gyro_extrinsics->translation[1] / 1000.f << ","
        << depth_to_gyro_extrinsics->translation[2] / 1000.f << "]},";
    calibration_json_str.replace(begin, end - begin, ss.str());

    // accel
    offset = calibration_json_str.find("CALIBRATION_InertialSensorId_LSM6DSM", begin);
    begin = calibration_json_str.find("\"Rt\": {\"Rotation\": [", offset);
    end = calibration_json_str.find("]},", begin) +3;

    k4a_calibration_extrinsics_t *depth_to_accel_extrinsics  = (k4a_calibration_extrinsics_t*)&calibration_param.extrinsics[OB_SENSOR_DEPTH][OB_SENSOR_ACCEL];
    std::stringstream ss2;
    ss2 << "\"Rt\": {\"Rotation\": [" << depth_to_accel_extrinsics->rotation[0] << ","
        << depth_to_accel_extrinsics->rotation[1] << ","<< depth_to_accel_extrinsics->rotation[2] << ","
        << depth_to_accel_extrinsics->rotation[3] << "," << depth_to_accel_extrinsics->rotation[4] << ","
        << depth_to_accel_extrinsics->rotation[5] << "," << depth_to_accel_extrinsics->rotation[6] << ","
        << depth_to_accel_extrinsics->rotation[7] << "," << depth_to_accel_extrinsics->rotation[8] << "], \"Translation\": ["
        << depth_to_accel_extrinsics->translation[0] / 1000.f << "," << depth_to_accel_extrinsics->translation[1] / 1000.f << ","
        << depth_to_accel_extrinsics->translation[2] / 1000.f << "]},";
    calibration_json_str.replace(begin, end - begin, ss2.str());

    memcpy(device_ctx->json->calibration_json, calibration_json_str.c_str(), calibration_json_str.size());
    device_ctx->json->calibration_json[calibration_json_str.size()] = '\0';
    device_ctx->json->json_actual_size = (uint32_t)calibration_json_str.size();

    result = K4A_RESULT_SUCCEEDED;
    return result;
}


k4a_result_t fetch_raw_calibration_data(k4a_device_context_t *device_ctx)
{
    k4a_result_t cali_create_rst = K4A_RESULT_FAILED;
    ob_device_info *device_info = NULL;
    ob_error *ob_err = NULL;

    do{
        device_info = ob_device_get_device_info(device_ctx->device, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        int pid = ob_device_info_pid(device_info, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        if (pid == ORBBEC_MEGA_PID || pid == ORBBEC_BOLT_PID){
            if (device_ctx->json == NULL)
            {
                device_ctx->json = (calibration_json_t *)malloc(sizeof(calibration_json_t));
                device_ctx->json->json_max_size = MAX_JSON_FILE_SIZE;
                device_ctx->json->json_actual_size = 0;
                device_ctx->json->calibration_json = (char *)malloc(device_ctx->json->json_max_size);
                device_ctx->json->status = JSON_FILE_INVALID;
            }

            ob_device_get_raw_data(device_ctx->device,
                                OB_RAW_DATA_CAMERA_CALIB_JSON_FILE,
                                ob_get_json_callback,
                                false,
                                device_ctx,
                                &ob_err);
            CHECK_OB_ERROR_BREAK(&ob_err);

            cali_create_rst = update_imu_raw_calibration_data_from_orbbec_sdk(device_ctx);
        }
    } while(0);

    if (device_info != NULL)
    {
        ob_delete_device_info(device_info, &ob_err);
        CHECK_OB_ERROR(&ob_err);
    }
    return cali_create_rst;
}

k4a_result_t init_device_context(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    auto ob_context_handler = get_ob_context_handler_instance();
    ob_context *ob_ctx = ob_context_handler->context;
    ob_device_list *dev_list = NULL;
    ob_error *ob_err = NULL;

    k4a_result_t result = K4A_RESULT_FAILED;
    do
    {
        dev_list = ob_query_device_list(ob_ctx, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        device_ctx->device = ob_device_list_get_device_by_serial_number(dev_list, device_ctx->serial_number, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        device_ctx->pipe = ob_create_pipeline_with_device((ob_device *)device_ctx->device, &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        if (K4A_FAILED(fetch_raw_calibration_data(device_ctx))){
            break;
        }

        if (K4A_FAILED(TRACE_CALL(imusync_create(&device_ctx->imusync))))
        {
            break;
        }

        if (K4A_FAILED(TRACE_CALL(frame_queue_create(
                FRAME_QUEUE_DEFAULT_SIZE / 2, "frame_set", &device_ctx->frameset_queue, k4a_capture_release))))
        {
            break;
        }

        frame_queue_disable(device_ctx->frameset_queue);

        #ifdef RESET_DEVICE_TIMESTAMP_MODE
            // reset device timestamp
            reset_device_timestamp(device_handle, 0);
        #else
            // sync devices timer with host
            ob_enable_device_clock_sync(ob_ctx, 60000, &ob_err);
        #endif
        CHECK_OB_ERROR_BREAK(&ob_err);

        result = K4A_RESULT_SUCCEEDED;
    } while (0);

    // release
    if (dev_list != NULL)
    {
        ob_delete_device_list(dev_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
    }

    if (result == K4A_RESULT_FAILED)
    {
        if (device_ctx->device)
        {
            ob_delete_device((ob_device *)device_ctx->device, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            device_ctx->device = NULL;
        }

        if(device_ctx->pipe)
        {
            ob_delete_pipeline(device_ctx->pipe, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            device_ctx->pipe = NULL;
        }

        if (device_ctx->frameset_queue)
        {
            frame_queue_destroy(device_ctx->frameset_queue);
        }

        if (device_ctx->imusync != NULL)
        {
            imusync_destroy(device_ctx->imusync);
        }

        if (device_ctx->json != NULL)
        {
            if (device_ctx->json->calibration_json != NULL)
            {
                free(device_ctx->json->calibration_json);
                device_ctx->json->calibration_json = NULL;
            }

            free(device_ctx->json);
            device_ctx->json = NULL;
        }
    }

    return result;
}

#define CHECK_AND_TRY_INIT_DEVICE_CONTEXT(_fail_value_, device_handle)                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
                                                                                                                       \
        RETURN_VALUE_IF_HANDLE_INVALID(_fail_value_, k4a_device_t, device_handle);                                     \
        k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);                                    \
        if (!device_ctx->have_been_init_once)                                                                          \
        {                                                                                                              \
            device_ctx->have_been_init_once = true;                                                                    \
            init_device_context(device_handle);                                                                        \
        }                                                                                                              \
        if (device_ctx->device == NULL)                                                                                \
        {                                                                                                              \
            return _fail_value_;                                                                                       \
        }                                                                                                              \
    } while (0);

void switch_device_clock_sync_mode(k4a_device_t device_handle, uint32_t clock_sync_time, k4a_device_clock_sync_mode_t timestamp_mode){
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(VOID_VALUE, device_handle);
    ob_error *ob_err = NULL;
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    auto ob_context_handler = get_ob_context_handler_instance();
    do{
        OB_DEVICE_SYNC_CONFIG ob_sync_config;
        memset(&ob_sync_config, 0, sizeof(OB_DEVICE_SYNC_CONFIG));
        uint32_t len;

        ob_device_get_structured_data(device_ctx->device,
                                      OB_STRUCT_MULTI_DEVICE_SYNC_CONFIG,
                                      &ob_sync_config,
                                      &len,
                                      &ob_err);
        CHECK_OB_ERROR_BREAK(&ob_err);

        if(ob_sync_config.syncMode == OB_SYNC_MODE_PRIMARY_MCU_TRIGGER) {
            if(timestamp_mode == K4A_TIMESTAMP_RESET){
                ob_enable_device_clock_sync(ob_context_handler->context, 0, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);
                ob_device_timestamp_reset_config reset_config = {true, static_cast<int>(clock_sync_time*1000), true};
                const ob_device_timestamp_reset_config* device_timestamp_reset_config = &reset_config;
                ob_device_set_timestamp_reset_config(device_ctx->device, device_timestamp_reset_config, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);
                ob_device_timestamp_reset(device_ctx->device, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);
                current_device_clock_sync_mode = K4A_TIMESTAMP_RESET;
            }else{
                ob_device_timestamp_reset_config reset_config = {false, 0, false};
                const ob_device_timestamp_reset_config* device_timestamp_reset_config = &reset_config;
                ob_device_set_timestamp_reset_config(device_ctx->device, device_timestamp_reset_config, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);
                ob_enable_device_clock_sync(ob_context_handler->context, clock_sync_time, &ob_err);
                CHECK_OB_ERROR_BREAK(&ob_err);
                current_device_clock_sync_mode = K4A_TIMESTAMP_SYNC;
            }
        }
    }while(0);
}

void k4a_device_close(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    ob_error *ob_err = NULL;

    if( device_ctx->is_streaming ){
        k4a_device_stop_cameras(device_handle);
    }

    if (device_ctx->device)
    {
        ob_delete_device((ob_device *)device_ctx->device, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        device_ctx->device = NULL;
    }

    if (device_ctx->pipe)
    {
        ob_delete_pipeline(device_ctx->pipe, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        device_ctx->pipe = NULL;
    }

    if (device_ctx->frameset_queue)
    {
        frame_queue_destroy(device_ctx->frameset_queue);
    }

    if (device_ctx->imusync != NULL)
    {
        imusync_destroy(device_ctx->imusync);
    }

    if (device_ctx->json != NULL)
    {
        if (device_ctx->json->calibration_json != NULL)
        {
            free(device_ctx->json->calibration_json);
            device_ctx->json->calibration_json = NULL;
        }

        free(device_ctx->json);
        device_ctx->json = NULL;
    }
    k4a_device_t_destroy(device_handle);
}

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle,
                                         k4a_capture_t *capture_handle,
                                         int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_WAIT_RESULT_FAILED, device_handle);
    if (capture_handle == NULL)
    {
        LOG_WARNING("param invalid", 0);
        return K4A_WAIT_RESULT_FAILED;
    }

    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    k4a_wait_result_t result = K4A_WAIT_RESULT_SUCCEEDED;

    result = frame_queue_pop(device_ctx->frameset_queue, timeout_in_ms, (k4a_capture_t *)capture_handle);

    // for test
    // if (result == K4A_WAIT_RESULT_SUCCEEDED)
    // {
    //     static int count = 0;
    //     ob_frame *frameset = (ob_frame *)*capture_handle;
    //     ob_error *ob_err = NULL;
    //     ob_frame *color_frame = ob_frameset_color_frame(frameset, &ob_err);
    //     uint64_t color_timestamp = ob_frame_time_stamp_us(color_frame, &ob_err);
    //     ob_frame *depth_frame = ob_frameset_depth_frame(frameset, &ob_err);
    //     uint64_t depth_timestamp = ob_frame_time_stamp_us(depth_frame, &ob_err);
    //     ob_frame *ir_frame = ob_frameset_ir_frame(frameset, &ob_err);
    //     uint64_t ir_timestamp = ob_frame_time_stamp_us(ir_frame, &ob_err);
    //     LOG_ERROR("color_timestamp =%lld,depth_timestamp =%lld,ir_timestamp=%lld,count=%d",
    //               color_timestamp,
    //               depth_timestamp,
    //               ir_timestamp,
    //               count++);
    //     ob_delete_frame(color_frame, &ob_err);
    //     ob_delete_frame(depth_frame, &ob_err);
    //     ob_delete_frame(ir_frame, &ob_err);
    // }

    switch (result)
    {
    case K4A_WAIT_RESULT_FAILED:
        LOG_WARNING("frame_queue_pop failed", 0);
        result = K4A_WAIT_RESULT_FAILED;
        break;
    case K4A_WAIT_RESULT_TIMEOUT:
        LOG_WARNING("frame_queue_pop timeout", 0);
        result = K4A_WAIT_RESULT_TIMEOUT;
        break;
    default:
        break;
    }

    return result;
}

void ob_accel_frame(ob_frame *frame, void *user_data)
{
    k4a_device_t device_handle = (k4a_device_t)user_data;
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    if (frame == NULL)
    {
        return;
    }

    ob_error *ob_err = NULL;
    if (device_ctx->imusync == NULL)
    {
        ob_delete_frame(frame, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
        return;
    }

    ob_accel_value accel_value = ob_accel_frame_value(frame, &ob_err);

    CHECK_OB_ERROR_RETURN(&ob_err);
    uint64_t timestamp = ob_frame_time_stamp_us(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
    float temperature = ob_accel_frame_temperature(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);

    imu_frame_data imu_data;
    imu_data.timestamp = timestamp;
    memcpy(imu_data.data, &accel_value, sizeof(accel_value));
    imu_data.temp = temperature;

    imusync_push_frame(device_ctx->imusync, imu_data, ACCEL_FRAME_TYPE);
    // LOG_ERROR("timestamp =%lld", timestamp);
    ob_delete_frame(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
}

void ob_gyro_frame(ob_frame *frame, void *user_data)
{
    k4a_device_t device_handle = (k4a_device_t)user_data;
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    if (frame == NULL)
    {
        return;
    }

    ob_error *ob_err = NULL;
    if (device_ctx->imusync == NULL)
    {
        ob_delete_frame(frame, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
        return;
    }

    ob_gyro_value gyro_value = ob_gyro_frame_value(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
    uint64_t timestamp = ob_frame_time_stamp_us(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
    float temperature = ob_gyro_frame_temperature(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);

    imu_frame_data imu_data;
    imu_data.timestamp = timestamp;
    memcpy(imu_data.data, &gyro_value, sizeof(gyro_value));
    imu_data.temp = temperature;
    imusync_push_frame(device_ctx->imusync, imu_data, GYRO_FRAME_TYPE);
    // LOG_ERROR("gyro.x =%f,gyro.y =%f,gyro.z=f", gyro_value.x,gyro_value.y,gyro_value.z);

    ob_delete_frame(frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
}

k4a_wait_result_t k4a_device_get_imu_sample(k4a_device_t device_handle,
                                            k4a_imu_sample_t *imu_sample,
                                            int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, k4a_device_t, device_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, imu_sample == NULL);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_WAIT_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    if (device_ctx->imusync == NULL)
    {
        return K4A_WAIT_RESULT_FAILED;
    }

    k4a_wait_result_t result = imusync_get_frame(device_ctx->imusync, imu_sample, timeout_in_ms);
    // LOG_ERROR("imu_sample.acc.timestamp =%lld,gyro.timestamp =%lld,temperate=%f,result =%d",
    //           imu_sample->acc_timestamp_usec,
    //           imu_sample->gyro_timestamp_usec,imu_sample->temperature,result);
    return result;
}

k4a_result_t k4a_device_start_imu(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    ob_error *ob_err = NULL;
    OB_DEVICE_SYNC_CONFIG ob_sync_config;
    memset(&ob_sync_config, 0, sizeof(OB_DEVICE_SYNC_CONFIG));
    uint32_t len;
    ob_device_get_structured_data(device_ctx->device,
                                    OB_STRUCT_MULTI_DEVICE_SYNC_CONFIG,
                                    &ob_sync_config,
                                    &len,
                                    &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    if(current_device_clock_sync_mode == K4A_TIEMSTAMP_UNKNOWN){
        auto sync_config = ob_device_get_sync_config(device_ctx->device, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if((ob_sync_config.syncMode == OB_SYNC_MODE_PRIMARY_MCU_TRIGGER)) {
            switch_device_clock_sync_mode(device_handle, 0, K4A_TIMESTAMP_RESET);
            current_device_clock_sync_mode = K4A_TIMESTAMP_RESET;
        }
    }

    if (device_ctx->device == NULL)
    {
        LOG_ERROR("device_ctx->device == NULL", 0);
        return K4A_RESULT_FAILED;
    }

    ob_sensor_list *sensor_list = ob_device_get_sensor_list(device_ctx->device, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    ob_sensor *accel_sensor = ob_sensor_list_get_sensor_by_type(sensor_list, OB_SENSOR_ACCEL, &ob_err);
    if (ob_err != NULL)
    {
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor_list(sensor_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        return K4A_RESULT_FAILED;
    }

    ob_stream_profile_list *accel_profile_list = ob_sensor_get_stream_profile_list(accel_sensor, &ob_err);
    if (ob_err != NULL)
    {
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor_list(sensor_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(accel_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        return K4A_RESULT_FAILED;
    }

    uint32_t count = ob_stream_profile_list_count(accel_profile_list, &ob_err);
    if (ob_err != NULL)
    {
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor_list(sensor_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(accel_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        return K4A_RESULT_FAILED;
    }

    if (count > 0)
    {
        ob_stream_profile *profile = NULL;
        for (uint32_t i = 0; i < count; i++)
        {
            profile = ob_stream_profile_list_get_profile(accel_profile_list, i, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);

            ob_accel_sample_rate accel_rate = ob_accel_stream_profile_sample_rate(profile, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);

            ob_accel_full_scale_range accel_range = ob_accel_stream_profile_full_scale_range(profile, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);

            if (accel_rate == OB_SAMPLE_RATE_500_HZ && accel_range == OB_ACCEL_FS_4g)
            {
                break;
            }

            ob_delete_stream_profile(profile, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);
        }

        ob_sensor_start(accel_sensor, profile, ob_accel_frame, device_handle, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile(profile, &ob_err);
        if (ob_err != NULL)
        {
            CHECK_OB_ERROR(&ob_err);
            ob_delete_sensor_list(sensor_list, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            ob_delete_stream_profile_list(accel_profile_list, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            return K4A_RESULT_FAILED;
        }
    }

    ob_sensor *gyro_sensor = ob_sensor_list_get_sensor_by_type(sensor_list, OB_SENSOR_GYRO, &ob_err);
    if (ob_err != NULL)
    {
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor_list(sensor_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(accel_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        return K4A_RESULT_FAILED;
    }

    ob_stream_profile_list *gyro_profile_list = ob_sensor_get_stream_profile_list(gyro_sensor, &ob_err);
    if (ob_err != NULL)
    {
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor(gyro_sensor,  &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor_list(sensor_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(accel_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(gyro_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        return K4A_RESULT_FAILED;
    }

    count = ob_stream_profile_list_count(gyro_profile_list, &ob_err);
    if (ob_err != NULL)
    {
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor(gyro_sensor,  &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor_list(sensor_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(accel_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile_list(gyro_profile_list, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        return K4A_RESULT_FAILED;
    }

    if (count > 0)
    {
        ob_stream_profile *profile = NULL;
        for (uint32_t i = 0; i < count; i++)
        {
            profile = ob_stream_profile_list_get_profile(gyro_profile_list, i, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);

            ob_gyro_sample_rate gyro_rate = ob_gyro_stream_profile_sample_rate(profile, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);

            ob_gyro_full_scale_range gyro_range = ob_gyro_stream_profile_full_scale_range(profile, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);

            if (gyro_rate == OB_SAMPLE_RATE_500_HZ && gyro_range == OB_GYRO_FS_500dps)
            {
                break;
            }

            ob_delete_stream_profile(profile, &ob_err);
            CHECK_OB_ERROR_CONTINUE(&ob_err);
        }

        ob_sensor_start(gyro_sensor, profile, ob_gyro_frame, device_handle, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_stream_profile(profile, &ob_err);
        if (ob_err != NULL)
        {
            CHECK_OB_ERROR(&ob_err);
            ob_delete_sensor_list(sensor_list, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            ob_delete_stream_profile_list(accel_profile_list, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            ob_delete_stream_profile_list(gyro_profile_list, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            LOG_ERROR("ob_sensor_start failed", 0);
            return K4A_RESULT_FAILED;
        }
    }

    ob_delete_sensor_list(sensor_list, &ob_err);
    CHECK_OB_ERROR(&ob_err);
    ob_delete_stream_profile_list(accel_profile_list, &ob_err);
    CHECK_OB_ERROR(&ob_err);
    ob_delete_stream_profile_list(gyro_profile_list, &ob_err);
    CHECK_OB_ERROR(&ob_err);

    device_ctx->accel_sensor = accel_sensor;
    device_ctx->gyro_sensor = gyro_sensor;

    if (device_ctx->imusync != NULL)
    {
        return TRACE_CALL(imusync_start(device_ctx->imusync));
    }
    return K4A_RESULT_FAILED;
}

void k4a_device_stop_imu(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(VOID_VALUE, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    ob_error *ob_err = NULL;

    if (device_ctx->accel_sensor != NULL)
    {
        ob_sensor_stop(device_ctx->accel_sensor, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor(device_ctx->accel_sensor, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        device_ctx->accel_sensor = NULL;
    }

    if (device_ctx->gyro_sensor != NULL)
    {
        ob_sensor_stop(device_ctx->gyro_sensor, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        ob_delete_sensor(device_ctx->gyro_sensor, &ob_err);
        CHECK_OB_ERROR(&ob_err);
        device_ctx->gyro_sensor = NULL;
    }

    if (device_ctx->imusync != NULL)
    {
        imusync_stop(device_ctx->imusync);
    }
}

k4a_result_t k4a_capture_create(k4a_capture_t *capture_handle)
{
    k4a_result_t result = K4A_RESULT_FAILED;
    ob_error *ob_err = NULL;
    ob_frame *frame = ob_create_frameset(&ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    if (frame != NULL)
    {
        *capture_handle = (k4a_capture_t)frame;
        result = K4A_RESULT_SUCCEEDED;
    }
    return result;
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    if (capture_handle != NULL)
    {
        ob_error *ob_err = NULL;
        ob_frame *frame = (ob_frame *)capture_handle;
        ob_delete_frame(frame, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
    }
}

void k4a_capture_reference(k4a_capture_t capture_handle)
{
    if (capture_handle != NULL)
    {
        ob_error *ob_err = NULL;
        ob_frame *frame = (ob_frame *)capture_handle;
        ob_frame_add_ref(frame, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
    }
}

float k4a_capture_get_temperature_c(k4a_capture_t capture_handle)
{

    UNREFERENCED_VALUE(capture_handle);
    LOG_WARNING("k4a_capture_get_temperature_c unsupport api ", 0);
    return 0;
}

k4a_image_t k4a_capture_get_color_image(k4a_capture_t capture_handle)
{
    if (capture_handle == NULL)
    {
        LOG_WARNING("k4a_capture_get_color_image param invalid ", 0);
        return NULL;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame_set = (ob_frame *)capture_handle;
    ob_frame *color_frame = ob_frameset_color_frame(frame_set, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, NULL);

    if (color_frame == NULL)
    {
        LOG_INFO("color_frame is null ", 0);
        return NULL;
    }
    return (k4a_image_t)color_frame;
}

k4a_image_t k4a_capture_get_depth_image(k4a_capture_t capture_handle)
{
    if (capture_handle == NULL)
    {
        LOG_WARNING("k4a_capture_get_depth_image param invalid ", 0);
        return NULL;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame_set = (ob_frame *)capture_handle;
    ob_frame *depth_frame = ob_frameset_depth_frame(frame_set, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, NULL);
    if (depth_frame == NULL)
    {
        LOG_INFO("depth_frame is null ", 0);
        return NULL;
    }

    return (k4a_image_t)depth_frame;
}

k4a_image_t k4a_capture_get_ir_image(k4a_capture_t capture_handle)
{
    if (capture_handle == NULL)
    {
        LOG_WARNING("k4a_capture_get_ir_image param invalid ", 0);
        return NULL;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame_set = (ob_frame *)capture_handle;
    ob_frame *ir_frame = ob_frameset_ir_frame(frame_set, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, NULL);

    if (ir_frame == NULL)
    {
        LOG_INFO("ir_frame is null ", 0);
        return NULL;
    }

    return (k4a_image_t)ir_frame;
}

void k4a_capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    if (capture_handle == NULL || image_handle == NULL)
    {
        LOG_WARNING("k4a_capture_set_color_image param invalid ", 0);
        return;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame_set = (ob_frame *)capture_handle;
    ob_frame *color_frame = (ob_frame *)image_handle;
    ob_frameset_push_frame(frame_set, OB_FRAME_COLOR, color_frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
}

void k4a_capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    if (capture_handle == NULL || image_handle == NULL)
    {
        LOG_WARNING("k4a_capture_set_depth_image param invalid ", 0);
        return;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame_set = (ob_frame *)capture_handle;
    ob_frame *depth_frame = (ob_frame *)image_handle;
    ob_frameset_push_frame(frame_set, OB_FRAME_DEPTH, depth_frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
}

void k4a_capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image_handle)
{
    if (capture_handle == NULL || image_handle == NULL)
    {
        LOG_WARNING("k4a_capture_set_ir_image param invalid ", 0);
        return;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame_set = (ob_frame *)capture_handle;
    ob_frame *ir_frame = (ob_frame *)image_handle;
    ob_frameset_push_frame(frame_set, OB_FRAME_IR, ir_frame, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);
}

void k4a_capture_set_temperature_c(k4a_capture_t capture_handle, float temperature_c)
{
    UNREFERENCED_VALUE(capture_handle);
    UNREFERENCED_VALUE(temperature_c);
    LOG_WARNING("k4a_capture_set_temperature_c unsupport api ", 0);
}

k4a_result_t k4a_image_create(k4a_image_format_t format,
                              int width_pixels,
                              int height_pixels,
                              int stride_bytes,
                              k4a_image_t *image_handle)
{

    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    ob_error *ob_err = NULL;
    ob_frame *obFrame = NULL;
    switch (format)
    {
    case K4A_IMAGE_FORMAT_DEPTH16:
        obFrame = ob_create_frame(OB_FORMAT_Y16, width_pixels, height_pixels, stride_bytes, OB_FRAME_DEPTH, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_IR16:
        obFrame = ob_create_frame(OB_FORMAT_Y16, width_pixels, height_pixels, stride_bytes, OB_FRAME_IR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        obFrame = ob_create_frame(OB_FORMAT_BGRA, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        obFrame = ob_create_frame(OB_FORMAT_YUYV, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        obFrame = ob_create_frame(OB_FORMAT_NV12, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        obFrame = ob_create_frame(OB_FORMAT_MJPEG, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_CUSTOM8:
        obFrame = ob_create_frame(OB_FORMAT_Y8, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_CUSTOM16:
        obFrame = ob_create_frame(OB_FORMAT_Y16, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    case K4A_IMAGE_FORMAT_CUSTOM:
        obFrame =
            ob_create_frame(OB_FORMAT_UNKNOWN, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    default:
        obFrame = ob_create_frame(OB_FORMAT_BGRA, width_pixels, height_pixels, stride_bytes, OB_FRAME_COLOR, &ob_err);
        break;
    }
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    *image_handle = (k4a_image_t)obFrame;
    return result;
}

k4a_result_t k4a_image_create_from_buffer(k4a_image_format_t format,
                                          int width_pixels,
                                          int height_pixels,
                                          int stride_bytes,
                                          uint8_t *buffer,
                                          size_t buffer_size,
                                          k4a_memory_destroy_cb_t *buffer_release_cb,
                                          void *buffer_release_cb_context,
                                          k4a_image_t *image_handle)
{
    k4a_result_t result = K4A_RESULT_FAILED;
    stride_bytes = stride_bytes;

    ob_error *ob_err = NULL;
    ob_frame *obFrame = NULL;
    ob_format obFormat = OB_FORMAT_UNKNOWN;
    switch (format)
    {
    case K4A_IMAGE_FORMAT_DEPTH16:
        obFormat = OB_FORMAT_Y16;
        break;
    case K4A_IMAGE_FORMAT_IR16:
        obFormat = OB_FORMAT_Y16;
        break;
    case K4A_IMAGE_FORMAT_COLOR_BGRA32:
        obFormat = OB_FORMAT_BGRA;
        break;
    case K4A_IMAGE_FORMAT_COLOR_MJPG:
        obFormat = OB_FORMAT_MJPG;
        break;
    case K4A_IMAGE_FORMAT_COLOR_YUY2:
        obFormat = OB_FORMAT_YUYV;
        break;
    case K4A_IMAGE_FORMAT_COLOR_NV12:
        obFormat = OB_FORMAT_NV12;
        break;
    case K4A_IMAGE_FORMAT_CUSTOM8:
        obFormat = OB_FORMAT_Y8;
        break;
    case K4A_IMAGE_FORMAT_CUSTOM16:
        obFormat = OB_FORMAT_Y16;
        break;
    case K4A_IMAGE_FORMAT_CUSTOM:
        obFormat = OB_FORMAT_UNKNOWN;
        break;
    default:
        break;
    }

    if (obFormat == OB_FORMAT_UNKNOWN)
    {
        LOG_WARNING("k4a_image_create_from_buffer format error", 0);
        return result;
    }

    obFrame = ob_create_frame_from_buffer(obFormat,
                                          width_pixels,
                                          height_pixels,
                                          buffer,
                                          (uint32_t)buffer_size,
                                          buffer_release_cb,
                                          buffer_release_cb_context,
                                          &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    if (obFrame != NULL)
    {
        *image_handle = (k4a_image_t)obFrame;
        result = K4A_RESULT_SUCCEEDED;
    }

    return result;
}

uint8_t *k4a_image_get_buffer(k4a_image_t image_handle)
{
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_buffer param invalid ", 0);
        return NULL;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;
    uint8_t *data = (uint8_t *)ob_frame_data(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, NULL);
    return data;
}

size_t k4a_image_get_size(k4a_image_t image_handle)
{
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_size param invalid ", 0);
        return 0;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;
    uint32_t data_size = ob_frame_data_size(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    return (size_t)data_size;
}

k4a_image_format_t k4a_image_get_format(k4a_image_t image_handle)
{
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_format param invalid ", 0);
        return K4A_IMAGE_FORMAT_CUSTOM;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;
    ob_format frame_format = ob_frame_format(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, K4A_IMAGE_FORMAT_CUSTOM);
    k4a_image_format_t k4a_image_format = K4A_IMAGE_FORMAT_CUSTOM;
    ob_frame_type frame_type = ob_frame_get_type(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, K4A_IMAGE_FORMAT_CUSTOM);

    switch (frame_format)
    {
    case OB_FORMAT_YUY2:
    case OB_FORMAT_YUYV:
        if (frame_type == OB_FRAME_DEPTH)
        {
            k4a_image_format = K4A_IMAGE_FORMAT_DEPTH16;
        }
        else if (frame_type == OB_FRAME_IR)
        {
            k4a_image_format = K4A_IMAGE_FORMAT_IR16;
        }
        else
        {
            k4a_image_format = K4A_IMAGE_FORMAT_COLOR_YUY2;
        }

        break;
    case OB_FORMAT_Y16:
        if (frame_type == OB_FRAME_DEPTH)
        {
            k4a_image_format = K4A_IMAGE_FORMAT_DEPTH16;
        }
        else if (frame_type == OB_FRAME_IR)
        {
            k4a_image_format = K4A_IMAGE_FORMAT_IR16;
        }
        else
        {
            k4a_image_format = K4A_IMAGE_FORMAT_CUSTOM16;
        }
        break;
    case OB_FORMAT_Y8:
        k4a_image_format = K4A_IMAGE_FORMAT_CUSTOM8;
        break;
    case OB_FORMAT_MJPG:
        k4a_image_format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        break;
    case OB_FORMAT_BGRA:
        k4a_image_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        break;
    case OB_FORMAT_NV12:
        k4a_image_format = K4A_IMAGE_FORMAT_COLOR_NV12;
        break;
    case OB_FORMAT_UNKNOWN:
        k4a_image_format = K4A_IMAGE_FORMAT_CUSTOM;
    default:
        break;
    }
    return k4a_image_format;
}

int k4a_image_get_width_pixels(k4a_image_t image_handle)
{
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_width_pixels param invalid ", 0);
        return 0;
    }
    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;
    int width_pixels = ob_video_frame_width(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    return width_pixels;
}

int k4a_image_get_height_pixels(k4a_image_t image_handle)
{
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_height_pixels param invalid ", 0);
        return 0;
    }
    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;
    int height_pixels = ob_video_frame_height(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    return height_pixels;
}

int k4a_image_get_stride_bytes(k4a_image_t image_handle)
{
    // return image_get_stride_bytes(image_handle);
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_stride_bytes param invalid ", 0);
        return 0;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;
    ob_format frame_format = ob_frame_format(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    int width_pixels = ob_video_frame_width(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    int stride_bytes = 0;

    switch (frame_format)
    {
    case OB_FORMAT_YUYV:
    case OB_FORMAT_YUY2:
    case OB_FORMAT_UYVY:
    case OB_FORMAT_Y16:
        stride_bytes = width_pixels * 2;
        break;
    case OB_FORMAT_RGB888:
    case OB_FORMAT_BGR:
        stride_bytes = width_pixels * 3;
        break;
    case OB_FORMAT_Y8:
        stride_bytes = width_pixels;
        break;
    case OB_FORMAT_BGRA:
        stride_bytes = width_pixels * 4;
        break;
        break;
    default:
        break;
    }
    return stride_bytes;
}

// Deprecated
uint64_t k4a_image_get_timestamp_usec(k4a_image_t image_handle)
{
    return k4a_image_get_device_timestamp_usec(image_handle);
}

uint64_t k4a_image_get_device_timestamp_usec(k4a_image_t image_handle)
{
    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_device_timestamp_usec param invalid ", 0);
        return 0;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;

    uint64_t time_stamp = ob_frame_time_stamp_us(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);

    return time_stamp;
}

uint64_t k4a_image_get_system_timestamp_nsec(k4a_image_t image_handle)
{

    if (image_handle == NULL)
    {
        LOG_WARNING("k4a_image_get_system_timestamp_nsec param invalid ", 0);
        return 0;
    }

    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;

    uint64_t time_stamp = ob_frame_system_time_stamp(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);

    time_stamp = time_stamp * 1000000;

    return time_stamp;
}

uint64_t k4a_image_get_exposure_usec(k4a_image_t image_handle)
{
    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;

    void *metadata = ob_video_frame_metadata(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    if (metadata == NULL)
    {
        LOG_WARNING("get exposure failed", 0);
        return 0;
    }
    else
    {
        PKSCAMERA_METADATA_CAPTURESTATS pMeta_data = (PKSCAMERA_METADATA_CAPTURESTATS)metadata;
        return pMeta_data->ExposureTime;
    }
}

uint32_t k4a_image_get_white_balance(k4a_image_t image_handle)
{
    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;

    void *metadata = ob_video_frame_metadata(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    if (metadata == NULL)
    {
        LOG_WARNING("get white balance failed", 0);
        return 0;
    }
    else
    {
        PKSCAMERA_METADATA_CAPTURESTATS pMeta_data = (PKSCAMERA_METADATA_CAPTURESTATS)metadata;
        return pMeta_data->WhiteBalance;
    }
}

uint32_t k4a_image_get_iso_speed(k4a_image_t image_handle)
{
    ob_error *ob_err = NULL;
    ob_frame *frame = (ob_frame *)image_handle;

    void *metadata = ob_video_frame_metadata(frame, &ob_err);
    CHECK_OB_ERROR_RETURN_VALUE(&ob_err, 0);
    if (metadata == NULL)
    {
        LOG_WARNING("get image iso_speed failed", 0);
        return 0;
    }
    else
    {
        PKSCAMERA_METADATA_CAPTURESTATS pMeta_data = (PKSCAMERA_METADATA_CAPTURESTATS)metadata;
        return pMeta_data->IsoSpeed;
    }
}

void k4a_image_set_device_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec)
{
    if (image_handle == NULL)
    {
        LOG_ERROR("k4a_image_set_device_timestamp_usec param invalid", 0);
        return;
    }

    ob_frame *frame = (ob_frame *)image_handle;
    ob_error *ob_err = NULL;

    ob_frame_set_device_time_stamp_us(frame, timestamp_usec, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);

    if (ob_err != NULL)
    {
        LOG_ERROR("k4a_image_set_device_timestamp_usec ob_err != NULL", 0);
    }
}

// Deprecated
void k4a_image_set_timestamp_usec(k4a_image_t image_handle, uint64_t timestamp_usec)
{
    k4a_image_set_device_timestamp_usec(image_handle, timestamp_usec);
}

void k4a_image_set_system_timestamp_nsec(k4a_image_t image_handle, uint64_t timestamp_nsec)
{
    if (image_handle == NULL)
    {
        LOG_ERROR("k4a_image_set_system_timestamp_nsec param invalid", 0);
        return;
    }

    ob_frame *frame = (ob_frame *)image_handle;
    ob_error *ob_err = NULL;
    uint64_t timestamp_mill = timestamp_nsec / 1000000;

    ob_frame_set_system_time_stamp(frame, timestamp_mill, &ob_err);
    CHECK_OB_ERROR_RETURN(&ob_err);

    if (ob_err != NULL)
    {
        LOG_ERROR("ob_frame_set_system_time_stamp ob_err != NULL", 0);
    }
}

// Deprecated
void k4a_image_set_exposure_time_usec(k4a_image_t image_handle, uint64_t exposure_usec)
{
    UNREFERENCED_VALUE(image_handle);
    UNREFERENCED_VALUE(exposure_usec);
    LOG_WARNING("k4a_image_set_exposure_time_usec unsupport api ", 0);
}

void k4a_image_set_exposure_usec(k4a_image_t image_handle, uint64_t exposure_usec)
{
    UNREFERENCED_VALUE(image_handle);
    UNREFERENCED_VALUE(exposure_usec);
    LOG_WARNING("k4a_image_set_exposure_usec unsupport api ", 0);
}

void k4a_image_set_white_balance(k4a_image_t image_handle, uint32_t white_balance)
{
    UNREFERENCED_VALUE(image_handle);
    UNREFERENCED_VALUE(white_balance);
    LOG_WARNING("k4a_image_set_white_balance unsupport api ", 0);
}

void k4a_image_set_iso_speed(k4a_image_t image_handle, uint32_t iso_speed)
{
    UNREFERENCED_VALUE(image_handle);
    UNREFERENCED_VALUE(iso_speed);
    LOG_WARNING("k4a_image_set_iso_speed unsupport api ", 0);
}

void k4a_image_reference(k4a_image_t image_handle)
{
    if (image_handle != NULL)
    {
        ob_error *ob_err = NULL;
        ob_frame *frame = (ob_frame *)image_handle;
        ob_frame_add_ref(frame, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
    }
}

void k4a_image_release(k4a_image_t image_handle)
{
    if (image_handle != NULL)
    {
        ob_error *ob_err = NULL;
        ob_frame *frame = (ob_frame *)image_handle;
        ob_delete_frame(frame, &ob_err);
        CHECK_OB_ERROR_RETURN(&ob_err);
    }
}
/*
static const char *k4a_depth_mode_to_string(k4a_depth_mode_t depth_mode)
{
    switch (depth_mode)
    {
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_OFF);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_NFOV_2X2BINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_NFOV_UNBINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_WFOV_2X2BINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_WFOV_UNBINNED);
        K4A_DEPTH_MODE_TO_STRING_CASE(K4A_DEPTH_MODE_PASSIVE_IR);
    }
    return "Unexpected k4a_depth_mode_t value.";
}

static const char *k4a_color_resolution_to_string(k4a_color_resolution_t resolution)
{
    switch (resolution)
    {
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_OFF);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_720P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_1080P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_480P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_960P);
        K4A_COLOR_RESOLUTION_TO_STRING_CASE(K4A_COLOR_RESOLUTION_1024X768);
    }
    return "Unexpected k4a_color_resolution_t value.";
}

static const char *k4a_image_format_to_string(k4a_image_format_t image_format)
{
    switch (image_format)
    {
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_MJPG);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_NV12);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_YUY2);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_COLOR_BGRA32);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_DEPTH16);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_IR16);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_CUSTOM8);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_CUSTOM16);
        K4A_IMAGE_FORMAT_TO_STRING_CASE(K4A_IMAGE_FORMAT_CUSTOM);
    }
    return "Unexpected k4a_image_format_t value.";
}

static const char *k4a_fps_to_string(k4a_fps_t fps)
{
    switch (fps)
    {
        K4A_FPS_TO_STRING_CASE(K4A_FRAMES_PER_SECOND_5);
        K4A_FPS_TO_STRING_CASE(K4A_FRAMES_PER_SECOND_15);
        K4A_FPS_TO_STRING_CASE(K4A_FRAMES_PER_SECOND_30);
    }
    return "Unexpected k4a_fps_t value.";
}

*/

static k4a_result_t validate_configuration(k4a_device_context_t *device_ctx, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, device_ctx == NULL);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    bool depth_enabled = false;
    bool color_enabled = false;

    if (config->color_format != K4A_IMAGE_FORMAT_COLOR_MJPG && config->color_format != K4A_IMAGE_FORMAT_COLOR_YUY2 &&
        config->color_format != K4A_IMAGE_FORMAT_COLOR_NV12 && config->color_format != K4A_IMAGE_FORMAT_COLOR_BGRA32)
    {
        result = K4A_RESULT_FAILED;
        LOG_ERROR("The configured color_format is not a valid k4a_color_format_t value.", 0);
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->color_resolution < K4A_COLOR_RESOLUTION_OFF)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured color_resolution is not a valid k4a_color_resolution_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->depth_mode < K4A_DEPTH_MODE_OFF)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured depth_mode is not a valid k4a_depth_mode_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->camera_fps != K4A_FRAMES_PER_SECOND_5 && config->camera_fps != K4A_FRAMES_PER_SECOND_15 &&
            config->camera_fps != K4A_FRAMES_PER_SECOND_25 && config->camera_fps != K4A_FRAMES_PER_SECOND_30)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured camera_fps is not a valid k4a_fps_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->wired_sync_mode < K4A_WIRED_SYNC_MODE_STANDALONE ||
            config->wired_sync_mode > K4A_WIRED_SYNC_MODE_SUBORDINATE)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("The configured wired_sync_mode is not a valid k4a_wired_sync_mode_t value.", 0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE &&
            config->subordinate_delay_off_master_usec != 0)
        {
            uint32_t fps_in_usec = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config->camera_fps));
            if (config->subordinate_delay_off_master_usec > fps_in_usec)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("The configured subordinate device delay from the master device cannot exceed one frame "
                          "interval of %d. User requested %d",
                          fps_in_usec,
                          config->subordinate_delay_off_master_usec);
            }
        }

        if (config->wired_sync_mode != K4A_WIRED_SYNC_MODE_SUBORDINATE &&
            config->subordinate_delay_off_master_usec != 0)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("When wired_sync_mode is K4A_WIRED_SYNC_MODE_STANDALONE or K4A_WIRED_SYNC_MODE_MASTER, the "
                      "subordinate_delay_off_master_usec must be 0.",
                      0);
        }
    }

    if (K4A_SUCCEEDED(result))
    {
        if (config->depth_mode != K4A_DEPTH_MODE_OFF)
        {
            depth_enabled = true;
        }

        if (config->color_resolution != K4A_COLOR_RESOLUTION_OFF)
        {
            color_enabled = true;
        }

        if (depth_enabled && color_enabled)
        {
            int64_t fps = HZ_TO_PERIOD_US(k4a_convert_fps_to_uint(config->camera_fps));
            if (config->depth_delay_off_color_usec < -fps || config->depth_delay_off_color_usec > fps)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("The configured depth_delay_off_color_usec must be within +/- one frame interval of %d. User "
                          "requested %d",
                          fps,
                          config->depth_delay_off_color_usec);
            }
        }
        else if (!depth_enabled && !color_enabled)
        {
            result = K4A_RESULT_FAILED;
            LOG_ERROR("Neither depth camera nor color camera are enabled in the configuration, at least one needs to "
                      "be enabled.",
                      0);
        }
        else
        {
            if (config->depth_delay_off_color_usec != 0)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR("If depth_delay_off_color_usec is not 0, both depth camera and color camera must be enabled.",
                          0);
            }

            if (config->synchronized_images_only)
            {
                result = K4A_RESULT_FAILED;
                LOG_ERROR(
                    "To enable synchronized_images_only, both depth camera and color camera must also be enabled.", 0);
            }
        }
    }

    return result;
}

k4a_result_t k4a_device_start_cameras(k4a_device_t device_handle, const k4a_device_configuration_t *config)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    ob_error *ob_err = NULL;
    OB_DEVICE_SYNC_CONFIG ob_sync_config;
    memset(&ob_sync_config, 0, sizeof(OB_DEVICE_SYNC_CONFIG));
    uint32_t len;
    ob_device_get_structured_data(device_ctx->device,
                                    OB_STRUCT_MULTI_DEVICE_SYNC_CONFIG,
                                    &ob_sync_config,
                                    &len,
                                    &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    if(current_device_clock_sync_mode == K4A_TIEMSTAMP_UNKNOWN){
        auto sync_config = ob_device_get_sync_config(device_ctx->device, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if(sync_config.syncMode == OB_SYNC_MODE_PRIMARY_MCU_TRIGGER) {
            switch_device_clock_sync_mode(device_handle, 0, K4A_TIMESTAMP_RESET);
            current_device_clock_sync_mode = K4A_TIMESTAMP_RESET;
        }
    }

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    if (config == NULL)
    {
        LOG_ERROR("param invalid,[%s]", __func__);
        return K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result))
    {
        LOG_INFO("Starting camera's with the following config.", 0);
        LOG_INFO("    color_format:%d", config->color_format);
        LOG_INFO("    color_resolution:%d", config->color_resolution);
        LOG_INFO("    depth_mode:%d", config->depth_mode);
        LOG_INFO("    camera_fps:%d", config->camera_fps);
        LOG_INFO("    synchronized_images_only:%d", config->synchronized_images_only);
        LOG_INFO("    depth_delay_off_color_usec:%d", config->depth_delay_off_color_usec);
        LOG_INFO("    wired_sync_mode:%d", config->wired_sync_mode);
        LOG_INFO("    subordinate_delay_off_master_usec:%d", config->subordinate_delay_off_master_usec);
        LOG_INFO("    disable_streaming_indicator:%d", config->disable_streaming_indicator);
        result = TRACE_CALL(validate_configuration(device_ctx, config));
    }

    if (K4A_FAILED(result))
    {
        return K4A_RESULT_FAILED;
    }

    if (config->depth_delay_off_color_usec > MAX_DELAY_TIME || config->depth_delay_off_color_usec < MIN_DELAY_TIME)
    {
        LOG_ERROR("depth_delay_off_color_usec out of range", 0);
        return K4A_RESULT_FAILED;
    }

    if (config->subordinate_delay_off_master_usec > MAX_DELAY_TIME)
    {
        LOG_ERROR("subordinate_delay_off_master_usec out of range", 0);
        return K4A_RESULT_FAILED;
    }

    ob_device_info *dev_info = ob_device_get_device_info(device_ctx->device, &ob_err);
    int pid = ob_device_info_pid(dev_info, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    ob_delete_device_info(dev_info, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    if (pid == ORBBEC_MEGA_PID || pid == ORBBEC_BOLT_PID)
    {
        ob_device_set_bool_property(device_ctx->device, OB_PROP_INDICATOR_LIGHT_BOOL, config->disable_streaming_indicator, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        uint32_t base_delay = 0;
        if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_MASTER)
        {
            ob_sync_config.syncMode = OB_SYNC_MODE_PRIMARY_MCU_TRIGGER;
        }
        else if (config->wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE)
        {
            ob_sync_config.syncMode = OB_SYNC_MODE_SECONDARY;
            base_delay = config->subordinate_delay_off_master_usec;
        }
        else
        {
            ob_sync_config.syncMode = OB_SYNC_MODE_STANDALONE;
        }

        if (config->depth_delay_off_color_usec > 0)
        {
            ob_sync_config.rgbTriggerSignalInDelay = base_delay > 65535 ? 65535 : (uint16_t)base_delay;
            uint32_t depth_delay = ob_sync_config.rgbTriggerSignalInDelay + config->depth_delay_off_color_usec;
            ob_sync_config.irTriggerSignalInDelay = depth_delay > 65535 ? 65535 : (uint16_t)depth_delay;
        }
        else
        {
            ob_sync_config.irTriggerSignalInDelay = base_delay > 65535 ? 65535 : (uint16_t)base_delay;
            uint32_t color_delay = ob_sync_config.irTriggerSignalInDelay - config->depth_delay_off_color_usec;
            ob_sync_config.rgbTriggerSignalInDelay = color_delay > 65535 ? 65535 : (uint16_t)color_delay;
        }

        ob_device_set_structured_data(device_ctx->device,
                                      OB_STRUCT_MULTI_DEVICE_SYNC_CONFIG,
                                      &ob_sync_config,
                                      sizeof(ob_sync_config),
                                      &ob_err);

        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    }

    if (pid != ORBBEC_MEGA_PID && pid != ORBBEC_BOLT_PID && config->depth_mode == K4A_DEPTH_MODE_PASSIVE_IR)
    {
        LOG_ERROR("not support passive ir", 0);
        return K4A_RESULT_FAILED;
    }

    if (!(config->color_format == K4A_IMAGE_FORMAT_COLOR_MJPG ||
          config->color_format == K4A_IMAGE_FORMAT_COLOR_BGRA32 ||
          config->color_format == K4A_IMAGE_FORMAT_COLOR_YUY2 || config->color_format == K4A_IMAGE_FORMAT_COLOR_NV12))
    {
        LOG_ERROR("color format unsupport,[%s]", __func__);
        return K4A_RESULT_FAILED;
    }

    uint32_t k4a_depth_width = 0;
    uint32_t k4a_depth_height = 0;
    switch (config->depth_mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED:
        k4a_depth_width = 320;
        k4a_depth_height = 288;
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED:
        k4a_depth_width = 640;
        k4a_depth_height = 576;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED:
        k4a_depth_width = 512;
        k4a_depth_height = 512;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED:
        k4a_depth_width = 1024;
        k4a_depth_height = 1024;
        break;
    case K4A_DEPTH_MODE_PASSIVE_IR:
        k4a_depth_width = 1024;
        k4a_depth_height = 1024;
        break;
    default:
        break;
    }

    uint32_t k4a_fps = 30;
    switch (config->camera_fps)
    {
    case K4A_FRAMES_PER_SECOND_30:
        k4a_fps = 30;
        break;
    case K4A_FRAMES_PER_SECOND_25:
        k4a_fps = 25;
        break;
    case K4A_FRAMES_PER_SECOND_15:
        k4a_fps = 15;
        break;
    case K4A_FRAMES_PER_SECOND_5:
        k4a_fps = 5;
        break;
    default:
        break;
    }

    uint32_t k4a_color_width = 0;
    uint32_t k4a_color_height = 0;
    switch (config->color_resolution)
    {
    case K4A_COLOR_RESOLUTION_1080P:
        k4a_color_width = 1920;
        k4a_color_height = 1080;
        break;
    case K4A_COLOR_RESOLUTION_720P:
        k4a_color_width = 1280;
        k4a_color_height = 720;
        break;
    case K4A_COLOR_RESOLUTION_1440P:
        k4a_color_width = 2560;
        k4a_color_height = 1440;
        break;
    case K4A_COLOR_RESOLUTION_1536P:
        k4a_color_width = 2048;
        k4a_color_height = 1536;
        break;
    case K4A_COLOR_RESOLUTION_2160P:
        k4a_color_width = 3840;
        k4a_color_height = 2160;
        break;
    case K4A_COLOR_RESOLUTION_3072P:
        k4a_color_width = 4096;
        k4a_color_height = 3072;
        break;
    default:
        break;
    }

    device_ctx->pipe = ob_create_pipeline_with_device((ob_device *)device_ctx->device, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    ob_config *obConfig = ob_create_config(&ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    ob_stream_profile *depth_profile = NULL;
    ob_stream_profile_list *depth_profiles = NULL;

    ob_stream_profile *ir_profile = NULL;
    ob_stream_profile_list *ir_profiles = NULL;

    ob_stream_profile *color_profile = NULL;
    ob_stream_profile_list *color_profiles = NULL;

    if (config->depth_mode != K4A_DEPTH_MODE_OFF)
    {

        if (config->depth_mode != K4A_DEPTH_MODE_PASSIVE_IR)
        {

            if (pid == ORBBEC_MEGA_PID || pid == ORBBEC_BOLT_PID)
            {
                ob_device_set_int_property(device_ctx->device, OB_PROP_SWITCH_IR_MODE_INT, ACTIVE_IR, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
            depth_profiles = ob_pipeline_get_stream_profile_list(device_ctx->pipe, OB_SENSOR_DEPTH, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            depth_profile = ob_stream_profile_list_get_video_stream_profile(depth_profiles,
                                                                            k4a_depth_width,
                                                                            k4a_depth_height,
                                                                            OB_FORMAT_Y16, // OB_FORMAT_YUYV
                                                                                           // OB_FORMAT_Y16
                                                                            k4a_fps,
                                                                            &ob_err);
            CHECK_OB_ERROR(&ob_err);

            if (!depth_profile)
            {

                ob_delete_pipeline(device_ctx->pipe, &ob_err);
                device_ctx->pipe = NULL;
                CHECK_OB_ERROR(&ob_err);

                ob_delete_config(obConfig, &ob_err);
                obConfig = NULL;
                CHECK_OB_ERROR(&ob_err);

                ob_delete_stream_profile_list(depth_profiles, &ob_err);
                depth_profiles = NULL;
                CHECK_OB_ERROR(&ob_err);
                return K4A_RESULT_FAILED;
            }

            ob_config_enable_stream(obConfig, depth_profile, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {

            if (pid == ORBBEC_MEGA_PID || pid == ORBBEC_BOLT_PID)
            {
                ob_device_set_int_property(device_ctx->device, OB_PROP_SWITCH_IR_MODE_INT, PASSIVE_IR, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
        }

        ir_profiles = ob_pipeline_get_stream_profile_list(device_ctx->pipe, OB_SENSOR_IR, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        ir_profile = ob_stream_profile_list_get_video_stream_profile(ir_profiles,
                                                                     k4a_depth_width,
                                                                     k4a_depth_height,
                                                                     OB_FORMAT_Y16,
                                                                     k4a_fps,
                                                                     &ob_err);
        CHECK_OB_ERROR(&ob_err);

        if (!ir_profile)
        {

            ob_delete_pipeline(device_ctx->pipe, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            device_ctx->pipe = NULL;
            ob_delete_config(obConfig, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            if (config->depth_mode != K4A_DEPTH_MODE_PASSIVE_IR)
            {
                ob_delete_stream_profile_list(depth_profiles, &ob_err);
                depth_profiles = NULL;
                CHECK_OB_ERROR(&ob_err);
            }

            ob_delete_stream_profile_list(ir_profiles, &ob_err);
            ir_profiles = NULL;
            CHECK_OB_ERROR(&ob_err);

            return K4A_RESULT_FAILED;
        }
        ob_config_enable_stream(obConfig, ir_profile, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    }

    if (config->color_resolution != K4A_COLOR_RESOLUTION_OFF)
    {

        color_profiles = ob_pipeline_get_stream_profile_list(device_ctx->pipe, OB_SENSOR_COLOR, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        ob_format color_format = OB_FORMAT_UNKNOWN;
        switch (config->color_format)
        {
        case K4A_IMAGE_FORMAT_COLOR_MJPG:
            color_format = OB_FORMAT_MJPG;
            break;
        case K4A_IMAGE_FORMAT_COLOR_BGRA32:
            color_format = OB_FORMAT_BGRA;
            break;
        case K4A_IMAGE_FORMAT_COLOR_YUY2:
            color_format = OB_FORMAT_YUYV;
            break;
        case K4A_IMAGE_FORMAT_COLOR_NV12:
            color_format = OB_FORMAT_NV12;
            break;
        default:
            break;
        }

        if (color_format == OB_FORMAT_UNKNOWN)
        {
            LOG_ERROR("color format unsupport,[%s]", __func__);
            return K4A_RESULT_FAILED;
        }

        color_profile = ob_stream_profile_list_get_video_stream_profile(color_profiles,
                                                                        k4a_color_width,
                                                                        k4a_color_height,
                                                                        color_format, // OB_FORMAT_MJPG
                                                                        k4a_fps,
                                                                        &ob_err);
        CHECK_OB_ERROR(&ob_err);

        if (!color_profile)
        {

            ob_delete_pipeline(device_ctx->pipe, &ob_err);
            device_ctx->pipe= NULL;
            CHECK_OB_ERROR(&ob_err);

            ob_delete_config(obConfig, &ob_err);
            obConfig = NULL;
            CHECK_OB_ERROR(&ob_err);

            ob_delete_stream_profile_list(color_profiles, &ob_err);
            color_profiles = NULL;
            CHECK_OB_ERROR(&ob_err);

            if (config->depth_mode != K4A_DEPTH_MODE_OFF)
            {
                if (config->depth_mode != K4A_DEPTH_MODE_PASSIVE_IR)
                {
                    ob_delete_stream_profile_list(depth_profiles, &ob_err);
                    depth_profiles = NULL;
                    CHECK_OB_ERROR(&ob_err);
                }

                ob_delete_stream_profile_list(ir_profiles, &ob_err);
                ir_profiles = NULL;
                CHECK_OB_ERROR(&ob_err);
            }

            return K4A_RESULT_FAILED;
        }

        ob_config_enable_stream(obConfig, color_profile, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    }

    ob_pipeline_enable_frame_sync(device_ctx->pipe, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    if (config->synchronized_images_only && config->depth_mode != K4A_DEPTH_MODE_PASSIVE_IR)
    {
        ob_config_set_frame_aggregate_output_mode(obConfig, OB_FRAME_AGGREGATE_OUTPUT_FULL_FRAME_REQUIRE, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    }

    frame_queue_enable(device_ctx->frameset_queue);

    ob_pipeline_start_with_callback(device_ctx->pipe, obConfig, ob_frame_set_ready, device_handle, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    device_ctx->is_streaming = true;

    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    ob_delete_config(obConfig, &ob_err);
    if (config->depth_mode != K4A_DEPTH_MODE_OFF)
    {
        ob_delete_stream_profile_list(depth_profiles, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        depth_profiles = NULL;

        ob_delete_stream_profile_list(ir_profiles, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        ir_profiles = NULL;
    }

    if (config->color_resolution != K4A_COLOR_RESOLUTION_OFF)
    {
        ob_delete_stream_profile_list(color_profiles, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        color_profiles = NULL;
    }
    if (color_profiles != NULL)
    {
        ob_delete_stream_profile_list(color_profiles, &ob_err);
    }
    if (color_profile != NULL)
    {
        ob_delete_stream_profile(color_profile, &ob_err);
    }
    if (depth_profiles != NULL)
    {
        ob_delete_stream_profile_list(depth_profiles, &ob_err);
    }
    if (depth_profile != NULL)
    {
        ob_delete_stream_profile(depth_profile, &ob_err);
    }
    if (ir_profiles != NULL)
    {
        ob_delete_stream_profile_list(ir_profiles, &ob_err);
    }
    if (ir_profile != NULL)
    {
        ob_delete_stream_profile(ir_profile, &ob_err);
    }
    return result;
}

void k4a_device_stop_cameras(k4a_device_t device_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(VOID_VALUE, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    if (device_ctx != NULL)
    {
        ob_error *ob_err = NULL;

        if (device_ctx->pipe != NULL)
        {
            ob_pipeline_stop(device_ctx->pipe, &ob_err);
            CHECK_OB_ERROR(&ob_err);

            ob_delete_pipeline(device_ctx->pipe, &ob_err);
            CHECK_OB_ERROR(&ob_err);
            device_ctx->pipe = NULL;
        }

        frame_queue_disable(device_ctx->frameset_queue);
    }
}

k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
                                             char *serial_number,
                                             size_t *serial_number_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_device_t, device_handle);
    // dose not need to check and init device context since serial number is got when enumerating devices
    // CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_BUFFER_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    if (serial_number_size == NULL)
    {
        LOG_WARNING("serial_number_size is NULL ", 0);
        return K4A_BUFFER_RESULT_FAILED;
    }

    size_t caller_buffer_size = 0;
    caller_buffer_size = *serial_number_size;

    size_t snLen = strlen(device_ctx->serial_number);
    *serial_number_size = snLen + 1;

    if (caller_buffer_size <= (size_t)snLen || serial_number == NULL)
    {
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }

    memcpy(serial_number, device_ctx->serial_number, snLen);
    serial_number[snLen] = '\0';

    return K4A_BUFFER_RESULT_SUCCEEDED;
}

k4a_result_t version_convert(const char *orbbec_version, k4a_version_t *k4a_version)
{
    if (orbbec_version == NULL || k4a_version == NULL)
    {
        LOG_WARNING("param invalid ", 0);
        return K4A_RESULT_FAILED;
    }

    int orbbec_version_len = (int)strlen(orbbec_version);
    if (orbbec_version_len >= MAX_FIREWARE_VERSION_LEN)
    {
        LOG_WARNING("orbbec_version_len overflow ", 0);
        return K4A_RESULT_FAILED;
    }

    char split_version[MAX_FIREWARE_VERSION_LEN] = { 0 };
    int count = 0;

    for (int i = 0; i < orbbec_version_len; i++)
    {
        if (orbbec_version[i] >= '0' && orbbec_version[i] <= '9')
        {
            count = i;
            break;
        }
    }

    int split_version_len = orbbec_version_len - count;
    memcpy(split_version, orbbec_version + count, (uint16_t)split_version_len);

    count = 0;
    for (int i = 0; i < split_version_len; i++)
    {
        if (split_version[i] == '.')
        {
            count++;
        }
    }

    if (count != 2)
    {
        return K4A_RESULT_FAILED;
    }

    // split spot
    int spot_pos[2] = { 0 };
    count = 0;
    for (int i = 0; i < split_version_len; i++)
    {
        if (split_version[i] == '.')
        {

            spot_pos[count] = i + 1;
            count++;
        }
    }

    char major[MAX_FIREWARE_VERSION_LEN] = { 0 };
    char minor[MAX_FIREWARE_VERSION_LEN] = { 0 };
    char iteration[MAX_FIREWARE_VERSION_LEN] = { 0 };
    memcpy(major, split_version, spot_pos[0] - 1);
    memcpy(minor, split_version + spot_pos[0], spot_pos[1] - spot_pos[0] - 1);
    memcpy(iteration, split_version + spot_pos[1], orbbec_version_len - spot_pos[1]);

    k4a_version->major = atoi(major);
    k4a_version->minor = atoi(minor);
    k4a_version->iteration = atoi(iteration);

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t k4a_device_get_version(k4a_device_t device_handle, k4a_hardware_version_t *version)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    if (version == NULL)
    {
        LOG_WARNING("param invalid ", 0);
        return K4A_RESULT_FAILED;
    }

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    ob_error *ob_err = NULL;
    ob_device_info *dev_info = ob_device_get_device_info(device_ctx->device, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    const char *firmware_version = ob_device_info_firmware_version(dev_info, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    k4a_version_t k4a_version = { 0 };

    result = version_convert(firmware_version, &k4a_version);
    if (result != K4A_RESULT_SUCCEEDED)
    {
        LOG_WARNING("version_convert failed ", 0);
        return result;
    }

    memset(&version->rgb, 0, sizeof(k4a_version));
    memcpy(&version->depth, &k4a_version, sizeof(k4a_version));
    memset(&version->depth_sensor, 0, sizeof(k4a_version));
    memset(&version->audio, 0, sizeof(k4a_version));

    version->firmware_build = K4A_FIRMWARE_BUILD_RELEASE;
    version->firmware_signature = K4A_FIRMWARE_SIGNATURE_UNSIGNED;

    ob_delete_device_info(dev_info, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t k4a_device_get_sync_jack(k4a_device_t device_handle,
                                      bool *sync_in_jack_connected,
                                      bool *sync_out_jack_connected)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    UNREFERENCED_VALUE(device_handle);
    UNREFERENCED_VALUE(sync_in_jack_connected);
    UNREFERENCED_VALUE(sync_out_jack_connected);
    *sync_in_jack_connected = false;
    *sync_out_jack_connected = false;
    LOG_WARNING("The Orbbec device does not support retrieving the jack connection status, so this function will "
                "always return false(disconnected).",
                0);
    return result;
}

k4a_result_t k4a_device_get_color_control_capabilities(k4a_device_t device_handle,
                                                       k4a_color_control_command_t command,
                                                       bool *supports_auto,
                                                       int32_t *min_value,
                                                       int32_t *max_value,
                                                       int32_t *step_value,
                                                       int32_t *default_value,
                                                       k4a_color_control_mode_t *default_mode)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    ob_device *obDevice = device_ctx->device;
    k4a_result_t result = K4A_RESULT_FAILED;
    ob_error *ob_err = NULL;

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_EXPOSURE_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = true;
            const int unit = 100;
            *min_value = colorParamRange.min * unit;
            *max_value = colorParamRange.max * unit;
            *step_value = colorParamRange.step * unit;
            *default_value = colorParamRange.def * unit; // unit =100us
            *default_mode = K4A_COLOR_CONTROL_MODE_AUTO;

            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color exposure param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY: {
        LOG_WARNING("unsupport control command ", 0);
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_BRIGHTNESS_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        if (ob_err == NULL)
        {
            *supports_auto = false;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color brightness param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_CONTRAST_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = false;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color contrast param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_SATURATION_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = false;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color saturation param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_SHARPNESS_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = false;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color sharpness param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_WHITEBALANCE: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_WHITE_BALANCE_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = true;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_AUTO;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color white balance param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION: {
        LOG_WARNING("unsupport api ", 0);
        result = K4A_RESULT_FAILED;
    }
    break;
    case K4A_COLOR_CONTROL_GAIN: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_GAIN_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = false;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color gain param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (ob_err == NULL)
        {
            *supports_auto = false;
            *min_value = colorParamRange.min;
            *max_value = colorParamRange.max;
            *step_value = colorParamRange.step;
            *default_value = colorParamRange.def;
            *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            LOG_WARNING("get color gain param range failed ", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_HDR:{
        ob_bool_property_range colorParamRange = ob_device_get_bool_property_range(obDevice,
                                                                                 OB_PROP_COLOR_HDR_BOOL,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        *supports_auto = false;
        *min_value = colorParamRange.min;
        *max_value = colorParamRange.max;
        *step_value = colorParamRange.step;
        *default_value = colorParamRange.def;
        *default_mode = K4A_COLOR_CONTROL_MODE_MANUAL;
        result = K4A_RESULT_SUCCEEDED;
    }
    break;
    default:
        LOG_WARNING("unsupport control command ", 0);
        break;
    }

    return result;
}

k4a_result_t k4a_device_get_color_control(k4a_device_t device_handle,
                                          k4a_color_control_command_t command,
                                          k4a_color_control_mode_t *mode,
                                          int32_t *value)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    UNREFERENCED_VALUE(mode);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    ob_device *obDevice = device_ctx->device;
    ob_error *ob_err = NULL;

    if (command < K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE || command > K4A_COLOR_CONTROL_HDR)
    {
        LOG_WARNING("command out of range ", 0);

        return K4A_RESULT_FAILED;
    }

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE: {
        // Convert micro-second unit to KSProperty exposure time value
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_EXPOSURE_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue * 100;
        bool enableAuto = false;
        enableAuto = ob_device_get_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (enableAuto)
        {
            *mode = K4A_COLOR_CONTROL_MODE_AUTO;
        }
        else
        {
            *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_BRIGHTNESS_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_CONTRAST_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_SATURATION_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_SHARPNESS_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_WHITEBALANCE: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_WHITE_BALANCE_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
        bool enableAuto = false;
        enableAuto = ob_device_get_int_property(obDevice, OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (enableAuto)
        {
            *mode = K4A_COLOR_CONTROL_MODE_AUTO;
        }
        else
        {
            *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
        }
    }
    break;
    case K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION: {
        bool supported = ob_device_is_property_supported(obDevice,
                                                         OB_PROP_COLOR_BACKLIGHT_COMPENSATION_INT,
                                                         OB_PERMISSION_READ,
                                                         &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (supported)
        {
            int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_BACKLIGHT_COMPENSATION_INT, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            *value = obValue;
        }
    }
    break;
    case K4A_COLOR_CONTROL_GAIN: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_GAIN_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY: {
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY: {
        // LOG_WARNING("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY is deprecated and does nothing.");
        int32_t obValue = ob_device_get_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_PRIORITY_INT, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        *value = obValue;
    }
    break;
    case K4A_COLOR_CONTROL_HDR:{
        bool supported = ob_device_is_property_supported(obDevice,
                                                         OB_PROP_COLOR_HDR_BOOL,
                                                         OB_PERMISSION_READ,
                                                         &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if(supported){
            bool obValue = ob_device_get_bool_property(obDevice, OB_PROP_COLOR_HDR_BOOL, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            *value = obValue;
        }
    }
    break;
    default:
        LOG_ERROR("Failed, unknown command %u", command);
        return K4A_RESULT_FAILED;
    }

    return result;
}

k4a_result_t k4a_device_set_color_control(k4a_device_t device_handle,
                                          k4a_color_control_command_t command,
                                          k4a_color_control_mode_t mode,
                                          int32_t value)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    k4a_result_t result = K4A_RESULT_SUCCEEDED;
    ob_device *obDevice = device_ctx->device;

    ob_error *ob_err = NULL;

    if (command < K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE || command > K4A_COLOR_CONTROL_HDR)
    {
        LOG_WARNING("command out of range ", 0);
        return K4A_RESULT_FAILED;
    }

    if (mode != K4A_COLOR_CONTROL_MODE_AUTO && mode != K4A_COLOR_CONTROL_MODE_MANUAL)
    {
        LOG_WARNING("mode param out of range ", 0);
        return K4A_RESULT_FAILED;
    }

    switch (command)
    {
    case K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE: {
        // Convert micro-second unit to KSProperty exposure time value
        bool enableAutoExp = false;
        enableAutoExp = ob_device_get_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        if (mode == K4A_COLOR_CONTROL_MODE_AUTO)
        {
            if (!enableAutoExp)
            {
                enableAutoExp = true;
                ob_device_set_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, enableAutoExp, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {

            if (enableAutoExp)
            {
                enableAutoExp = false;
                ob_device_set_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, enableAutoExp, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }

            ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                     OB_PROP_COLOR_EXPOSURE_INT,
                                                                                     &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            int exposureValue = value / 100;
            int min = colorParamRange.min;
            int max = colorParamRange.max;
            if (min <= exposureValue && exposureValue <= max)
            {
                ob_device_set_int_property(obDevice, OB_PROP_COLOR_EXPOSURE_INT, exposureValue, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
            else
            {
                LOG_WARNING("k4a color control set exposure failed [exposure out of range(min=%d,max=%d,value=%d)]",
                            min,
                            max,
                            exposureValue);
                result = K4A_RESULT_FAILED;
            }
        }
    }
    break;

    case K4A_COLOR_CONTROL_GAIN: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_GAIN_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        int min = colorParamRange.min;
        int max = colorParamRange.max;
        if (min <= value && value <= max)
        {
            int remainder = value % colorParamRange.step;
            if (remainder != 0)
            {
                value = value - remainder;
            }

            ob_device_set_int_property(obDevice, OB_PROP_COLOR_GAIN_INT, value, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {
            LOG_WARNING("k4a color control set gain failed [ gain out of range(min=%d,max=%d,value=%d)]",
                        min,
                        max,
                        value);
            result = K4A_RESULT_FAILED;
        }
    }
    break;

    case K4A_COLOR_CONTROL_WHITEBALANCE: {
        bool enableWhiteBalance = false;
        enableWhiteBalance = ob_device_get_int_property(obDevice, OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if (mode == K4A_COLOR_CONTROL_MODE_AUTO)
        {
            if (!enableWhiteBalance)
            {
                enableWhiteBalance = true;
                ob_device_set_int_property(obDevice,
                                           OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL,
                                           enableWhiteBalance,
                                           &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
            result = K4A_RESULT_SUCCEEDED;
        }
        else
        {
            if (enableWhiteBalance)
            {
                enableWhiteBalance = false;
                ob_device_set_int_property(obDevice,
                                           OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL,
                                           enableWhiteBalance,
                                           &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }

            ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                     OB_PROP_COLOR_WHITE_BALANCE_INT,
                                                                                     &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            int min = colorParamRange.min;
            int max = colorParamRange.max;
            if (min <= value && value <= max)
            {
                int remainder = value % colorParamRange.step;
                if (remainder != 0)
                {
                    value = value - remainder;
                }

                ob_device_set_int_property(obDevice, OB_PROP_COLOR_WHITE_BALANCE_INT, value, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
            else
            {
                LOG_WARNING("k4a color control set white balance failed [white balance out of "
                            "range(min=%d,max=%d,value=%d)]",
                            min,
                            max,
                            value);
                result = K4A_RESULT_FAILED;
            }
        }
    }
    break;
    case K4A_COLOR_CONTROL_BRIGHTNESS: {

        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_BRIGHTNESS_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        int min = colorParamRange.min;
        int max = colorParamRange.max;
        if (min <= value && value <= max)
        {
            ob_device_set_int_property(obDevice, OB_PROP_COLOR_BRIGHTNESS_INT, value, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {
            LOG_WARNING("k4a color control set brightness failed [ brightness out of range(min=%d,max=%d,value=%d)]",
                        min,
                        max,
                        value);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_CONTRAST: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_CONTRAST_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        int min = colorParamRange.min;
        int max = colorParamRange.max;
        if (min <= value && value <= max)
        {
            ob_device_set_int_property(obDevice, OB_PROP_COLOR_CONTRAST_INT, value, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {
            LOG_WARNING("k4a color control set contrast failed [ contrast out of range(min=%d,max=%d,value=%d)]",
                        min,
                        max,
                        value);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_SATURATION: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_SATURATION_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        int min = colorParamRange.min;
        int max = colorParamRange.max;
        if (min <= value && value <= max)
        {
            ob_device_set_int_property(obDevice, OB_PROP_COLOR_SATURATION_INT, value, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {
            LOG_WARNING("k4a color control set saturation failed [ saturation out of range(min=%d,max=%d,value=%d)]",
                        min,
                        max,
                        value);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_SHARPNESS: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_SHARPNESS_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        int min = colorParamRange.min;
        int max = colorParamRange.max;
        if (min <= value && value <= max)
        {
            ob_device_set_int_property(obDevice, OB_PROP_COLOR_SHARPNESS_INT, value, &ob_err);

            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {
            LOG_WARNING("k4a color control set sharpness failed [ sharpness out of range(min=%d,max=%d,value=%d)]",
                        min,
                        max,
                        value);
            result = K4A_RESULT_FAILED;
        }
    }
    break;

    case K4A_COLOR_CONTROL_POWERLINE_FREQUENCY: {
        ob_int_property_range colorParamRange = ob_device_get_int_property_range(obDevice,
                                                                                 OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT,
                                                                                 &ob_err);
        CHECK_OB_ERROR(&ob_err);
        if (ob_err == NULL)
        {
            int min = colorParamRange.min;
            int max = colorParamRange.max;
            if (min <= value && value <= max)
            {
                ob_device_set_int_property(obDevice, OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT, value, &ob_err);
                CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
            }
            else
            {
                LOG_WARNING("k4a color control set powerline frequency failed [ powerline frequency out of "
                            "range(min=%d,max=%d,value=%d)]",
                            min,
                            max,
                            value);
                result = K4A_RESULT_FAILED;
            }
        }
        else if (ob_err->exception_type == OB_EXCEPTION_TYPE_UNSUPPORTED_OPERATION)
        {
            ob_device_set_int_property(obDevice, OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT, value, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
        else
        {
            LOG_WARNING("k4a color control set powerline frequency failed [get powerline frequency range failed]", 0);
            result = K4A_RESULT_FAILED;
        }
    }
    break;
    case K4A_COLOR_CONTROL_HDR:{
        bool supported = ob_device_is_property_supported(obDevice,
                                                         OB_PROP_COLOR_HDR_BOOL,
                                                         OB_PERMISSION_READ,
                                                         &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        if(supported){
            ob_device_set_bool_property(obDevice, OB_PROP_COLOR_HDR_BOOL, value, &ob_err);
            CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
        }
    }
    break;

        //  case K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY: {
        //     // LOG_WARNING("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY is deprecated and does nothing.");
        //
        // ob_int_property_range colorParamRange =
        //         ob_device_get_int_property_range(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_PRIORITY_INT,
        //                                                                               &ob_err);
        //      if (ob_err == NULL)
        //      {
        //          int min = colorParamRange.min;
        //          int max = colorParamRange.max;
        //          if (min <= value && value <= max)
        //          {
        //              ob_device_set_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_PRIORITY_INT, value, &ob_err);
        //              if (ob_err != NULL)
        //              {
        //                   LOG_WARNING("k4a color control set exposure priority failed ",0);
        //                  result = K4A_RESULT_FAILED;
        //              }
        //          }
        //          else
        //          {
        //              LOG_WARNING("k4a color control set exposure priority  failed [exposure priority out of "
        //                          "range(min=%d,max=%d,value=%d)]",
        //                          min,
        //                          max,
        //                          value);
        //              result = K4A_RESULT_FAILED;
        //          }
        //      }
        //      else if (ob_err->exception_type == OB_EXCEPTION_TYPE_UNSUPPORTED_OPERATION)
        //      {
        //          ob_device_set_int_property(obDevice, OB_PROP_COLOR_AUTO_EXPOSURE_PRIORITY_INT, value, &ob_err);
        //          if (ob_err != NULL)
        //          {
        //              LOG_WARNING("k4a color control set exposure priority failed ",0);
        //              result = K4A_RESULT_FAILED;
        //          }
        //      }
        //      else
        //      {
        //          LOG_WARNING("k4a color control set exposure priority  failed [get exposure priority  range
        //          failed]",0); result = K4A_RESULT_FAILED;
        //      }

        //  }
        //  break;
    default:
        LOG_ERROR("Failing, unknown command %u", command);
        return K4A_RESULT_FAILED;
    }

    return result;
}

/*
#include <stdio.h>

bool get_calibration_json_file(uint8_t *data,size_t *data_size)
{
    FILE *file;
    //file = fopen("F:\\Camera_Calibration_Json_File.json","rb");
    file = fopen("F:\\kinect1.json", "rb");

    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *calibration_file = (uint8_t *)malloc(len + 1);
    fread(calibration_file, 1, len, file);
    fclose(file);

    if (*data_size > len)
    {
        memcpy(data,calibration_file,len);
        *data_size = len;
        free(calibration_file);
        return true;
    }
    else
    {
        free(calibration_file);
        return false;
    }
}

bool writeFile(uint8_t* data, size_t* data_size)
{
    FILE *file;
    file = fopen("kinect1_1.json", "w");

    fwrite(data, 1, data_size, file);
    fclose(file);
    return true;
}
*/

k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle, uint8_t *data, size_t *data_size)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_BUFFER_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_BUFFER_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    if (device_ctx->device == NULL)
    {
        return K4A_BUFFER_RESULT_FAILED;
    }

    k4a_buffer_result_t bresult = K4A_BUFFER_RESULT_SUCCEEDED;
    if (data == NULL)
    {
        (*data_size) = 1024 * 10;
        bresult = K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
#if 0

        bool bRet = get_calibration_json_file(data, data_size);
       if (bRet)
        {
            bresult = K4A_BUFFER_RESULT_SUCCEEDED;
        }
        else
        {
            bresult = K4A_BUFFER_RESULT_TOO_SMALL;
        }
#else
        if (device_ctx->json && device_ctx->json->calibration_json && device_ctx->json->status == JSON_FILE_VALID)
        {
            if (*data_size < device_ctx->json->json_actual_size)
            {
                bresult = K4A_BUFFER_RESULT_FAILED;
            }
            else
            {
                *data_size = device_ctx->json->json_actual_size;
                memcpy(data, device_ctx->json->calibration_json, *data_size);
                // writeFile(data, *data_size);
            }
        }
        else
        {
            bresult = K4A_BUFFER_RESULT_FAILED;
        }
#endif
    }

    return bresult;
}

k4a_result_t k4a_device_get_calibration_from_json(k4a_device_t device_handle,
                                                  const k4a_depth_mode_t depth_mode,
                                                  const k4a_color_resolution_t color_resolution,
                                                  k4a_calibration_t *calibration)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);

    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);
    if (device_ctx->json && device_ctx->json->calibration_json && device_ctx->json->status == JSON_FILE_VALID)
    {
        char *json = device_ctx->json->calibration_json;
        size_t json_len = device_ctx->json->json_actual_size;
        return k4a_calibration_get_from_raw(json, json_len, depth_mode, color_resolution, calibration);
    }

    LOG_WARNING("json file parse failed ", 0);
    return K4A_RESULT_FAILED;

}

k4a_result_t k4a_device_get_calibration_from_orbbec_sdk(k4a_device_t device_handle,
                                                        const k4a_depth_mode_t depth_mode,
                                                        const k4a_color_resolution_t color_resolution,
                                                        k4a_calibration_t *calibration)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    int k4a_depth_width = 0;
    int k4a_depth_height = 0;
    switch (depth_mode)
    {
    case K4A_DEPTH_MODE_NFOV_2X2BINNED: // 320x288
        k4a_depth_width = 320;
        k4a_depth_height = 288;
        break;
    case K4A_DEPTH_MODE_NFOV_UNBINNED: // 640x576
        k4a_depth_width = 640;
        k4a_depth_height = 576;
        break;
    case K4A_DEPTH_MODE_WFOV_2X2BINNED: // 512x512
        k4a_depth_width = 512;
        k4a_depth_height = 512;
        break;
    case K4A_DEPTH_MODE_WFOV_UNBINNED: // 1024x1024
        k4a_depth_width = 1024;
        k4a_depth_height = 1024;
        break;
    default:
        break;
    }

    if (k4a_depth_width == 0 && k4a_depth_height == 0)
    {
        LOG_WARNING("param invalid ", 0);
        return K4A_RESULT_FAILED;
    }

    int k4a_color_width = 0;
    int k4a_color_height = 0;
    switch (color_resolution)
    {
    case K4A_COLOR_RESOLUTION_720P:
        k4a_color_width = 1280;
        k4a_color_height = 720;
        break;
    case K4A_COLOR_RESOLUTION_1080P:
        k4a_color_width = 1920;
        k4a_color_height = 1080;
        break;
    case K4A_COLOR_RESOLUTION_1440P:
        k4a_color_width = 2560;
        k4a_color_height = 1440;
        break;
    case K4A_COLOR_RESOLUTION_1536P:
        k4a_color_width = 2048;
        k4a_color_height = 1536;
        break;
    case K4A_COLOR_RESOLUTION_2160P:
        k4a_color_width = 3840;
        k4a_color_height = 2160;
        break;
    case K4A_COLOR_RESOLUTION_3072P:
        k4a_color_width = 4096;
        k4a_color_height = 3072;
        break;
    default:
        break;
    }

    ob_error *ob_err = NULL;
    bool is_property_support =
        ob_device_is_property_supported(device_ctx->device, OB_PROP_D2C_PREPROCESS_BOOL, OB_PERMISSION_WRITE, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    if (is_property_support)
    {
        ob_device_set_bool_property(device_ctx->device, OB_PROP_D2C_PREPROCESS_BOOL, true, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    }

    ob_camera_param_list *camera_param_list = ob_device_get_calibration_camera_param_list(device_ctx->device, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    uint32_t camera_list_count = ob_camera_param_list_count(camera_param_list, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

    ob_camera_param camera_param;
    bool find = false;
    memset(&camera_param, 0, sizeof(camera_param));
    for (uint32_t i = 0; i < camera_list_count; i++)
    {

        ob_camera_param param = ob_camera_param_list_get_param(camera_param_list, i, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);

        float depth_width = (float)param.depthIntrinsic.width;
        float depth_height = (float)param.depthIntrinsic.height;
        float color_width = (float)param.rgbIntrinsic.width;
        float color_height = (float)param.rgbIntrinsic.height;

        if (depth_mode != K4A_DEPTH_MODE_OFF && color_resolution != K4A_COLOR_RESOLUTION_OFF)
        {
            if (((color_width / color_height) == ((float)k4a_color_width / (float)k4a_color_height)) &&
                (((depth_width / depth_height) == ((float)k4a_depth_width / (float)k4a_depth_height))))
            {
                find = true;
                memcpy(&camera_param, &param, sizeof(ob_camera_param));
                break;
            }
        }
        else if (depth_mode != K4A_DEPTH_MODE_OFF)
        {
            if ((depth_width / depth_height) == ((float)k4a_depth_width / (float)k4a_depth_height))
            {
                find = true;
                memcpy(&camera_param, &param, sizeof(ob_camera_param));
                break;
            }
        }
        else if (color_resolution != K4A_COLOR_RESOLUTION_OFF)
        {
            if ((color_width / color_height) == ((float)k4a_color_width / (float)k4a_color_height))
            {
                find = true;
                memcpy(&camera_param, &param, sizeof(ob_camera_param));
                break;
            }
        }
    }

    if (!find)
    {
        LOG_WARNING("ob_camera_param_list_get_param failed ", 0);
        return K4A_RESULT_FAILED;
    }

    if(camera_param_list != NULL){
        ob_delete_camera_param_list(camera_param_list, &ob_err);
        CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    }

    ob_camera_intrinsic ob_depth_intrinsic = camera_param.depthIntrinsic;
    ob_camera_intrinsic ob_color_intrinsic = camera_param.rgbIntrinsic;

    ob_camera_distortion ob_depth_distortion = camera_param.depthDistortion;
    ob_camera_distortion ob_color_distortion = camera_param.rgbDistortion;

    ob_d2c_transform ob_extrinsics = camera_param.transform;

    memset(calibration, 0, sizeof(k4a_calibration_t));

    calibration->depth_mode = depth_mode;
    calibration->color_resolution = color_resolution;

    if (depth_mode != K4A_DEPTH_MODE_OFF)
    {
        calibration->depth_camera_calibration.resolution_width = ob_depth_intrinsic.width;
        calibration->depth_camera_calibration.resolution_height = ob_depth_intrinsic.height;
        calibration->depth_camera_calibration.intrinsics.parameter_count = 14;
        calibration->depth_camera_calibration.intrinsics.type = K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY;

        float depth_factor = (float)k4a_depth_width / (float)calibration->depth_camera_calibration.resolution_width;
        calibration->depth_camera_calibration.resolution_width = k4a_depth_width;
        calibration->depth_camera_calibration.resolution_height = k4a_depth_height;

        calibration->depth_camera_calibration.intrinsics.parameters.param.fx = ob_depth_intrinsic.fx * depth_factor;
        calibration->depth_camera_calibration.intrinsics.parameters.param.fy = ob_depth_intrinsic.fy * depth_factor;

        calibration->depth_camera_calibration.intrinsics.parameters.param.cx = ob_depth_intrinsic.cx * depth_factor;
        calibration->depth_camera_calibration.intrinsics.parameters.param.cy = ob_depth_intrinsic.cy * depth_factor;

        calibration->depth_camera_calibration.intrinsics.parameters.param.codx = 0;
        calibration->depth_camera_calibration.intrinsics.parameters.param.cody = 0;

        calibration->depth_camera_calibration.intrinsics.parameters.param.k1 = ob_depth_distortion.k1;
        calibration->depth_camera_calibration.intrinsics.parameters.param.k2 = ob_depth_distortion.k2;
        calibration->depth_camera_calibration.intrinsics.parameters.param.k3 = ob_depth_distortion.k3;
        calibration->depth_camera_calibration.intrinsics.parameters.param.k4 = 0;
        calibration->depth_camera_calibration.intrinsics.parameters.param.k5 = 0;
        calibration->depth_camera_calibration.intrinsics.parameters.param.k6 = 0;

        calibration->depth_camera_calibration.intrinsics.parameters.param.p1 = ob_depth_distortion.p1;
        calibration->depth_camera_calibration.intrinsics.parameters.param.p2 = ob_depth_distortion.p2;

        // memcpy(&calibration->depth_camera_calibration.extrinsics, &ob_extrinsics, sizeof(ob_d2c_transform));

        calibration->depth_camera_calibration.extrinsics.rotation[0] = 1;
        calibration->depth_camera_calibration.extrinsics.rotation[1] = 0;
        calibration->depth_camera_calibration.extrinsics.rotation[2] = 0;
        calibration->depth_camera_calibration.extrinsics.rotation[3] = 0;
        calibration->depth_camera_calibration.extrinsics.rotation[4] = 1;
        calibration->depth_camera_calibration.extrinsics.rotation[5] = 0;
        calibration->depth_camera_calibration.extrinsics.rotation[6] = 0;
        calibration->depth_camera_calibration.extrinsics.rotation[7] = 0;
        calibration->depth_camera_calibration.extrinsics.rotation[8] = 1;

        calibration->depth_camera_calibration.extrinsics.translation[0] = 0;
        calibration->depth_camera_calibration.extrinsics.translation[1] = 0;
        calibration->depth_camera_calibration.extrinsics.translation[2] = 0;
    }

    if (color_resolution != K4A_COLOR_RESOLUTION_OFF)
    {

        calibration->color_camera_calibration.resolution_width = ob_color_intrinsic.width;
        calibration->color_camera_calibration.resolution_height = ob_color_intrinsic.height;
        calibration->color_camera_calibration.intrinsics.parameter_count = 14;
        calibration->color_camera_calibration.intrinsics.type = K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY;

        float color_factor = (float)k4a_color_width / (float)calibration->color_camera_calibration.resolution_width;

        calibration->color_camera_calibration.resolution_width = k4a_color_width;
        calibration->color_camera_calibration.resolution_height = k4a_color_height;

        calibration->color_camera_calibration.intrinsics.parameters.param.fx = ob_color_intrinsic.fx * color_factor;
        calibration->color_camera_calibration.intrinsics.parameters.param.fy = ob_color_intrinsic.fy * color_factor;

        calibration->color_camera_calibration.intrinsics.parameters.param.cx = ob_color_intrinsic.cx * color_factor;
        calibration->color_camera_calibration.intrinsics.parameters.param.cy = ob_color_intrinsic.cy * color_factor;

        calibration->color_camera_calibration.intrinsics.parameters.param.codx = 0;
        calibration->color_camera_calibration.intrinsics.parameters.param.cody = 0;

        calibration->color_camera_calibration.intrinsics.parameters.param.k1 = ob_color_distortion.k1;
        calibration->color_camera_calibration.intrinsics.parameters.param.k2 = ob_color_distortion.k2;
        calibration->color_camera_calibration.intrinsics.parameters.param.k3 = ob_color_distortion.k3;
        calibration->color_camera_calibration.intrinsics.parameters.param.k4 = 0;
        calibration->color_camera_calibration.intrinsics.parameters.param.k5 = 0;
        calibration->color_camera_calibration.intrinsics.parameters.param.k6 = 0;

        calibration->color_camera_calibration.intrinsics.parameters.param.p1 = ob_color_distortion.p1;
        calibration->color_camera_calibration.intrinsics.parameters.param.p2 = ob_color_distortion.p2;

        memcpy(&calibration->color_camera_calibration.extrinsics, &ob_extrinsics, sizeof(ob_d2c_transform));
    }

    for (int i = 0; i < K4A_CALIBRATION_TYPE_NUM; i++)
    {
        for (int j = 0; j < K4A_CALIBRATION_TYPE_NUM; j++)
        {
            memcpy(&calibration->extrinsics[i][j], &ob_extrinsics, sizeof(ob_d2c_transform));
        }
    }

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle,
                                        const k4a_depth_mode_t depth_mode,
                                        const k4a_color_resolution_t color_resolution,
                                        k4a_calibration_t *calibration)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, k4a_device_t, device_handle);
    CHECK_AND_TRY_INIT_DEVICE_CONTEXT(K4A_RESULT_FAILED, device_handle);
    k4a_device_context_t *device_ctx = k4a_device_t_get_context(device_handle);

    ob_error *ob_err = NULL;
    ob_device_info *dev_info = ob_device_get_device_info(device_ctx->device, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    int pid = ob_device_info_pid(dev_info, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    ob_delete_device_info(dev_info, &ob_err);
    CHECK_OB_ERROR_RETURN_K4A_RESULT(&ob_err);
    if (pid == ORBBEC_MEGA_PID || pid == ORBBEC_BOLT_PID)
    {
        return k4a_device_get_calibration_from_json(device_handle, depth_mode, color_resolution, calibration);
    }
    else
    {
        return k4a_device_get_calibration_from_orbbec_sdk(device_handle, depth_mode, color_resolution, calibration);
    }
}

k4a_result_t k4a_calibration_get_from_raw(char *raw_calibration,
                                          size_t raw_calibration_size,
                                          const k4a_depth_mode_t depth_mode,
                                          const k4a_color_resolution_t color_resolution,
                                          k4a_calibration_t *calibration)
{

    k4a_calibration_camera_t depth_calibration;
    k4a_calibration_camera_t color_calibration;
    k4a_calibration_imu_t gyro_calibration;
    k4a_calibration_imu_t accel_calibration;
    k4a_result_t result;

    result = TRACE_CALL(calibration_create_from_raw(raw_calibration,
                                                    raw_calibration_size,
                                                    &depth_calibration,
                                                    &color_calibration,
                                                    &gyro_calibration,
                                                    &accel_calibration));

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(transformation_get_mode_specific_calibration(&depth_calibration,
                                                                         &color_calibration,
                                                                         &gyro_calibration.depth_to_imu,
                                                                         &accel_calibration.depth_to_imu,
                                                                         depth_mode,
                                                                         color_resolution,
                                                                         calibration));
    }
    return result;
}

k4a_result_t k4a_calibration_3d_to_3d(const k4a_calibration_t *calibration,
                                      const k4a_float3_t *source_point3d_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float3_t *target_point3d_mm)
{
    return TRACE_CALL(
        transformation_3d_to_3d(calibration, source_point3d_mm->v, source_camera, target_camera, target_point3d_mm->v));
}

k4a_result_t k4a_calibration_2d_to_3d(const k4a_calibration_t *calibration,
                                      const k4a_float2_t *source_point2d,
                                      const float source_depth_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float3_t *target_point3d_mm,
                                      int *valid)
{
    k4a_result_t result = transformation_2d_to_3d(
        calibration, source_point2d->v, source_depth_mm, source_camera, target_camera, target_point3d_mm->v, valid);
    return result;
}

k4a_result_t k4a_calibration_3d_to_2d(const k4a_calibration_t *calibration,
                                      const k4a_float3_t *source_point3d_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float2_t *target_point2d,
                                      int *valid)
{

    return TRACE_CALL(transformation_3d_to_2d(
        calibration, source_point3d_mm->v, source_camera, target_camera, target_point2d->v, valid));
}

k4a_result_t k4a_calibration_2d_to_2d(const k4a_calibration_t *calibration,
                                      const k4a_float2_t *source_point2d,
                                      const float source_depth_mm,
                                      const k4a_calibration_type_t source_camera,
                                      const k4a_calibration_type_t target_camera,
                                      k4a_float2_t *target_point2d,
                                      int *valid)
{
    return TRACE_CALL(transformation_2d_to_2d(
        calibration, source_point2d->v, source_depth_mm, source_camera, target_camera, target_point2d->v, valid));
}

k4a_result_t k4a_calibration_color_2d_to_depth_2d(const k4a_calibration_t *calibration,
                                                  const k4a_float2_t *source_point2d,
                                                  const k4a_image_t depth_image,
                                                  k4a_float2_t *target_point2d,
                                                  int *valid)
{
    return TRACE_CALL(
        transformation_color_2d_to_depth_2d(calibration, source_point2d->v, depth_image, target_point2d->v, valid));
}

k4a_transformation_t k4a_transformation_create(const k4a_calibration_t *calibration)
{
    return transformation_create(calibration, TRANSFORM_ENABLE_GPU_OPTIMIZATION);
}

void k4a_transformation_destroy(k4a_transformation_t transformation_handle)
{
    transformation_destroy(transformation_handle);
}

static k4a_transformation_image_descriptor_t k4a_image_get_descriptor(const k4a_image_t image)
{

    k4a_transformation_image_descriptor_t descriptor;
    descriptor.width_pixels = k4a_image_get_width_pixels(image);
    descriptor.height_pixels = k4a_image_get_height_pixels(image);
    descriptor.stride_bytes = k4a_image_get_stride_bytes(image);
    descriptor.format = k4a_image_get_format(image);
    return descriptor;
}

k4a_result_t k4a_transformation_depth_image_to_color_camera(k4a_transformation_t transformation_handle,
                                                            const k4a_image_t depth_image,
                                                            k4a_image_t transformed_depth_image)
{

    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t transformed_depth_image_descriptor = k4a_image_get_descriptor(
        transformed_depth_image);

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *transformed_depth_image_buffer = k4a_image_get_buffer(transformed_depth_image);

    // Both k4a_transformation_depth_image_to_color_camera and k4a_transformation_depth_image_to_color_camera_custom
    // call the same implementation of transformation_depth_image_to_color_camera_custom. The below parameters need
    // to be passed in but they will be ignored in the internal implementation.
    k4a_transformation_image_descriptor_t dummy_descriptor = { 0 };
    uint8_t *custom_image_buffer = NULL;
    uint8_t *transformed_custom_image_buffer = NULL;
    k4a_transformation_interpolation_type_t interpolation_type = K4A_TRANSFORMATION_INTERPOLATION_TYPE_LINEAR;
    uint32_t invalid_custom_value = 0;

    return TRACE_CALL(transformation_depth_image_to_color_camera_custom(transformation_handle,
                                                                        depth_image_buffer,
                                                                        &depth_image_descriptor,
                                                                        custom_image_buffer,
                                                                        &dummy_descriptor,
                                                                        transformed_depth_image_buffer,
                                                                        &transformed_depth_image_descriptor,
                                                                        transformed_custom_image_buffer,
                                                                        &dummy_descriptor,
                                                                        interpolation_type,
                                                                        invalid_custom_value));
}

k4a_result_t
k4a_transformation_depth_image_to_color_camera_custom(k4a_transformation_t transformation_handle,
                                                      const k4a_image_t depth_image,
                                                      const k4a_image_t custom_image,
                                                      k4a_image_t transformed_depth_image,
                                                      k4a_image_t transformed_custom_image,
                                                      k4a_transformation_interpolation_type_t interpolation_type,
                                                      uint32_t invalid_custom_value)
{

    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t custom_image_descriptor = k4a_image_get_descriptor(custom_image);
    k4a_transformation_image_descriptor_t transformed_depth_image_descriptor = k4a_image_get_descriptor(
        transformed_depth_image);
    k4a_transformation_image_descriptor_t transformed_custom_image_descriptor = k4a_image_get_descriptor(
        transformed_custom_image);

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *custom_image_buffer = k4a_image_get_buffer(custom_image);
    uint8_t *transformed_depth_image_buffer = k4a_image_get_buffer(transformed_depth_image);
    uint8_t *transformed_custom_image_buffer = k4a_image_get_buffer(transformed_custom_image);

    return TRACE_CALL(transformation_depth_image_to_color_camera_custom(transformation_handle,
                                                                        depth_image_buffer,
                                                                        &depth_image_descriptor,
                                                                        custom_image_buffer,
                                                                        &custom_image_descriptor,
                                                                        transformed_depth_image_buffer,
                                                                        &transformed_depth_image_descriptor,
                                                                        transformed_custom_image_buffer,
                                                                        &transformed_custom_image_descriptor,
                                                                        interpolation_type,
                                                                        invalid_custom_value));
}

k4a_result_t k4a_transformation_color_image_to_depth_camera(k4a_transformation_t transformation_handle,
                                                            const k4a_image_t depth_image,
                                                            const k4a_image_t color_image,
                                                            k4a_image_t transformed_color_image)
{
    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t color_image_descriptor = k4a_image_get_descriptor(color_image);
    k4a_transformation_image_descriptor_t transformed_color_image_descriptor = k4a_image_get_descriptor(
        transformed_color_image);

    k4a_image_format_t color_image_format = k4a_image_get_format(color_image);
    k4a_image_format_t transformed_color_image_format = k4a_image_get_format(transformed_color_image);
    if (!(color_image_format == K4A_IMAGE_FORMAT_COLOR_BGRA32 &&
          transformed_color_image_format == K4A_IMAGE_FORMAT_COLOR_BGRA32))
    {
        LOG_ERROR("Require color image and transformed color image both have bgra32 format.", 0);
        return K4A_RESULT_FAILED;
    }

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *color_image_buffer = k4a_image_get_buffer(color_image);
    uint8_t *transformed_color_image_buffer = k4a_image_get_buffer(transformed_color_image);

    return TRACE_CALL(transformation_color_image_to_depth_camera(transformation_handle,
                                                                 depth_image_buffer,
                                                                 &depth_image_descriptor,
                                                                 color_image_buffer,
                                                                 &color_image_descriptor,
                                                                 transformed_color_image_buffer,
                                                                 &transformed_color_image_descriptor));
}

k4a_result_t k4a_transformation_depth_image_to_point_cloud(k4a_transformation_t transformation_handle,
                                                           const k4a_image_t depth_image,
                                                           const k4a_calibration_type_t camera,
                                                           k4a_image_t xyz_image)
{

    k4a_transformation_image_descriptor_t depth_image_descriptor = k4a_image_get_descriptor(depth_image);
    k4a_transformation_image_descriptor_t xyz_image_descriptor = k4a_image_get_descriptor(xyz_image);

    uint8_t *depth_image_buffer = k4a_image_get_buffer(depth_image);
    uint8_t *xyz_image_buffer = k4a_image_get_buffer(xyz_image);

    return TRACE_CALL(transformation_depth_image_to_point_cloud(transformation_handle,
                                                                depth_image_buffer,
                                                                &depth_image_descriptor,
                                                                camera,
                                                                xyz_image_buffer,
                                                                &xyz_image_descriptor));
}

#ifdef __cplusplus
}
#endif
