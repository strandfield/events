// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef OBJECT_H
#define OBJECT_H

#include  "event-emitter.h"

/**
 * @brief a class providing an event-listening mechanism similar to Qt signal/slot system
 * 
 * Subclasses can emit signals by declaring them as member functions and using emit() 
 * for emitting the signal.
 * Connections are made using the various overloads of the connect() function.
 * 
 * Currently the system is single-threaded: slot invocation is performed immediately 
 * in the thread that emitted the signal.
 */
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
  EventEmitter m_events; // stores the connections where this object is the source of the signal and handles signal emission.
  std::vector<ConnectionHandle> m_connection_list; // stores the connection in which this object receives the notification.
                                                   // this is used to automatically break the connections when this object
                                                   // is destroyed.
};

/**
 * @brief emits a signal
 * @param event  a pointer to a member function representing the signal
 * @param args   the signal arguments
 * 
 * This immediately invokes (in the current thread) the slots connected to this signal.
 */
template<typename T, typename... Params, typename... Args>
inline void Object::emit(void (T::*event)(Params...), Args &&...args)
{
  m_events.emit(event, std::forward<Args>(args)...);
}

/**
 * @brief connects a signal to a callback function
 * @param srcObject  the object that may emit the signal
 * @param event      a pointer to a member function representing the signal
 * @param callback   a function to call when the signal is emitted
 * 
 * @warning the connection will remain active until @a srcObject is destroyed so 
 * be careful about the lifetime of @a callback.
 */
template<typename SrcT, typename SigObjT, typename F, typename... Args>
inline void Object::connect(SrcT* srcObject, void (SigObjT::*event)(Args...), F&& callback)
{
  static_assert(std::is_base_of_v<Object, SrcT>, "Source object must be derived from Object");
  auto* src = static_cast<Object*>(srcObject);
  src->m_events.on(event, std::forward<F>(callback));
}

/**
 * @brief connects a signal to a callback function
 * @param srcObject      the object that may emit the signal
 * @param event          a pointer to a member function representing the signal
 * @param contextObject  a context object used to manage the lifetime of the connection
 * @param callable       a function to call when the signal is emitted
 * 
 * @note unlike the previous overload in which the connection remains active until @a srcObject
 * is destroyed, this overload takes an additional @a contextObject. The connection is broken 
 * when either of these objects is destroyed.
 */
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

/**
 * @brief connects a signal from an object to the slot of another
 * @param srcObject   the object that may emit the signal
 * @param event       a pointer to a member function representing the signal
 * @param destObject  the object which slot will be invoked
 * @param slotFunc    a pointer to the slot (member function)
 * 
 * The connection is broken when either of the objects is destroyed.
 */
template <typename SrcT, typename DestT, typename SigObjT, typename... SrcArgs, typename... DestArgs>
inline void Object::connect(SrcT *srcObject, void (SigObjT::*event)(SrcArgs...), DestT *destObject, void (DestT::*slotFunc)(DestArgs...))
{
  Object::connect(srcObject, event, destObject, [destObject, slotFunc](DestArgs... data)
                                             { std::invoke(slotFunc, *destObject, std::forward<DestArgs>(data)...); });
}

#endif // OBJECT_H