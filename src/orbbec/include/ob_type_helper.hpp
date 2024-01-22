#pragma once

#include "k4a/k4a.h"
#include <libobsensor/h/Context.h>
#include <libobsensor/h/Error.h>
#include <libobsensor/h/Version.h>
#include <libobsensor/h/Device.h>
#include <memory>
#include <vector>
#include <string>

k4a_result_t check_ob_error(ob_error *error);

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
        CHECK_OB_ERROR_RETURN(error);

        auto device_count = ob_device_list_device_count(device_list, &error);
        CHECK_OB_ERROR_RETURN(error);

        for (uint32_t i = 0; i < device_count; i++)
        {
            auto uid = ob_device_list_get_device_uid(device_list, i, &error);
            device_uid_list.push_back(uid);
        }
        ob_delete_device_list(device_list, &error);
        CHECK_OB_ERROR_RETURN(error);

        ob_set_device_changed_callback(context, on_device_changed_callback, this, &error);
        CHECK_OB_ERROR_RETURN(error);
    };

    ~ob_context_handler()
    {
        if (context != nullptr)
        {
            ob_error *error = nullptr;
            ob_delete_context(context, &error);
            check_ob_error(error);
        }
    }
    ob_context *context;
    std::vector<std::string> device_uid_list;
};
std::shared_ptr<ob_context_handler> get_ob_context_handler_instance();

class depthengine_context
{
public:
    depthengine_context()
    {
        char json_raw[] = "{\"CalibrationInformation\": {\"Cameras\": [{\"Intrinsics\": {\"ModelParameterCount\": 14,\"ModelParameters\": [0.51141625642776489,0.49393671751022339,0.49239182472229004,0.49228072166442871,18.309083938598633,9.0141849517822266,0.31449863314628601,18.613100051879883,15.208014488220215,2.0382039546966553,0.0,0.0,-2.4292179659823887e-5,3.0484003218589351e-5],\"ModelType\": \"CALIBRATION_LensDistortionModelRational6KT\"},\"Location\": \"CALIBRATION_CameraLocationD0\",\"Purpose\": \"CALIBRATION_CameraPurposeDepth\",\"MetricRadius\": 0,\"Rt\": {\"Rotation\": [1,0,0,0,1,0,0,0,1],\"Translation\": [0,0,0]},\"SensorHeight\": 1024,\"SensorWidth\": 1024,\"Shutter\": \"CALIBRATION_ShutterTypeUndefined\",\"ThermalAdjustmentParams\": {\"Params\": [0,0,0,0,0,0,0,0,0,0,0,0]}},{\"Intrinsics\": {\"ModelParameterCount\": 14,\"ModelParameters\": [0.50146462122599289,0.47872441609700522,0.58478528261184692,0.77964627742767334,13.921833038330078,9.3853960037231445,12.013153076171875,13.829848289489746,8.4936428070068359,12.522320747375488,0.0,0.0,-0.00041514806798659265,-0.00012913208047393709],\"ModelType\": \"CALIBRATION_LensDistortionModelRational6KT\"},\"Location\": \"CALIBRATION_CameraLocationPV0\",\"Purpose\": \"CALIBRATION_CameraPurposePhotoVideo\",\"MetricRadius\": 0,\"Rt\": {\"Rotation\": [0.99379253387451172,0.0053923730738461018,-0.00082581327296793461,-0.0052670259028673172,0.99377810955047607,0.11125373095273972,0.0014205967308953404,-0.11124772578477859,0.99379169940948486],\"Translation\": [-0.032658439129590988,-0.0012330524623394012,0.0018142733024433255]},\"SensorHeight\": 3072,\"SensorWidth\": 4096,\"Shutter\": \"CALIBRATION_ShutterTypeUndefined\",\"ThermalAdjustmentParams\": {\"Params\": [0,0,0,0,0,0,0,0,0,0,0,0]}}],\"InertialSensors\": [{\"BiasTemperatureModel\": [-0.00070090778172016144,0.0,0.0,0.0,-0.002562826732173562,0.0,0.0,0.0,-0.013967962004244328,0.0,0.0,0.0],\"BiasUncertainty\": [9.9999997473787516e-5,9.9999997473787516e-5,9.9999997473787516e-5],\"Id\": \"CALIBRATION_InertialSensorId_LSM6DSM\",\"MixingMatrixTemperatureModel\": [0.99787753820419312,0.0,0.0,0.0,0.0021519437432289124,0.0,0.0,0.0,0.0040874113328754902,0.0,0.0,0.0,0.0021521428134292364,0.0,0.0,0.0,0.99776923656463623,0.0,0.0,0.0,-0.00090417405590415001,0.0,0.0,0.0,0.0040977406315505505,0.0,0.0,0.0,-0.00090637523680925369,0.0,0.0,0.0,0.99535828828811646,0.0,0.0,0.0],\"ModelTypeMask\": 16,\"Noise\": [0.00095000001601874828,0.00095000001601874828,0.00095000001601874828,0.0,0.0,0.0],\"Rt\": {\"Rotation\": [0.0028619086369872093,0.11397748440504074,-0.99347919225692749,-0.99998742341995239,-0.003759800223633647,-0.0033120021689683199,-0.0041127768345177174,0.99347621202468872,0.11396529525518417],\"Translation\": [0,0,0]},\"SecondOrderScaling\": [0,0,0,0,0,0,0,0,0],\"SensorType\": \"CALIBRATION_InertialSensorType_Gyro\",\"TemperatureBounds\": [5,60],\"TemperatureC\": 0},{\"BiasTemperatureModel\": [0.061099298298358917,0.0,0.0,0.0,0.1208026260137558,0.0,0.0,0.0,-0.136076420545578,0.0,0.0,0.0],\"BiasUncertainty\": [0.0099999997764825821,0.0099999997764825821,0.0099999997764825821],\"Id\": \"CALIBRATION_InertialSensorId_LSM6DSM\",\"MixingMatrixTemperatureModel\": [1.0172514915466309,0.0,0.0,0.0,-0.00027497464907355607,0.0,0.0,0.0,-0.0052592288702726364,0.0,0.0,0.0,-0.00027595480787567794,0.0,0.0,0.0,1.0136110782623291,0.0,0.0,0.0,0.00068440736504271626,0.0,0.0,0.0,-0.0053186626173555851,0.0,0.0,0.0,0.00068968330742791295,0.0,0.0,0.0,1.0058845281600952,0.0,0.0,0.0],\"ModelTypeMask\": 56,\"Noise\": [0.010700000450015068,0.010700000450015068,0.010700000450015068,0.0,0.0,0.0],\"Rt\": {\"Rotation\": [9.9685930763371289e-5,0.10628914088010788,-0.99433523416519165,-0.99999529123306274,-0.0030350952874869108,-0.00042468888568691909,-0.0030630421824753284,0.99433064460754395,0.10628833621740341],\"Translation\": [-0.050903361290693283,-0.050903361290693283,-0.050903361290693283]},\"SecondOrderScaling\": [0,0,0,0,0,0,0,0,0],\"SensorType\": \"CALIBRATION_InertialSensorType_Accelerometer\",\"TemperatureBounds\": [5,60],\"TemperatureC\": 0}],\"Metadata\": {\"SerialId\": \"CL2LC2P00EQ\",\"FactoryCalDate\": \"12/21/2018 8:25:23 AM GMT\",\"Version\": {\"Major\": 1,\"Minor\": 2},\"DeviceName\": \"Eden-EV2\",\"Notes\": \"PV0_max_radius_invalid\"}}}";
        k4a_calibration_t calibration;
        k4a_calibration_get_from_raw(json_raw, sizeof(json_raw), K4A_DEPTH_MODE_NFOV_2X2BINNED, K4A_COLOR_RESOLUTION_720P, &calibration);
        transformation = k4a_transformation_create(&calibration);
    };

    ~depthengine_context(){
        if(transformation!=nullptr){
            k4a_transformation_destroy(transformation);
        }
    }
private:
    k4a_transformation_t transformation;
};
void get_depthengine_context_instance();

