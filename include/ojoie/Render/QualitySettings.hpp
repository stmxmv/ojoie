//
// Created by aojoie on 5/18/2023.
//

#ifndef OJOIE_QUALITYSETTINGS_HPP
#define OJOIE_QUALITYSETTINGS_HPP

#include <ojoie/Core/Name.hpp>
#include <ojoie/Core/CGTypes.hpp>
#include <ojoie/Render/RenderTypes.hpp>
#include <vector>

namespace AN {


struct QualitySetting {
    Name name;
    Size targetResolution;
    UInt32 vSyncCount;
    UInt32 antiAliasing;
};

class AN_API QualitySettings {

    int _current;
    std::vector<QualitySetting> _settings;

    void checkConsistency();

    /// for now does not support runtime change
    void applyExpensive();

public:

    QualitySettings();

    const QualitySetting &getCurrent() { return _settings[_current]; }

    void setTargetResolution(const Size &targetResolution);
    void setVSyncCount(UInt32 count);
    void setAntiAliasing(UInt32 aa);

};

AN_API QualitySettings &GetQualitySettings();

}

#endif//OJOIE_QUALITYSETTINGS_HPP
