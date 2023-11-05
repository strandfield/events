// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef OBJECT_H
#define OBJECT_H

#include  "event-emitter.h"

class Object
{
public:
  virtual ~Object();

  template<typename SrcT, typename SigObjT, typename F, typename... Args>
  static void connect(SrcT* srcObject, void (SigObjT::*event)(Args...), F&& callback);

  template<typename SrcT, typename ContextT, typename SigObjT, typename F, typename... SrcArgs>
  static void connect(SrcT* srcObject, void (SigObjT::*event)(SrcArgs...), ContextT* contextObject, F&& callable);

  template<typename SrcT, typename DestT, typename SigObjT, typename... SrcArgs, typename... DestArgs>
  static void connect(SrcT* srcObject, void (SigObjT::*event)(SrcArgs...), DestT* destObject, void (DestT::*slotFunc)(DestArgs...));

  template<typename T, typename... Params, typename... Args>
  void emit(void (T::*event)(Params...), Args &&...args);

private:
  EventEmitter m_events;
  std::vector<ConnectionHandle> m_connection_list;
};

template<typename T, typename... Params, typename... Args>
inline void Object::emit(void (T::*event)(Params...), Args &&...args)
{
  m_events.emit(event, std::forward<Args>(args)...);
}

template<typename SrcT, typename SigObjT, typename F, typename... Args>
inline void Object::connect(SrcT* srcObject, void (SigObjT::*event)(Args...), F&& callback)
{
  static_assert(std::is_base_of_v<Object, SrcT>, "Source object must be derived from Object");
  auto* src = static_cast<Object*>(srcObject);
  src->m_events.on(event, std::forward<F>(callback));
}

template<typename SrcT, typename ContextT, typename SigObjT, typename F, typename... SrcArgs>
inline void Object::connect(SrcT* srcObject, void (SigObjT::*event)(SrcArgs...), ContextT* contextObject, F&& callable)
{
  static_assert(std::is_base_of_v<Object, SrcT>, "Source object must be derived from Object");
  static_assert(std::is_base_of_v<Object, ContextT>, "Context object must be derived from Object");
  auto *src = static_cast<Object *>(srcObject);
  auto *context = static_cast<Object *>(contextObject);
  ConnectionHandle handle = src->m_events.on(event, std::forward<F>(callable));
  context->m_connection_list.push_back(std::move(handle));
}

template <typename SrcT, typename DestT, typename SigObjT, typename... SrcArgs, typename... DestArgs>
inline void Object::connect(SrcT *srcObject, void (SigObjT::*event)(SrcArgs...), DestT *destObject, void (DestT::*slotFunc)(DestArgs...))
{
  /*
  static_assert(std::is_base_of_v<Object, SrcT>, "Source object must be derived from Object");
  static_assert(std::is_base_of_v<Object, DestT>, "Destination object must be derived from Object");
  auto *src = static_cast<Object *>(srcObject);
  auto *dest = static_cast<Object *>(destObject);
  ConnectionHandle handle = src->m_events.on(event, [destObject, slotFunc](DestArgs... data)
                                             { std::invoke(slotFunc, *destObject, std::forward<DestArgs>(data)...); });
  dest->m_connection_list.push_back(std::move(handle));
  */

  Object::connect(srcObject, event, destObject, [destObject, slotFunc](DestArgs... data)
                                             { std::invoke(slotFunc, *destObject, std::forward<DestArgs>(data)...); });
}

#endif // OBJECT_H