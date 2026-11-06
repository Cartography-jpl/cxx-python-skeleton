#ifndef PYTHON_EXCEPTION_H
#define PYTHON_EXCEPTION_H
#include <stdexcept>
#include <string>

class PythonException : public std::runtime_error
{
public:
  PythonException();
  PythonException(const PythonException& other);
  PythonException& operator=(const PythonException& other);
  ~PythonException() override;
  const char* what() const noexcept override;
  void restore_python_exception() const;
private:
  mutable void *type, *value, *tb;
  std::string desc;
};
     
#endif
