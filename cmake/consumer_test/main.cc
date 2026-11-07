// Uses the installed skeleton headers and library: writes an object to a
// string and reads it back.
#include "generic_object_sample.h"
#include "serialize_function.h"
#include <iostream>

int main()
{
  auto a = std::make_shared<Skeleton::GenericObjectA>(7);
  auto a2 = Skeleton::serialize_read_string<Skeleton::GenericObjectA>(
      Skeleton::serialize_write_string(a));
  std::cout << "consumer: " << *a2 << " " << a2->v() << "\n";
  return a2->v() == 7 ? 0 : 1;
}
