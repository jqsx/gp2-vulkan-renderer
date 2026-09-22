//
// Created by frane on 2/13/2026.
//
#pragma once

#ifndef SINGLETON_H
#define SINGLETON_H

template<typename T>
class Singleton {

public:
    static T& GetInstance() {
        static T instance{};
        return instance;
    }

    virtual ~Singleton() = default;

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
    protected:
        Singleton() = default;
};

#endif //SINGLETON_H
