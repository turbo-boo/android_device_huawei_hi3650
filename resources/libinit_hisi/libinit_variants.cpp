/*
 * Copyright (C) 2023-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "libinit_variants"
#include <libinit_utils.h>
#include <libinit_variants.h>
#include <android-base/logging.h>
#include <fstream>

using namespace std;

constexpr const char* kBoardInfoPath = "/sys/firmware/devicetree/base/hisi,boardname";

string ReadModelInfo() {

    ifstream fd;
    string buf;
    
    fd.open(kBoardInfoPath);
    if (!fd.is_open()) {
        LOG(ERROR) << "Unable to open: " << kBoardInfoPath << ", error: " << strerror(errno);
        return buf;
    }

    getline(fd, buf);
    fd.close();

    return buf;
}

void load_variants() {
    string model_info = ReadModelInfo();

    // Load the phone model dynamically from devicetree.
    if (!model_info.empty()) {
        LOG(INFO) << "Found model info: " << model_info;
        if (model_info.find("BTV_DL09") != string::npos)
        	set_ro_build_prop("model", "BTV-DL09", true);
        else if (model_info.find("BTV_W09") != string::npos)
        	set_ro_build_prop("model", "BTV-W09", true);
        else if (model_info.find("BTV_L0J") != string::npos)
        	set_ro_build_prop("model", "BTV-L0J", true);
    } else {
        LOG(ERROR) << "Unable to parse model information!";
    }
}
