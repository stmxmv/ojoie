//
// Created by Aleudillonam on 8/23/2022.
//

#include "Core/Configuration.hpp"

namespace AN {


Configuration &Configuration::GetSharedConfiguration() {
    static Configuration configuration;
    return configuration;
}

}