// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef EVENT_EMITTER_H
#define EVENT_EMITTER_H

#include "utils.h"

#include <algorithm>
#include <any>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace details
{
  class AbstractEventListener 
  {
  public:
    explicit AbstractEventListener(int i)
      : id(i)
      {

      }

    AbstractEventListener(const AbstractEventListener&) = delete;
    virtual ~AbstractEventListener() {}

    virtual bool matches(const std::any& signal) const = 0;
    
    int id;
    bool once_flag = false;
  };

  template<typename... Args>
  class EventListener : public AbstractEventListener
  {
  public:
    using AbstractEventListener::AbstractEventListener;
    virtual void invoke(Args... args) = 0;
  };

  template<typename...Args>
  class FreeFunctionEventListener : public EventListener<Args...>
  {
  public:
    FreeFunctionEventListener(int i, void* fp, std::function<void(Args...)> callback)
      : EventListener<Args...>(i)
      , m_fn(fp)
      , m_callback(std::move(callback))
    {
    }
    FreeFunctionEventListener(const FreeFunctionEventListener&) = delete;
    FreeFunctionEventListener& operator=(const FreeFunctionEventListener&) = delete;
    ~FreeFunctionEventListener(){}

    bool matches(const std::any& signal) const override
    {
      return std::any_cast<void*>(signal) == m_fn;
    }

    void invoke(Args... args) override
    {
      try {
        m_callback(std::forward<Args>(args)...);
      } catch (...) {
      }
    }

  private:
    void* m_fn;
    std::function<void(Args...)> m_callback;
  };

  template<typename Class, typename Callback, typename...Args>
  class MemberFunctionEventListener : public EventListener<Args...>
  {
  public:
    using MemFnPtr = void(Class::*)(Args...);

    MemberFunctionEventListener(int i, MemFnPtr event, Callback callback)
      : EventListener<Args...>(i)
      , m_signal(event)
      , m_callback(std::move(callback))
    {
    }
    MemberFunctionEventListener(const MemberFunctionEventListener&) = delete;
    MemberFunctionEventListener& operator=(const MemberFunctionEventListener&) = delete;
    ~MemberFunctionEventListener(){}

    bool matches(const std::any& signal) const override
    {
      return std::any_cast<MemFnPtr>(signal) == m_signal;
    }

    void invoke(Args... args) override
    {
      try {
        invoke_relaxed(m_callback, std::forward<Args>(args)...);
      } catch (...) {
      }
    }

  private:
    MemFnPtr m_signal;
    Callback m_callback;
  };

} // namespace details

class EventEmitter;

/**
 * \brief POD struct containing information about a connection
 */
struct ConnectionData
{
  /**
   * \brief pointer to the EventEmitter
   */
  EventEmitter* emitter = nullptr;

  /**
   * \brief id of the connection
   */
  int connection_id = 0;
};

/**
 * \brief a class providing a mean to emit events that can be listened to 
 * 
 * Events are emitted using the emit() member function.
 * No prior registration of events is necessary. 
 * Any EventEmitter can emit any type of events.
 * Events are identified by a pointer to a member function.
 * 
 * Registering a listener is done by calling on() or once().
 * A ConnectionHandle can be constructed from the return value of these functions 
 * and can be used to manage the lifetime of the connection.
 */
class EventEmitter
{
public:
  /**
   * \brief constructs an event emitter 
   */
  EventEmitter()
  {
    m_lifecontrol = std::make_shared<int>(1);
  }

  /**
   * \brief add an event listener
   * \param event     a pointer to a member function identifying the event
   * \param callback  a function that is called when the event is emitted
   * 
   * The parameter list of the callback must match the one of the event.
   * 
   * The new listener is added at the end of the list of listeners, meaning that it will 
   * be called after all other listeners to \a event previously registered.
   * 
   * \note \a callback can be any callable object that can be stored in a std::function.
   */
  template<typename T, typename F, typename... Args>
  ConnectionData on(void (T::*event)(Args...), F&& callback)
  {
    int id = ++m_id_generator;
    auto* listener = new details::MemberFunctionEventListener<T, F, Args...>(m_id_generator, event, std::forward<F>(callback));
    m_listeners.push_back(std::unique_ptr<details::AbstractEventListener>(listener));
    return {this, id};
  }

  /**
   * \brief add an event listener that will only be invoked once 
   * \param event     a pointer to a member function identifying the event
   * \param callback  a function that is called when the event is emitted
   * 
   * This is the same as on(), except that the listener will be automatically removed after
   * its first invokation (if it ever happens). 
   */
  template<typename T, typename F, typename... Args>
  ConnectionData once(void (T::*event)(Args...), F &&callback)
  {
    int id = ++m_id_generator;
    auto* listener = new details::MemberFunctionEventListener<T, F, Args...>(m_id_generator, event, std::forward<F>(callback));
    m_listeners.push_back(std::unique_ptr<details::AbstractEventListener>(listener));
    listener->once_flag = true;
    return {this, id};
  }

  /**
   * \brief fires an event
   * \param event  a pointer to a member function identifying the event
   * \param args   event data (template pack)
   * 
   * The arguments in \a args must have types that are compatible with a call
   * to the \a event function.
   */
  template<typename T, typename... Params, typename... Args>
  void emit(void (T::*event)(Params...), Args &&...args)
  {
    for (auto it = m_listeners.begin(); it != m_listeners.end();)
    {
      ListenerPtr &l = *it;

      if (l->matches(event))
      {
        auto *ll = static_cast<details::EventListener<Params...> *>(l.get());
        ll->invoke(std::forward<Args>(args)...);

        if (l->once_flag)
        {
          it = m_listeners.erase(it);
        }
        else
        {
          ++it;
        }
      }
      else
      {
        ++it;
      }
    }
  }

  /**
   * \brief removes a listener previously registered using on() or once()
   * \param connection_id  the id of the connection
   * \return whether a connection was actually removed 
   */
  bool removeListener(int connection_id)
  {
    auto it = std::lower_bound(m_listeners.begin(), m_listeners.end(), connection_id, [](const ListenerPtr &ptr, int cid)
                               { return ptr->id < cid; });

    if (it != m_listeners.end() && (*it)->id == connection_id)
    {
      m_listeners.erase(it);
      return true;
    }
    else
    {
      return false;
    }
  }

protected:
  friend class ConnectionHandle;

  /**
   * \brief internal shared pointer used to track the lifetime of the EventEmitter
   * 
   * This class holds the only strong ref to data, meaning that it is destroyed when 
   * the EventEmitter is destroyed.
   * A ConnectionHandle holds a weak pointer to the data and can therefore test if 
   * the EventEmitter is alive by calling lock() on the weak pointer.
   */
  std::shared_ptr<int> m_lifecontrol;
  
private:
  /**
   * \brief member variable used to generate listener ids
  */
  int m_id_generator = 0;

  using ListenerPtr = std::unique_ptr<details::AbstractEventListener>;

  /**
   * \brief vector containing all the listeners
   * 
   * The vector is sorted by increasing connection id.
   */
  std::vector<ListenerPtr> m_listeners;
};

/**
 * \brief class for managing a connection to an EventEmitter
 * 
 * This RAII class manages a connection created with EventEmitter::on() or 
 * EventEmitter::once().
 * The connection is automatically removed when the handle is destroyed 
 * (unless the link was broken using release()).
 * 
 * Connection handles are not copyable but are movable.
 */
class ConnectionHandle
{
public:
  ConnectionHandle() = default;
  
  ConnectionHandle(ConnectionData data) 
    : m_emitter(data.emitter)
    , m_connection_id(data.connection_id)
  {
    if (m_emitter)
      m_live = m_emitter->m_lifecontrol;
  }

  ConnectionHandle(const ConnectionHandle&) = delete;
  
  ConnectionHandle(ConnectionHandle&& other)
    : m_emitter(other.m_emitter)
    , m_live(other.m_live)
    , m_connection_id(other.m_connection_id)
  {
    other.release();
  }
  
  ~ConnectionHandle() {
    disconnect();
  }

  /**
   * \brief returns whether the handle refers to a valid connection to an event emitter 
   */
  bool isValid() const
  {
    return m_emitter && m_live.lock() != nullptr;
  }

  /**
   * \brief returns a pointer to the event emitter that holds the connection
   * 
   * \note this function either returns nullptr or a valid pointer to an EventEmitter, 
   * it won't return a non-null pointer to an already destroyed EventEmitter.
   */
  EventEmitter* eventEmitter() const
  {
    return isValid() ? m_emitter : nullptr;
  }

  /**
   * \brief returns the id of the connection 
   * 
   * The value 0 is used to indicate an invalid connection.
   */
  int connectionId() const
  {
    return m_connection_id;
  }

  /**
   * \brief breaks the link to the connection
   * \return the id of the connection
   * 
   * \note the connection itself remains active, it just will no longer be managed 
   * by this handle.
   */
  int release()
  {
    m_emitter = nullptr;
    m_live.reset();
    return std::exchange(m_connection_id, 0);
  }

  /**
   * \brief destroys the connection 
   */
  void disconnect()
  {
    if (isValid()) {
      m_emitter->removeListener(m_connection_id);
      release();
    }
  }

  ConnectionHandle& operator=(const ConnectionHandle&) = delete;

  ConnectionHandle& operator=(ConnectionHandle&& other)
  {
    disconnect();

    m_emitter = other.m_emitter;
    m_live = other.m_live;
    m_connection_id = other.m_connection_id;

    other.release();

    return *this;
  }

private:
  EventEmitter* m_emitter = nullptr;
  std::weak_ptr<int> m_live;
  int m_connection_id = 0;
};

#endif // EVENT_EMITTER_H