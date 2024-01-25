// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "k4aviewer.h"

#include <algorithm>
#include <string>
#include <cctype>

K4AViewerArgs ProcessArgs(int argc, char **argv);

K4AViewerArgs ProcessArgs(int argc, char **argv)
{
    K4AViewerArgs args;

    // Skip argv[0], which is the path to the executable
    //
    for (int i = 1; i < argc; i++)
    {
        // Force to uppercase
        //
        std::string arg = argv[i];
        std::transform(arg.begin(), arg.end(), arg.begin(), [](unsigned char c) {
            return static_cast<unsigned char>(std::toupper(c));
        });

        if (arg == "-HIGHDPI")
        {
            args.HighDpi = true;
        }
        else if (arg == "-NORMALDPI")
        {
            args.HighDpi = false;
        }
    }

    return args;
}

int main(int argc, char **argv)
{
    // add this to avoid failure during depth engine init when multi opengl context using on user app and inside SDK!
    k4a::depth_engine_helper::create();
    k4aviewer::K4AViewer viewer(ProcessArgs(argc, argv));
    viewer.Run();
    return 0;
}
