//
// Created by Aleudillonam on 8/27/2022.
//

#include <iostream>
#include <fstream>

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        std::cerr << "Expected file name and output file name" << endl;
        return -1;
    }

    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Open file" << argv[1] << " error" << endl;
        return -1;
    }

    std::ofstream outFile(argv[2]);

    if (!outFile.is_open()) {
        std::cerr << "Create file" << argv[2] << " error" << endl;
        return -1;
    }

    unsigned char byte;
    int lineCount = 0;
    while (file.read((char *)&byte, sizeof byte)) {
        if (file.eof()) {
            break;
        }
        outFile << "0x" << std::hex << (unsigned int)byte << ", ";
        ++lineCount;

        if (lineCount > 24) {
            lineCount = 0;
            outFile << '\n';
        }
    }

    return 0;
}