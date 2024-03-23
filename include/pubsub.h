// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef PUBSUB_H
#define PUBSUB_H

#include <algorithm>
#include <type_traits>
#include <vector>

template<typename T, typename Iter, typename... Params, typename... Args>
void notify_all(Iter begin, Iter end, void (T::*method)(Params...), Args &&...args)
{
  for (auto it = begin; it != end; ++it)
  {
    T* listener = *it;
    (listener->*method)(std::forward<Args>(args)...);
  }
}

template<typename T, typename... Params, typename... Args>
void notify_all(const std::vector<T*>& subscribers, void (T::*method)(Params...), Args &&...args)
{
  // for (T* listener : subscribers)
  // {
  //   (listener->*method)(std::forward<Args>(args)...);
  // }
  notify_all(subscribers.begin(), subscribers.end(), method, std::forward<Args>(args)...);
}

template<typename P>
class Subscriber;

template<typename S>
class Publisher
{
public:

  using subscriber_t = S;

  struct PubPtrSetter
  {
    void operator()(subscriber_t* s, Publisher<S>* p)
    {
      s->m_publisher = static_cast<typename subscriber_t::publisher_t*>(p);;
    }
  };

  ~Publisher()
  {
    std::for_each(m_subscribers.begin(), m_subscribers.end(), [](subscriber_t* s){
      PubPtrSetter()(s, nullptr);
    });
  }

  void addSubscriber(subscriber_t* sub)
  {
    auto it = find_subscriber(sub);

    if (it == m_subscribers.end())
    {
      m_subscribers.push_back(sub);
      PubPtrSetter()(sub, this);
    }
  }

  // void removeSubscriber(subscriber_t* sub)
  // {
  //   auto it = find_subscriber(sub);

  //   if (it != m_subscribers.end())
  //   {
  //     m_subscribers.erase(it);
  //     sub->m_publisher = nullptr;
  //   }
  // }

  template<typename P>
  void removeSubscriber(Subscriber<P>* sub)
  {
    static_assert(std::is_base_of<Subscriber<P>, subscriber_t>::value);

    auto it = find_subscriber(static_cast<subscriber_t*>(sub));

    if (it != m_subscribers.end())
    {
      m_subscribers.erase(it);
      PubPtrSetter()(static_cast<subscriber_t*>(sub), nullptr);
    }
  }

  template<typename... Params, typename... Args>
  void notify(void (subscriber_t::*method)(Params...), Args &&...args)
  {
    notify_all(m_subscribers, method, std::forward<Args>(args)...);
  }

  const std::vector<subscriber_t*>& subscribers() const
  {
    return m_subscribers;
  }

protected:
  typename std::vector<subscriber_t*>::iterator find_subscriber(subscriber_t* listener)
  {
    return std::find(m_subscribers.begin(), m_subscribers.end(), listener);
  }

private:
  std::vector<subscriber_t*> m_subscribers;
};

template<typename P>
class Subscriber
{
public:

  using publisher_t = P;

  virtual ~Subscriber()
  {
    if (m_publisher)
    {
      m_publisher->removeSubscriber(this);
    }
  }

  publisher_t* publisher() const
  {
    return m_publisher;
  }

private:
  friend typename P::PubPtrSetter;
  publisher_t* m_publisher = nullptr;
};

#endif // PUBSUB_H