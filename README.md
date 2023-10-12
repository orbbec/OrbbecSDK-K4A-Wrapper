# Orbbec SDK K4A Wrapper

Welcome to the Orbbec SDK K4A Wrapper! 

*This repo is forked from [Azure Kinect Sensor SDK](https://github.com/microsoft/Azure-Kinect-Sensor-SDK)*

*This branch is base on release/1.4.x, and use new impl code base on [OrbbecSDK](https://github.com/orbbec/OrbbecSDK) to replace the k4a implementation.*

*The usage of this library is same as [Native K4A](./README_K4A.md)*

## Introduction

**Wrapper** is contain the K4A wrapper for Orbbec SDK. It's mean that user can use this library to develop the application with K4A API, but use to access the Orbbec camera. Also user can use this library to replace native K4A library in your application to access the Orbbec camera without any code change.


**What we did?**

The [k4a.h](./include/k4a/k4a.h) is the header file of K4A API, and the source code in [k4a.c](./src/sdk/k4a.c) is the implementation of K4A API. We have reimplemented the K4A API in [ob_k4a_impl.c](./src/orbbec/ob_k4a_impl.c) with Orbbec SDK, and keep the same effect as the original K4A API. Therefore, all functions called on user's side will be redirected to the Orbbec SDK, and user can access the Orbbec camera like the K4A device.

![Orbbec SDK K4A Wrapper](src/orbbec/docs/resource/OrbbecSDK-K4A-Wrapper.png)


## Why use the Orbbec SDK K4A Wrapper

The Wrapper enables you to get the most out of your orbbec camera. Features include:

* Depth camera access
* RGB camera access and control (e.g. exposure and white balance)
* Motion sensor (gyroscope and accelerometer) access
* Synchronized Depth-RGB camera streaming with configurable delay between cameras
* External device synchronization control with configurable delay offset between devices
* Camera frame meta-data access for image resolution, timestamp and temperature
* Device calibration data access

## Installation
* Windows Device Setup

On Windows, once attached, the device should automatically enumerate and load
all drivers.

* Linux Device Setup

On Linux, once attached, the device should automatically enumerate and load
all drivers. However, in order to use the Orbbec SDK K4A Wrapper with the device and without
being 'root', you will need to setup udev rules. We have these rules checked
into this repo under 'scripts/99-k4a.rules'. To do so:

* Copy 'scripts/99-k4a.rules' into '/etc/udev/rules.d/'.
* Detach and reattach Azure Kinect devices if attached during this process.

Once complete, the orbbec camera is available without being 'root'.

## Documentation

API documentation is avaliable [here](https://orbbec.github.io/docs/OrbbecSDK_K4A_Wrapper/bolt-1.7.x-dev/).

## Building

It is the same as Native K4A, please refer to:[building](https://github.com/orbbec/OrbbecSDK-K4A-Wrapper/blob/ob/dev/1.8.x/docs/building.md)

Quick Instructions:

* Windows:
    * Ninja:

    ```powershell
    cd OrbbecSDK-K4A-Wrapper
    mkdir build
    cd build
    cmake .. -G Ninja
    ninja
    ninja install
    ```

    * cmake:

    ```powershell
    cd OrbbecSDK-K4A-Wrapper
    mkdir build
    cd build
    cmake ../
    cmake --build .
    cmake --install .
    ```

* Linux

    * Linux dependencds:

        ```bash
        sudo apt update
        sudo apt install -y \
            pkg-config \
            ninja-build \
            doxygen \
            clang \
            gcc-multilib \
            g++-multilib \
            python3 \
            nasm

        sudo apt install -y \
            libgl1-mesa-dev \
            libsoundio-dev \
            libvulkan-dev \
            libx11-dev \
            libxcursor-dev \
            libxinerama-dev \
            libxrandr-dev \
            libusb-1.0-0-dev \
            libssl-dev \
            libudev-dev \
            mesa-common-dev \
            uuid-dev
        ```

    * Linux building:
        * ninja:
        
        ```bash
        cd OrbbecSDK-K4A-Wrapper
        mkdir build && cd build
        sudo cmake .. -G Ninja
        sudo ninja
        sudo ninja install
        ```

        * cmake

        ```bash
        cd OrbbecSDK-K4A-Wrapper
        mkdir build && cd build
        sudo cmake ..
        sudo make -j8
        sudo make install
        ```

## Versioning

| **products list** | **firmware version** |**platform**|
| --- | --- | --- |
| Femto Bolt          | 1.1.7                     |Windows10+, Ubuntu20.04+ |
| Femto Mega          | 1.0.6                     |Windows10+, Ubuntu18.04+ |


## Testing

For information on writing or running tests, please see [testing.md](docs/testing.md)

## Contribute
TODO

## Feedback

For Orbbec SDK K4A Wrapper feedback or to report a bug, please file a [GitHub Issue](https://github.com/orbbec/OrbbecSDK-K4A-Wrapper/issues). For general suggestions or ideas, visit our [feedback forum](https://3dclub.orbbec3d.com/).

## Sample Code


There are several places where the sample code can be found.

- In this repository: [OrbbecSDK-K4A-Wrapper\examples](https://github.com/orbbec/OrbbecSDK-K4A-Wrapper/tree/ob/dev/1.8.x/examples)- each example has a readme page that describes it and the steps to set it up.
- [Azure-Kinect-Samples](https://github.com/microsoft/Azure-Kinect-Samples) repository. There are multiple examples of how to use both Sensor and Body tracking SDKs.

## Q&A
TODO

## Join Our Developer Program
Complete your developer profile [here](https://3dclub.orbbec3d.com/) to get connected with our Mixed Reality Developer Program. 

## Code of Conduct
TODO

## Reporting Security Issues
Security issues and bugs should be reported privately, via email, to the
administrators at <[it@orbbec.com](mailto:it@orbbec.com)>.
You should receive a response within 24 hours. If for some reason you do not, please follow up via
email to ensure we received your original message. 
## License Support for Orbbec SDK K4A Wrapper
[MIT License](LICENSE)