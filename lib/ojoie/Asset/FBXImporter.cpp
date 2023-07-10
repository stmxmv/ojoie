//
// Created by Aleudillonam on 5/2/2023.
//

#include "Asset/FBXImporter.hpp"

#include "Modules/Dylib.hpp"

namespace AN {

typedef FBXImporterImpl *(*PFN_ANCreateFBXImporter)(void);
typedef void (*PFN_ANDeleteFBXImporter)(FBXImporterImpl *importer);

static PFN_ANCreateFBXImporter ANCreateFBXImporter;
static PFN_ANDeleteFBXImporter ANDeleteFBXImporter;

FBXImporter::FBXImporter() {
    impl = nullptr;
    if (ANCreateFBXImporter == nullptr) {
        if (LoadAndLookupSymbols("FBXImporter",
                                 "ANCreateFBXImporter", &ANCreateFBXImporter,
                                 "ANDeleteFBXImporter", &ANDeleteFBXImporter, nullptr)) {
        }
    }

    if (ANCreateFBXImporter != nullptr) {
        impl = ANCreateFBXImporter();
    }
}

FBXImporter::~FBXImporter() {
    if (impl) {
        ANDeleteFBXImporter(impl);
        impl = nullptr;
    }
}


}