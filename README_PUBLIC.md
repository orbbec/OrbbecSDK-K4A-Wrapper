# release v1.9.2
## Change List
1. Switch OrbbecSDK version to v1.9.5;
2. add k4a_switch_device_clock_sync_mode interface;
3. add k4a_get_device_sync_mode interface;
4. add k4a_set_soft_filter interface;
5. k4aviewer support indicator switch;
6. fix vulnerabilities;

## Product support
| **products list** | **firmware version** |**platform**|
| --- | --- | --- |
| Orbbec Femto  Bolt| 1.0.6/1.0.9  |Windows10+, Ubuntu18.04+ |
| Orbbec Femto Mega  | 1.1.5/1.1.7  |Windows10+, Ubuntu20.04+ |
## Catalog Introduction
- /
  - bin : Executable files and dynamic loading libraries
  - doc : Guidelines for accessing AKDK Application Software with Femto Bolt
  - include : software interface
  - lib : Library files
  - scripts : Script for obtaining device timestamps on the Windows platform (Linux : Essential scripts for running programs)
## How to seamlessly replace the Azure Kinect camera with the Femto camera?
https://orbbec.github.io/OrbbecSDK-K4A-Wrapper/src/orbbec/docs/Access_AKDK_Application_Software_with_Femto_Bolt.pdf
## Q&A

1. The library of this branch is not support the K4A device, please use the [Native K4A](https://github.com/microsoft/Azure-Kinect-Sensor-SDK) library to access the K4A device.

2. The Orbbec SDK K4A Wrapper is aim to provide the same API as the K4A, but it's not full API for Orbbec SDK and feature for Orbbec camera. If you want to use the full feature of Orbbec camera, please use the [Orbbec SDK](https://github.com/orbbec/OrbbecSDK) directly.

3. For Linux user, there may be an issue with the initialization of DepthEngine when using Orbbec Femto Bolt due to modifications made by Microsoft in the new version of DepthEngine. This can cause failure during the start of the depth stream. The reason for this is that simultaneous use of multiple OpenGL contexts may result in conflicts. User can try to resolve it follow this: [https://www.khronos.org/opengl/wiki/OpenGL_and_multithreading](https://www.khronos.org/opengl/wiki/OpenGL_and_multithreading)

   For example：

    ``` c++
    // file: tools/k4aviewer/k4adevicedockcontrol.cpp
    GLFWwindow *currentContext = glfwGetCurrentContext(); // store the current context
    glfwMakeContextCurrent(NULL);  // make current context to NULL

    StartCameras(); //  will initialize the DepthEngine

    glfwMakeContextCurrent(currentContext); // restore the current context
    ```

4. Unable to obtain the device timestamp on the Windows platform
``` powershell
# Running as Administrator using PowerShell
cd src/orbbec/OrbbecSDK/misc/scripts
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
.\obsensor_metadata_win10.ps1 -op install_all
```

5. How to execute an application on the Linux platform without using sudo.
Install udev rules file：

 ``` bash
 cd src/orbbec/OrbbecSDK/misc/scripts
 sudo chmod +x ./install_udev_rules.sh
 ./install_udev_rules.sh
 # Once complete, the orbbec camera is available without being 'root'.
 ```
