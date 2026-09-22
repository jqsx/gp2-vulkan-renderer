//
// Created by frane on 2/13/2026.
//

#ifndef LOGGER_H
#define LOGGER_H
#include <string>

#include "Singleton.h"

class Logger final : public Singleton<Logger> {
public:
    void log(const std::string& str);
    void err(const std::string& str);
    void err(const std::string& str, int line, const std::string& file);
};

#endif //LOGGER_H
