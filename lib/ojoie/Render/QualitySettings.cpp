//
// Created by aojoie on 5/18/2023.
//

#include "Render/QualitySettings.hpp"

namespace AN {


static void InitializeDefaultQualitySettings(std::vector<QualitySetting> &settings) {
    settings.resize(4);

    settings[0].name = "Low";
    settings[0].targetResolution = { 800, 600 };
    settings[0].vSyncCount = 1;
    settings[0].antiAliasing = 0;

    settings[1].name = "Medium";
    settings[1].targetResolution = { 1920, 1080 };
    settings[1].vSyncCount = 0;
    settings[1].antiAliasing = 2;

    settings[2].name = "High";
    settings[2].targetResolution = { 2048, 1080 };
    settings[2].vSyncCount = 0;
    settings[2].antiAliasing = 4;

    settings[3].name = "Ultra";
    settings[3].targetResolution = { 3840, 2160 };
    settings[3].vSyncCount = 0;
    settings[3].antiAliasing = 4;
}


QualitySettings::QualitySettings() {
    InitializeDefaultQualitySettings(_settings);
    _current = _settings.size() - 1;
}

void QualitySettings::checkConsistency() {

}

void QualitySettings::applyExpensive() {

}

void QualitySettings::setTargetResolution(const Size &targetResolution) {
    if (_settings[_current].targetResolution == targetResolution) return;

    _settings[_current].targetResolution = targetResolution;
    checkConsistency();
    applyExpensive();
}

void QualitySettings::setVSyncCount(UInt32 count) {
    if (_settings[_current].vSyncCount == count) return;

    _settings[_current].vSyncCount = count;
    checkConsistency();
    applyExpensive();
}

void QualitySettings::setAntiAliasing(UInt32 aa) {
    if (_settings[_current].antiAliasing == aa) return;

    _settings[_current].antiAliasing = aa;
    checkConsistency();
    applyExpensive();
}


QualitySettings &GetQualitySettings() {
    static QualitySettings qualitySettings{};
    return qualitySettings;
}

}