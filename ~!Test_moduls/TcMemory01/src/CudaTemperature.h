// CudaTemperature.h
#pragma once
#include <string>
#include <msgpack.hpp>

// Эта структура полностью соответствует C# record:
// [MessagePackObject]
// public record CudaTemperature([property: Key(0)] string Dt, [property: Key(1)] float Temp);
struct CudaTemperature {
  std::string Dt;
  float Temp;

  // Используем MSGPACK_DEFINE_ARRAY для сериализации в виде массива,
  // что соответствует атрибутам [Key(0)], [Key(1)] в C#.
  MSGPACK_DEFINE_ARRAY(Dt, Temp);
};
