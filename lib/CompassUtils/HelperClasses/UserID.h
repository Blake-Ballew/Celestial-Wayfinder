#pragma once

#include <Arduino.h>

class UserID
{
public:
    byte id[32];

    UserID()
    {
        this->generateRandomID();
    }

    UserID(byte id[32])
    {
        memcpy(this->id, id, 32);
    }

    UserID(const UserID &other)
    {
        memcpy(this->id, other.id, 32);
    }

    bool operator==(const UserID &other) const
    {
        return memcmp(id, other.id, 32) == 0;
    }

    bool operator==(const byte other[32]) const
    {
        return memcmp(id, other, 32) == 0;
    }

    bool operator==(const char *other) const
    {
        return memcmp(id, other, 32) == 0;
    }

    UserID operator=(const UserID &other) const
    {
        UserID tmp;
        memcpy((void *)tmp.id, other.id, 32);
        return tmp;
    }

    UserID operator=(const byte other[32]) const
    {
        UserID tmp;
        memcpy((void *)tmp.id, other, 32);
        return tmp;
    }

    UserID operator=(const char *other) const
    {
        UserID tmp;
        memcpy((void *)tmp.id, other, 32);
        return tmp;
    }

    bool isEmpty()
    {
        for (int i = 0; i < 32; i++)
        {
            if (id[i] != 0)
            {
                return false;
            }
        }
        return true;
    }

    void generateRandomID()
    {
        esp_fill_random(id, 32);
    }
};