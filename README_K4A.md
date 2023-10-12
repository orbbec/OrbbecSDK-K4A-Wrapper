# OrbbecSDK-K4A-Wrapper

Welcome to the OrbbecSDK-K4A-Wrapper! This branch is contain the K4A wrapper for OrbbecSDK. It's mean that user can use this library to develop the application with K4A API, but use to access the Orbbec camera. Also user can use this library to replace native K4A library in your application to access the Orbbec camera without any code change.

This repo is forked from Azure Kinect Sensor SDK

This branch is base on release/1.4.x, and use new impl code base on OrbbecSDK to replace the k4a implementation.

The usage of this library is same as Native K4A

**What we did?**

The [k4a.h](./include/k4a/k4a.h) is the header file of K4A API, and the source code in [k4a.c](./src/sdk/k4a.c) is the implementation of K4A API. We have reimplemented the K4A API in [ob_k4a_impl.c](./src/orbbec/ob_k4a_impl.c) with OrbbecSDK, and keep the same effect as the original K4A API. Therefore, all functions called on user's side will be redirected to the OrbbecSDK, and user can access the Orbbec camera like the K4A device.

![OrbbecSDK-K4A-Wrapper](src/orbbec/docs/resource/OrbbecSDK-K4A-Wrapper.png)


## Introduction

**OrbbecSDK-K4A-Wrapper** is a cross platform (Linux and Windows) user mode SDK to read data from your orbbec device.

**Support camera and platform:**  Orbbec Femto Mega: Windows10+, Ubuntu20.04+; x64 Orbbec Femto Bolt: Windows10+, Ubuntu18.04+; x64 Other Orbbec cameras and platforms without test, don't use it in production environment.


## Why use the OrbbecSDK-K4A-Wrapper

The OrbbecSDK-K4A-Wrapper enables you to get the most out of your orbbec camera. Features include:

* Depth camera access
* RGB camera access and control (e.g. exposure and white balance)
* Motion sensor (gyroscope and accelerometer) access
* Synchronized Depth-RGB camera streaming with configurable delay between cameras
* External device synchronization control with configurable delay offset between devices
* Camera frame meta-data access for image resolution, timestamp and temperature
* Device calibration data access

## Installation

To use the OrbbecSDK-K4A-Wrapper, please refer to the installation instructions in [usage](docs/usage.md)

Quick Instructions:

 * Linux script :

    ```bash
    cd src/orbbec/OrbbecSDK/misc/scripts
    sudo chmod +x ./install_udev_rules.sh
    ./install_udev_rules.sh
    ```


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

The Azure Kinect SDK uses semantic versioning, please see [versioning.md](docs/versioning.md) for more information.

## Testing

For information on writing or running tests, please see [testing.md](docs/testing.md)

## Contribute

We welcome your contributions! Please see the [contribution guidelines](CONTRIBUTING.md).

## Feedback

For OrbbecSDK-K4A-Wrapper feedback or to report a bug, please file a [GitHub Issue](https://github.com/orbbec/OrbbecSDK-K4A-Wrapper/issues). For general suggestions or ideas, visit our [feedback forum](https://developer.orbbec.com.cn/).

## Sample Code

Please see the [sample](https://github.com/orbbec/OrbbecSDK-K4A-Wrapper/tree/ob/dev/1.8.x/examples)

## Q&A

Welcome to the [Q&A](kinect-qa.md) corner!

## Join Our Developer Program

Complete your developer profile [here](https://developer.orbbec.com.cn/) to get connected with our Mixed Reality Developer Program. You will receive the latest on our developer tools, events, and early access offers.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Reporting Security Issues
Security issues and bugs should be reported privately, via email, to the administrators <[daiyin@orbbec.com](daiyin@orbbec.com)>.
You will receive a response as soon as possible. If for some reason you do not, please follow up via email to ensure we received your original message.

## License Support for OrbbecSDK-K4A-Wrapper

[MIT License](LICENSE)