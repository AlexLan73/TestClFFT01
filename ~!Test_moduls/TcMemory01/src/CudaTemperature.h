#pragma once
#include <string>
#include <msgpack.hpp> // Для макроса сериализации

struct CudaTemperature {
  std::string time;
  float temperature;

  // Этот макрос ОБЯЗАТЕЛЕН. Он сообщает msgpack, какие поля нужно сериализовать.
  // Порядок полей важен и должен совпадать с C#.
  MSGPACK_DEFINE(time, temperature);
};