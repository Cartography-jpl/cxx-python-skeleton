#include "weak_ptr_serialize_support.h"
#include <set>
#include <vector>

using namespace CXX_PYTHON_RULES_NAMESPACE;

namespace {
// The mark sets of this thread, one per PtrSerializeScope, innermost
// last. The first one is used outside any scope and is never removed.
std::vector<std::set<const GenericObject*>>& mark_stack()
{
  thread_local std::vector<std::set<const GenericObject*>> s(1);
  return s;
}

std::set<const GenericObject*>& marks()
{
  return mark_stack().back();
}
}

void CXX_PYTHON_RULES_NAMESPACE::clear_ptr_serialized_reference()
{
  marks().clear();
}

void CXX_PYTHON_RULES_NAMESPACE::add_ptr_serialized_reference(
    const GenericObject* P)
{
  marks().insert(P);
}

bool CXX_PYTHON_RULES_NAMESPACE::is_ptr_serialized(const GenericObject* P)
{
  return marks().contains(P);
}

CXX_PYTHON_RULES_NAMESPACE::PtrSerializeScope::PtrSerializeScope()
{
  mark_stack().emplace_back();
}

CXX_PYTHON_RULES_NAMESPACE::PtrSerializeScope::~PtrSerializeScope()
{
  mark_stack().pop_back();
}
