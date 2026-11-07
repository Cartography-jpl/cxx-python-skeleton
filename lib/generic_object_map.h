#pragma once
#include "printable.h"
#include <memory>
#include <ranges>
#include <functional>
#include <string>
#include <string_view>
#include <map>
#include <vector>

namespace Skeleton {

/****************************************************************/ /**
  Boost serialization only maintains pointers for a single object
  being serialized. So for example we may have object 'a' and object
  'b'. Object 'b' contains a pointer to object 'a'. If we serialize
  object 'b' and object 'a' separately, and then read them back in,
  we end up with two copies of 'a' - one from the serialization of the
  original 'a' and one from 'b'. If instead, we have an object c that
  contains both 'a' and 'b', then there will be only one 'a', since
  boost in serializing c realized there are two pointers that point
  to the same object.

  As a help when we don't already have a object 'c', this map contains
  any arbitrary number of objects, allowing us maintain the
  relationships.

  This serves much the same purpose as "shelve" does for "pickle" in
  python.

  Thread safety: as for a standard library type: const member functions
  can be called from several threads at once; anything else needs
  exclusive access.
*******************************************************************/
class GenericObjectMap : public Printable<GenericObjectMap> {
public:
  // std::less<>: lookups take a std::string_view without making a
  // std::string.
  using map_type =
      std::map<std::string, std::shared_ptr<GenericObject>, std::less<>>;
  using iterator = map_type::iterator;
  using const_iterator = map_type::const_iterator;

  //-----------------------------------------------------------------------
  /// Print description of object.
  //-----------------------------------------------------------------------

  virtual void print(std::ostream& Os) const { Os << "GenericObjectMap"; }

  //-----------------------------------------------------------------------
  /// List of keys, for python
  //-----------------------------------------------------------------------

  std::vector<std::string> keys() const
  {
    return std::views::keys(map_) | std::ranges::to<std::vector>();
  }

  //-----------------------------------------------------------------------
  /// Save object - this is something more easy to call from python.
  //-----------------------------------------------------------------------

  void set_generic(const std::string& Key,
                   const std::shared_ptr<GenericObject>& V)
  {
    map_[Key] = V;
  }

  //-----------------------------------------------------------------------
  /// Return object if we have one, or a null pointer if there is no
  /// value in the map.
  //-----------------------------------------------------------------------

  [[nodiscard]] std::shared_ptr<GenericObject>
  get_generic(std::string_view Key) const
  {
    auto i = map_.find(Key);
    return i != map_.end() ? i->second : nullptr;
  }

  //-----------------------------------------------------------------------
  /// Return object if we have one, or a null pointer if there is no
  /// value in the map or it doesn't match type
  //-----------------------------------------------------------------------

  template<class T>
  [[nodiscard]] std::shared_ptr<T> get(std::string_view Key) const
  {
    return std::dynamic_pointer_cast<T>(get_generic(Key));
  }

  //-----------------------------------------------------------------------
  /// The usual map operations.
  //-----------------------------------------------------------------------

  std::shared_ptr<GenericObject>& operator[](const std::string& Key)
  {
    return map_[Key];
  }
  [[nodiscard]] bool contains(std::string_view Key) const
  {
    return map_.contains(Key);
  }
  bool erase(std::string_view Key)
  {
    auto i = map_.find(Key);
    if(i == map_.end())
      return false;
    map_.erase(i);
    return true;
  }
  std::size_t size() const { return map_.size(); }
  bool empty() const { return map_.empty(); }
  void clear() { map_.clear(); }
  iterator begin() { return map_.begin(); }
  iterator end() { return map_.end(); }
  const_iterator begin() const { return map_.begin(); }
  const_iterator end() const { return map_.end(); }
private:
  map_type map_;
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version);
};
}

SKELETON_EXPORT_KEY(GenericObjectMap);
