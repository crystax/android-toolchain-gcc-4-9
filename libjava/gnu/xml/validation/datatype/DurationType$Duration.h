
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __gnu_xml_validation_datatype_DurationType$Duration__
#define __gnu_xml_validation_datatype_DurationType$Duration__

#pragma interface

#include <java/lang/Object.h>
extern "Java"
{
  namespace gnu
  {
    namespace xml
    {
      namespace validation
      {
        namespace datatype
        {
            class DurationType$Duration;
        }
      }
    }
  }
}

class gnu::xml::validation::datatype::DurationType$Duration : public ::java::lang::Object
{

public: // actually package-private
  DurationType$Duration();
public:
  virtual jint hashCode();
  virtual jboolean equals(::java::lang::Object *);
  virtual jint compareTo(::java::lang::Object *);
public: // actually package-private
  jint __attribute__((aligned(__alignof__( ::java::lang::Object)))) years;
  jint months;
  jint days;
  jint minutes;
  jfloat seconds;
public:
  static ::java::lang::Class class$;
};

#endif // __gnu_xml_validation_datatype_DurationType$Duration__
