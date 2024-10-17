## Change Log

# release v2.0.3
1.Adapt to OrbbecSDK-V2.0.16;

# release v2.0.2
1.Adapt to OrbbecSDK-V2.0.10;
fix the exception that occurs when attempting to start a device while it has not been stopped.

# release v1.10.3
1.Adapt to mega-i;
2.Fix the playback issue of K4aViewer;
3.Correct the IMU data of Femto Bolt.

# release v1.10.2
1.adjust imu queue default size;
2.adapt to image related Interface;
3.remove excess compilation items;
4.palyback pre-init ob_context;
5.adjust the order of IMU starting;
6.adapt to mega-i device;
7.update orbbecsdk version to last.

# release v1.10.1
1. update orbbecsdk version to v1.10.5.

# release v1.9.4
1. update orbbecsdk version to v1.10.2;
2. support green_screen sample;
3. video frame timestamps for Femto Bolt modify microseconds (us).（Femto Bolt firmware version Need 1.1.1 or above）.

### v1.9.3
1. fix multi device sync bug;
2. support for obtaining stride.

### v1.9.2
1. switch OrbbecSDK version to v1.9.5;
2. add k4a_device_switch_device_clock_sync_mode interface;
3. add k4a_device_get_wired_sync_mode interface;
4. add k4a_device_enable_soft_filter interface;
5. k4aviewer support indicator switch;
6. fix other bugs.

### v1.9.1
| **products list** | **firmware version** |**platform**|
| --- | --- | --- |
| Orbbec Femto Bolt  | 1.0.6/1.0.9  |Windows10+, Ubuntu18.04+ |
| Orbbec Femto Mega  | 1.1.5/1.1.7  |Windows10+, Ubuntu20.04+ |

1. Switch OrbbecSDK version to v1.9.4;
2. Support pre initialization of depthengine;
3. Support nv12;
4. Adjust the imu data to be consistent with AKDK;
5. Support for obaining imu extrinsic parameter data;
6. Add color hdr control in color control command;
7. Fix system timestamp setting error;
8. Adapt indicator light switch;
9. Shut unsupport resolutions by k4aviewer;
10. Filter out device not supported by k4aviewer;
11. Filter out null device by k4aviewer;
12. Turn off prompts that do not support audio by k4aviewer;
13. Fix log display error by k4aviewer.

### v1.8.3
1. Fix memory leakage issue;
2. Fix the issue of failed reopening after abnormal shutdown;
3. Other bug fixes.

### v1.8.1
support Femto Bolt and Femto Mega
| **products list** | **firmware version** |**platform**|
| --- | --- | --- |
| Orbbec Femto Bolt  | 1.0.6  |Windows10+, Ubuntu18.04+ |
| Orbbec Femto Mega  | 1.1.5/1.1.7  |Windows10+, Ubuntu20.04+ |