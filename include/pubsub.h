// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef PUBSUB_H
#define PUBSUB_H

#include <algorithm>
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

template<typename P, typename S>
class Subscriber;

template<typename P, typename S>
class Publisher
{
public:

  using publisher_t = P;
  using subscriber_t = S;

  ~Publisher()
  {
    std::for_each(m_subscribers.begin(), m_subscribers.end(), [](subscriber_t* s){
      s->m_publisher = nullptr;
    });
  }

  void addSubscriber(subscriber_t* sub)
  {
    auto it = find_subscriber(sub);

    if (it == m_subscribers.end())
    {
      m_subscribers.push_back(sub);

      static_assert(std::is_same<typename subscriber_t::publisher_t, P>::value);
      sub->m_publisher = static_cast<typename subscriber_t::publisher_t*>(this);
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

  void removeSubscriber(Subscriber<P, S>* sub)
  {
    auto it = find_subscriber(static_cast<subscriber_t*>(sub));

    if (it != m_subscribers.end())
    {
      m_subscribers.erase(it);
      sub->m_publisher = nullptr;
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

template<typename P, typename S>
class Subscriber
{
public:

  using publisher_t = P;
  using subscriber_t = S;

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
  friend Publisher<P, S>;
  publisher_t* m_publisher = nullptr;
};

#endif // PUBSUB_H