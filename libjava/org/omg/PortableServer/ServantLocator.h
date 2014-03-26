
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __org_omg_PortableServer_ServantLocator__
#define __org_omg_PortableServer_ServantLocator__

#pragma interface

#include <java/lang/Object.h>
#include <gcj/array.h>

extern "Java"
{
  namespace org
  {
    namespace omg
    {
      namespace CORBA
      {
          class Context;
          class ContextList;
          class DomainManager;
          class ExceptionList;
          class NVList;
          class NamedValue;
          class Object;
          class Policy;
          class Request;
          class SetOverrideType;
      }
      namespace PortableServer
      {
          class POA;
          class Servant;
          class ServantLocator;
        namespace ServantLocatorPackage
        {
            class CookieHolder;
        }
      }
    }
  }
}

class org::omg::PortableServer::ServantLocator : public ::java::lang::Object
{

public:
  virtual ::org::omg::CORBA::Request * _create_request(::org::omg::CORBA::Context *, ::java::lang::String *, ::org::omg::CORBA::NVList *, ::org::omg::CORBA::NamedValue *) = 0;
  virtual ::org::omg::CORBA::Request * _create_request(::org::omg::CORBA::Context *, ::java::lang::String *, ::org::omg::CORBA::NVList *, ::org::omg::CORBA::NamedValue *, ::org::omg::CORBA::ExceptionList *, ::org::omg::CORBA::ContextList *) = 0;
  virtual ::org::omg::CORBA::Object * _duplicate() = 0;
  virtual JArray< ::org::omg::CORBA::DomainManager * > * _get_domain_managers() = 0;
  virtual ::org::omg::CORBA::Object * _get_interface_def() = 0;
  virtual ::org::omg::CORBA::Policy * _get_policy(jint) = 0;
  virtual jint _hash(jint) = 0;
  virtual jboolean _is_a(::java::lang::String *) = 0;
  virtual jboolean _is_equivalent(::org::omg::CORBA::Object *) = 0;
  virtual jboolean _non_existent() = 0;
  virtual void _release() = 0;
  virtual ::org::omg::CORBA::Request * _request(::java::lang::String *) = 0;
  virtual ::org::omg::CORBA::Object * _set_policy_override(JArray< ::org::omg::CORBA::Policy * > *, ::org::omg::CORBA::SetOverrideType *) = 0;
  virtual ::org::omg::PortableServer::Servant * preinvoke(JArray< jbyte > *, ::org::omg::PortableServer::POA *, ::java::lang::String *, ::org::omg::PortableServer::ServantLocatorPackage::CookieHolder *) = 0;
  virtual void postinvoke(JArray< jbyte > *, ::org::omg::PortableServer::POA *, ::java::lang::String *, ::java::lang::Object *, ::org::omg::PortableServer::Servant *) = 0;
  static ::java::lang::Class class$;
} __attribute__ ((java_interface));

#endif // __org_omg_PortableServer_ServantLocator__
