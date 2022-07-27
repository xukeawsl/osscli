#include "parser.h"

#include <iostream>

int main(int argc, char* argv[]) {
    OssParser parser;
    if (!parser.ParseConfigFile("../config.json")) {
        std::cout << "Config File Error!" << std::endl;
        return EXIT_FAILURE;
    }

    auto executor = parser.ParseCommandLine(argc, argv);
    if (!executor) {
        std::cout << "Command Error!" << std::endl;
        return EXIT_FAILURE;
    } 
    
    executor->Execute();
    
    return EXIT_SUCCESS;
}