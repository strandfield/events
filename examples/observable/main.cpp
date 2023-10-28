// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include <algorithm>
#include <iostream>
#include <vector>

class Listener
{
public:
  virtual ~Listener() = default;

  virtual void messageA() = 0;
  virtual void messageB(const std::string& str) = 0;
};

class Observable
{
public:

  void addListener(Listener* listener)
  {
    auto it = find_listener(listener);

    if (it == m_listeners.end())
    {
      m_listeners.push_back(listener);
    }
  }

  void removeListener(Listener* listener)
  {
    auto it = find_listener(listener);

    if (it != m_listeners.end())
    {
      m_listeners.erase(it);
    }
  }

  void notifyA()
  {
    for(auto* listener : m_listeners)
    {
      listener->messageA();
    }
  }

  void notifyB(const std::string& str)
  {
    for(auto* listener : m_listeners)
    {
      listener->messageB(str);
    }
  }

protected:
  std::vector<Listener*>::iterator find_listener(Listener* listener)
  {
    return std::find(m_listeners.begin(), m_listeners.end(), listener);
  }

private:
  std::vector<Listener*> m_listeners;
};

class Listener1 : public Listener
{
public:
  void messageA()
  {
    std::cout << "1: A" << std::endl;
  }

  void messageB(const std::string& str)
  {
    std::cout << "1: B: " << str << std::endl;
  }
};

class Listener2 : public Listener
{
public:
  void messageA()
  {
    std::cout << "2: A" << std::endl;
  }

  void messageB(const std::string& str)
  {
    std::cout << "2: B: " << str << std::endl;
  }
};

int main()
{
  Observable obs;

  Listener1 l1;
  Listener2 l2;

  obs.addListener(&l1);
  obs.addListener(&l2);
  
  obs.notifyA();
  obs.notifyB("ploup");

  // easy to forget, but much safer if present
  obs.removeListener(&l1);
  obs.removeListener(&l2);

  return 0;
}
