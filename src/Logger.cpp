//
// Created by frane on 2/13/2026.
//

#include "Logger.h"

#include <iostream>

void Logger::log(const std::string &str) {
    std::cout << "[INFO] " << str << std::endl;
}

void Logger::err(const std::string &str) {
    std::cerr << "[ERR] " << str << std::endl;
}

void Logger::err(const std::string &str, int line, const std::string &file) {
    std::cerr << "[ERR] [" << file << "] (" << std::to_string(line) << ") " << str << std::endl;
}