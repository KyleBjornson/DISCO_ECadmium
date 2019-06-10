/**
 * Copyright (c) 2019, Kyle Bjornson, Ben Earle
 * Carleton University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SHT31_TEMP_HUMIDITY_DRIVER_HPP
#define SHT31_TEMP_HUMIDITY_DRIVER_HPP

#define SHT31_DEFAULT_ADDR          0x44
#define SHT31_MEAS_HIGHREP_STRETCH  0x2C06
#define SHT31_MEAS_MEDREP_STRETCH   0x2C0D
#define SHT31_MEAS_LOWREP_STRETCH   0x2C10
#define SHT31_MEAS_HIGHREP          0x2400
#define SHT31_MEAS_MEDREP           0x240B
#define SHT31_MEAS_LOWREP           0x2416
#define SHT31_READSTATUS            0xF32D
#define SHT31_CLEARSTATUS           0x3041
#define SHT31_SOFTRESET             0x30A2
#define SHT31_HEATEREN              0x306D
#define SHT31_HEATERDIS             0x3066

#include <mbed.h>
#include <cadmium/embedded/embedded_error.hpp>

namespace drivers {

    /**
     * @brief Driver for the SHT31 Temperature and Humidity Sensor
     */
    class sht31 {
        private:

            I2C _i2c;
            int _i2c_address;
            double _humidity, _temperature;

            void reset(void) {
                write_command(SHT31_SOFTRESET);
                wait_ms(10);
            }

            /**
             * @brief Check status bytes
             */
            uint16_t read_status(void) {
                char *val;
                uint16_t stat;

                write_command(SHT31_READSTATUS);

                //Read MSB
                _i2c.read(_i2c_address, val, 1);
                stat = *val << 8;

                //Read LSB
                _i2c.read(_i2c_address, val, 1);
                stat |= *val;

                return stat;
            }

            /**
             * @brief Wrapper to write 2 byte I2C commands
             */
            void write_command(uint16_t cmd) {
                char buf[2];
                buf[0] = (cmd >> 8);
                buf[1] = (cmd & 0xFF);
                _i2c.write(_i2c_address, buf, 2);
            }

            /**
             * @brief Read/save temperature and humidity values
             *        from sensor over I2C
             */
            bool read_temperature_humidity(void) {
                char readbuffer[6];
                uint16_t temperature_word, humidity_word;

                write_command(SHT31_MEAS_HIGHREP);
                wait_ms(50);

                //Read in temperature, humidity, and both CRCs
                _i2c.read(_i2c_address, readbuffer, 6);

                //copy temperature bytes to uint16_t
                temperature_word = (readbuffer[0] << 8) | readbuffer[1];

                //Check Temperature CRC
                if (readbuffer[2] != crc8((uint8_t *) readbuffer, 2)) {
                    return false;
                }

                //copy humidity bytes to uint16_t
                humidity_word = (readbuffer[3] << 8) | readbuffer[4];

                //Check Humidity CRC
                if (readbuffer[5] != crc8((uint8_t *) readbuffer+3, 2)) {
                    return false;
                }

                //Calculate and save real temperature value
                _temperature = -45 + ((((double) temperature_word) * 175) / 0xFFFF);

                //Calculate and save real humidity value
                _humidity = (((double) humidity_word) * 100) / 0xFFFF;

                return true;
            }

            /**
             * @brief Sensor uses 8 bit CRC to validate readings
             */
            uint8_t crc8(const uint8_t *data, int len) {
                const uint8_t POLYNOMIAL(0x31); // x^8 + x^5 + x^4 + 1
                uint8_t crc(0xFF);

                for ( int j = len; j; --j ) {
                    crc ^= *data++;

                    for ( int i = 8; i; --i ) {
                        crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
                    }
                }
                return crc;
            }


        public:

            /**
             * @brief Constructor to init I2C, and verifiy that the
             *        sensor is connected.
             */
             sht31(PinName sda, PinName scl) : _i2c(sda, scl) {
                 _i2c_address = (SHT31_DEFAULT_ADDR << 1);
                 reset();
                 read_status();
             }

             /**
              * @brief Read sensor data and return temperature value
              */
             double read_temperature(void) {
                 if (! read_temperature_humidity()) {
                     cadmium::embedded::embedded_error::hard_fault("SHT31 TEMPERATURE CRC FAIL");
                 }
                 return _temperature;
             }

             /**
              * @brief Read sensor data and return humidity value
              */
             double read_humidity(void) {
                 if (! read_temperature_humidity()) {
                     cadmium::embedded::embedded_error::hard_fault("SHT31 HUMIDITY CRC FAIL");
                 }
                 return _humidity;
             }

    };
}

#endif //SHT31_TEMP_HUMIDITY_DRIVER_HPP
