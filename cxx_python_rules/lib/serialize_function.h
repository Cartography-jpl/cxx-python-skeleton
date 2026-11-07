#pragma once
#include "generic_object.h"
#include <string>
#include <memory>

namespace CXX_PYTHON_RULES_NAMESPACE {
bool skip_weak_ptr_handling();
void skip_weak_ptr_handling(bool Skip);
bool have_serialize_supported();
void serialize_write(const std::string& Fname,
                     const std::shared_ptr<GenericObject>& Obj);
void serialize_write_binary(const std::string& Fname,
                            const std::shared_ptr<GenericObject>& Obj);

std::string serialize_write_string(const std::shared_ptr<GenericObject>& Obj);

std::shared_ptr<GenericObject> serialize_read_generic(const std::string& Fname);
std::shared_ptr<GenericObject>
serialize_read_binary_generic(const std::string& Fname);

std::shared_ptr<GenericObject>
serialize_read_generic_string(const std::string& Data);

std::string serialize_write_binary(const std::shared_ptr<GenericObject>& Obj);
std::shared_ptr<GenericObject> serialize_read_binary(const std::string& Data);

template<class T>
inline std::shared_ptr<T> serialize_read(const std::string& Fname)
{
  return std::dynamic_pointer_cast<T>(serialize_read_generic(Fname));
}

template<class T>
inline std::shared_ptr<T> serialize_read_binary(const std::string& Fname)
{
  return std::dynamic_pointer_cast<T>(serialize_read_binary_generic(Fname));
}

template<class T>
inline std::shared_ptr<T> serialize_read_string(const std::string& Data)
{
  return std::dynamic_pointer_cast<T>(serialize_read_generic_string(Data));
}

template<class T>
inline std::shared_ptr<T> serialize_read_binary_string(const std::string& Data)
{
  return std::dynamic_pointer_cast<T>(serialize_read_binary(Data));
}
}
