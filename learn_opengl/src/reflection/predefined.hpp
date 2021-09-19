#pragma once

#include "reflection/reflection.hpp"

namespace reflection {

template <>
struct TypeReflector<std::uint8_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "uint_8";
    handle->guid = "D0B2F2EB-7E9A-456A-A778-8077C1387B4E";
  }
};

template <>
struct TypeReflector<std::int8_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "int_8";
    handle->guid = "4DF46671-3707-4ECB-B51A-93D66AFC1325";
  }
};

template <>
struct TypeReflector<std::uint16_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "uint_16";
    handle->guid = "58E41EEA-BA36-42C0-8C84-A26D92F17AD3";
  }
};

template <>
struct TypeReflector<std::int16_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "int_16";
    handle->guid = "E67DB104-5C7F-46F4-953F-14AEE4456407";
  }
};

template <>
struct TypeReflector<std::uint32_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "uint_32";
    handle->guid = "41DBBCDA-A278-4808-9317-B654F9CCCA67";
  }
};

template <>
struct TypeReflector<std::int32_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "int_32";
    handle->guid = "05C16A40-9BC3-47D8-95C5-9545F09B2B20";
  }
};

template <>
struct TypeReflector<std::uint64_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "uint_64";
    handle->guid = "65D542A5-12EC-4079-B24D-AB4FD0DB84CA";
  }
};

template <>
struct TypeReflector<std::int64_t> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "int_64";
    handle->guid = "28D310C8-C966-42FF-8C5D-145947826729";
  }
};

template <>
struct TypeReflector<float> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "float";
    handle->guid = "9B07D939-1F2B-4CBF-89B0-0864CCFCE6A1";
  }
};

template <>
struct TypeReflector<double> {
  static void ReflectType(TypeHandle handle) {
    handle->name = "double";
    handle->guid = "889196C1-8E17-4993-A812-3076F9D19094";
  }
};

}