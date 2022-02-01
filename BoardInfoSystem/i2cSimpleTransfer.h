#include <Arduino.h>
#include <Wire.h>

template <typename T> int i2cSimpleWrite (const T& value) {
    Wire.write((byte *) &value, sizeof (value));
    return sizeof (value);
}

template <typename T> int i2cSimpleRead(T& value) {
    byte * p = (byte*) &value;
    int i;
    for (i = 0; i < sizeof value; i++)
        *p++ = Wire.read();
    return i;
}