
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_IOP_IOR__
#define __org_omg_IOP_IOR__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace org
  {
    namespace omg
    {
      namespace IOP
      {
          class IOR;
          class TaggedProfile;
      }
    }
  }
}

class org::omg::IOP::IOR : public ::java::lang::Object
{

public:
  IOR();
  IOR(::java::lang::String *, JArray< ::org::omg::IOP::TaggedProfile * > *);
private:
  static const jlong serialVersionUID = 1901439890645554948LL;
public:
  JArray< ::org::omg::IOP::TaggedProfile * > * __attribute__((aligned(__alignof__( ::java::lang::Object)))) profiles;
  ::java::lang::String * type_id;
  static ::java::lang::Class class$;
};

#endif // __org_omg_IOP_IOR__
