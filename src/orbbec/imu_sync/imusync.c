// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <imusync.h>

// Dependent libraries
#include <frame_queue.h>
#include <k4ainternal/handle.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/common.h>

#include <azure_c_shared_utility/lock.h>
#include <azure_c_shared_utility/envvariable.h>

// System dependencies
#include <stdlib.h>
#include <stdbool.h>

#define IMU_QUEUE_DEFAULT_SIZE 26

#define IMU_GRAVITATIONAL_CONSTANT 9.81f

#define PI 3.141592f
#define IMU_RADIANS_PER_DEGREES (PI / 180.0f)

typedef struct _imusync_context_t
{
    frame_queue_t sync_queue;    // Queue for storing synchronized captures in
    frame_queue_t accel_queue;
    frame_queue_t gyro_queue;

    volatile bool running;              // We have received start and should be processing data when true.
    LOCK_HANDLE lock;

} imusync_context_t;

K4A_DECLARE_CONTEXT(imusync_t, imusync_context_t);

void free_imu_buff(k4a_capture_t capture)
{
    void *pBuff = (void *)capture;
    if (pBuff != NULL)
    {
        free(pBuff);
        pBuff = NULL;
    }
}


void imusync_push_frame(imusync_t imusync_handle, imu_frame_data imu_data, imu_data_type imu_type)
{
    imusync_context_t *sync = NULL;
    k4a_wait_result_t wresult = K4A_WAIT_RESULT_SUCCEEDED;
    k4a_result_t result;
    bool locked = false;

    result = K4A_RESULT_FROM_BOOL(imusync_handle != NULL);
    if (K4A_SUCCEEDED(result))
    {
        sync = imusync_t_get_context(imusync_handle);
        result = K4A_RESULT_FROM_BOOL(sync != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        Lock(sync->lock);
        locked = true;
        result = sync->running == true ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;
    }

    if (K4A_SUCCEEDED(result))
    {
        // TODO: ʹ���ڴ�ع���
        imu_sync_frame_data *p_imu_frame_data = (imu_sync_frame_data *)malloc(sizeof(imu_sync_frame_data));
        p_imu_frame_data->temp = imu_data.temp;
        p_imu_frame_data->timestamp = imu_data.timestamp;

        if (imu_type == ACCEL_FRAME_TYPE)
        {
            memcpy(p_imu_frame_data->accel_data, imu_data.data, sizeof(imu_data.data));
            frame_queue_push(sync->accel_queue, (k4a_capture_t)p_imu_frame_data);
        }
        else
        {
            memcpy(p_imu_frame_data->gyro_data, imu_data.data, sizeof(imu_data.data));
            frame_queue_push(sync->gyro_queue, (k4a_capture_t)p_imu_frame_data);
        }

        //2.ȡ����ͷ�ڵ�
        k4a_capture_t accel_data = get_frame_queue_front(sync->accel_queue);
        k4a_capture_t gyro_data = get_frame_queue_front(sync->gyro_queue);
        if (accel_data != NULL && gyro_data != NULL)
        {
            imu_sync_frame_data *p_accel_data = (imu_sync_frame_data *)accel_data;
            imu_sync_frame_data *p_gyro_data = (imu_sync_frame_data *)gyro_data;
            if (p_accel_data->timestamp == p_gyro_data->timestamp)
            {
                //�ϳ�һ��Imu��push��ͬ������
                imu_sync_frame_data *p_imu_sync_frame_data = (imu_sync_frame_data *) malloc(sizeof(imu_sync_frame_data));

                p_imu_sync_frame_data->timestamp = p_accel_data->timestamp;
                p_imu_sync_frame_data->temp = p_accel_data->temp;
                memcpy(p_imu_sync_frame_data->accel_data,p_accel_data->accel_data,sizeof(p_accel_data->accel_data));
                memcpy(p_imu_sync_frame_data->gyro_data, p_gyro_data->gyro_data, sizeof(p_gyro_data->gyro_data));

                frame_queue_push(sync->sync_queue, (k4a_capture_t)p_imu_sync_frame_data);

                k4a_capture_t accel_handle = NULL;
                wresult =frame_queue_pop(sync->accel_queue, 0, &accel_handle);
                if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
                {
                    free_imu_buff(accel_handle);
                }

                k4a_capture_t gyro_handle = NULL;
                wresult =frame_queue_pop(sync->gyro_queue, 0, &gyro_handle);
                if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
                {
                    free_imu_buff(gyro_handle);
                }
            }
            else if (p_accel_data->timestamp > p_gyro_data->timestamp)
            {
                //pop gyro_data
                k4a_capture_t capture_handle;
                wresult = frame_queue_pop(sync->gyro_queue, 0, &capture_handle);
                if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
                {
                    free_imu_buff(capture_handle);
                }
            }
            else
            {
                // pop accel_data
                k4a_capture_t capture_handle;
                wresult = frame_queue_pop(sync->accel_queue, 0, &capture_handle);
                if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
                {
                    free_imu_buff(capture_handle);
                }
            }
        }
    }

    if (locked)
    {
        Unlock(sync->lock);
        locked = false;
    }
}

k4a_result_t imusync_create(imusync_t *imusync_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, imusync_handle == NULL);

    imusync_context_t *sync = imusync_t_create(imusync_handle);
    k4a_result_t result = K4A_RESULT_FROM_BOOL(sync != NULL);


    if (K4A_SUCCEEDED(result))
    {
        sync->lock = Lock_Init();
        result = K4A_RESULT_FROM_BOOL(sync->lock != NULL);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(frame_queue_create(IMU_QUEUE_DEFAULT_SIZE,
                                               "Queue_depth",
                                               &sync->accel_queue,
                                               free_imu_buff));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(frame_queue_create(IMU_QUEUE_DEFAULT_SIZE,
                                               "Queue_color",
                                               &sync->gyro_queue,
                                               free_imu_buff));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = TRACE_CALL(frame_queue_create(IMU_QUEUE_DEFAULT_SIZE / 2,
                                               "Queue_capture",
                                               &sync->sync_queue,
                                               free_imu_buff));
    }

    if (K4A_SUCCEEDED(result))
    {
        frame_queue_disable(sync->accel_queue);
        frame_queue_disable(sync->gyro_queue);
        frame_queue_disable(sync->sync_queue);
    }

    if (K4A_FAILED(result))
    {
        imusync_destroy(*imusync_handle);
        *imusync_handle = NULL;
    }

    return result;
}

void imusync_destroy(imusync_t imusync_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, imusync_t, imusync_handle);
    imusync_context_t *sync = imusync_t_get_context(imusync_handle);

    imusync_stop(imusync_handle);

    if (sync->accel_queue)
    {
        frame_queue_destroy(sync->accel_queue);
    }
    if (sync->gyro_queue)
    {
        frame_queue_destroy(sync->gyro_queue);
    }
    if (sync->sync_queue)
    {
        frame_queue_destroy(sync->sync_queue);
    }

    Lock_Deinit(sync->lock);
    imusync_t_destroy(imusync_handle);
}

k4a_result_t imusync_start(imusync_t imusync_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_RESULT_FAILED, imusync_t, imusync_handle);
    imusync_context_t *sync = imusync_t_get_context(imusync_handle);


    frame_queue_enable(sync->accel_queue);
    frame_queue_enable(sync->gyro_queue);
    frame_queue_enable(sync->sync_queue);

    // Not taking the lock as we don't need to synchronize this on start
    sync->running = true;

    return K4A_RESULT_SUCCEEDED;
}

void imusync_stop(imusync_t imusync_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, imusync_t, imusync_handle);
    imusync_context_t *sync = imusync_t_get_context(imusync_handle);

    Lock(sync->lock);
    sync->running = false;

    if (sync->accel_queue)
    {
        frame_queue_disable(sync->accel_queue);
    }
    if (sync->gyro_queue)
    {
        frame_queue_disable(sync->gyro_queue);
    }
    if (sync->sync_queue)
    {
        frame_queue_disable(sync->sync_queue);
    }

    Unlock(sync->lock);
}

k4a_wait_result_t imusync_get_frame(imusync_t imusync_handle, k4a_imu_sample_t *capture, int32_t timeout_in_ms)
{
    RETURN_VALUE_IF_HANDLE_INVALID(K4A_WAIT_RESULT_FAILED, imusync_t, imusync_handle);
    RETURN_VALUE_IF_ARG(K4A_WAIT_RESULT_FAILED, capture == NULL);

    imusync_context_t *sync = imusync_t_get_context(imusync_handle);
    k4a_capture_t capture_handle;

    k4a_wait_result_t wresult = frame_queue_pop(sync->sync_queue, timeout_in_ms, &capture_handle);

    if (wresult == K4A_WAIT_RESULT_SUCCEEDED)
    {
        imu_sync_frame_data *p_imu_sync_frame_data = (imu_sync_frame_data *)capture_handle;
        capture->acc_timestamp_usec = p_imu_sync_frame_data->timestamp;
        capture->gyro_timestamp_usec = p_imu_sync_frame_data->timestamp;
        capture->temperature = p_imu_sync_frame_data->temp;
        //��λת�� g to m/s/s
        capture->acc_sample.xyz.x = (float)(p_imu_sync_frame_data->accel_data[0] );
        capture->acc_sample.xyz.y = (float)(p_imu_sync_frame_data->accel_data[1] );
        capture->acc_sample.xyz.z = (float)(p_imu_sync_frame_data->accel_data[2] );

        capture->gyro_sample.xyz.x = (float)(p_imu_sync_frame_data->gyro_data[0] );
        capture->gyro_sample.xyz.y = (float)(p_imu_sync_frame_data->gyro_data[1] );
        capture->gyro_sample.xyz.z = (float)(p_imu_sync_frame_data->gyro_data[2] );

    }

    free_imu_buff(capture_handle);
    return wresult;
}
