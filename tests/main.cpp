// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "event-emitter.h"
#include "object.h"
#include "pubsub.h"

#include <iostream>

#define REQUIRE(x) test_require(x, #x, __FILE__, __LINE__)

void test_require(bool cond, const char* test, const char* file, int line)
{
  if (!cond)
  {
    std::cerr << "fail: " << file << ":" << line << ": " << test << std::endl;

    throw std::runtime_error("test fail!");
  }
}

void print_status_code(int status)
{
  std::cout << status << std::endl;
}

void test_invoke_relaxed()
{
  invoke_relaxed(&print_status_code, 200, "OK");

  int n = 0;
  invoke_relaxed([&n](int status) { n = status; }, 404, "Not found");
  REQUIRE(n == 404);
}

class Person : public EventEmitter
{
private:
  std::string m_name = "John Doe";
public:

  const std::string& name() const { return m_name; }

  void setName(std::string n)
  {
    if (n != m_name)
    {
      m_name = std::move(n);
      nameChanged(m_name);
    }
  }

  void nameChanged(const std::string& name)
  {
    emit(&Person::nameChanged, name);
  }
};

class MyClass : public EventEmitter
{
public:

  int n() const { return m_n; }

  void setN(int n)
  {
    if(m_n != n)
    {
        m_n = n;
        nChanged(n);
    }
  }

  void nChanged(int val)
  {
    emit(&MyClass::nChanged, val);
  }

  void setP(int p)
  {
    pChanged(p);
  }

  void pChanged(int val)
  {
    emit(&MyClass::pChanged, val);
  }

  void superEvent()
  {
    emit(&MyClass::superEvent);
  }

private:
  int m_n = 0;
};

void test_disconnect()
{
  // The goal of this test is to verify that a connection is 
  // effectively removed after a call to ConnectionHandle::disconnect().

  MyClass a;

  int m = 0;

  ConnectionHandle handle = a.on(&MyClass::nChanged, [&m](int n){
    m = n;
  });

  a.setN(3);

  REQUIRE(m == 3);

  handle.disconnect();

  a.setN(4);

  REQUIRE(m == 3 && a.n() == 4);
}

void test_two_events()
{
  // The goal of this test is to verify that multiple events with 
  // the same signature are supported.

  MyClass a;

  int n = 0;
  int p = 0;

  // if the following REQUIRE fails on MSVC, you may have to disable Identical COMDAT Folding (ICF)
  // with the /OPT:NOICF linker flag.
  // Reference: 
  // https://stackoverflow.com/questions/14176320/why-are-member-function-pointers-behaving-so-weirdly-in-visual-c
  REQUIRE(&MyClass::pChanged != &MyClass::nChanged);

  a.on(&MyClass::nChanged, [&n](int val){
    n = val;
  });

  a.on(&MyClass::pChanged, [&p](int val){
    p = val;
  });

  a.setN(4);
  REQUIRE(n == 4 && p == 0);

  a.setP(6);
  REQUIRE(n == 4 && p == 6);
}

void test_once()
{
  // The goal of this test is to verify that an event listener 
  // registered with once() is indeed called only once.
  
  MyClass a;

  int super = 0;

  a.once(&MyClass::superEvent, [&super](){
    ++super;
  });

  a.superEvent();
  a.superEvent();

  REQUIRE(super == 1);
}

void test_partial_args()
{
  // The goal of this test is to verify that an event listener 
  // is allowed to receive less argument than passed.
  
  struct PartialEE : EventEmitter
  {
    void twoArgs(int a, int b) {
      emit(&PartialEE::twoArgs, a, b);
    }
  };

  int total = 0;

  PartialEE ee;

  ee.on(&PartialEE::twoArgs, [&total](int a, int b){
    total += (a + b);
  });

  ee.on(&PartialEE::twoArgs, [&total](int a){
    total += a;
  });

  ee.twoArgs(1, 2);

  REQUIRE(total == (1+2+1));
}

class MySubscriber;

class MyPublisher : public Publisher<MySubscriber>
{
public:
  void greets();
  void haveLunch();
};

class MySubscriber : public Subscriber<MyPublisher>
{
public:
  explicit MySubscriber(MyPublisher* pub = nullptr);

  virtual void sayHello() = 0;
  virtual void eatIt(const std::string& meal) = 0;
};

inline MySubscriber::MySubscriber(MyPublisher* pub)
{
  if (pub)
  {
    pub->addSubscriber(this);
  }
}

void MyPublisher::greets()
{
  notify(&MySubscriber::sayHello);
}

void MyPublisher::haveLunch()
{
  notify(&MySubscriber::eatIt, "🥖");
  notify(&MySubscriber::eatIt, "🍻");
}

class FrenchSubscriber : public MySubscriber
{
public:
  using MySubscriber::MySubscriber;

  void sayHello() override
  {
    std::cout << "Bonjour!" << std::endl;
  }

  void eatIt(const std::string& meal)
  {
    if (meal == "🥖")
    {
      std::cout << "J'aime la baguette." << std::endl;
    }
    else 
    {
      std::cout << "Il manque le fromage..." << std::endl;
    }
  }
};

class GermanSubscriber : public MySubscriber
{
public:
  using MySubscriber::MySubscriber;

  void sayHello() override
  {
    std::cout << "Guten Tag!" << std::endl;
  }

  void eatIt(const std::string& meal)
  {
    if (meal == "🍻")
    {
      std::cout << "Zwei bier!" << std::endl;
    }
    else 
    {
      std::cout << "Ein kilogramm kartoffel, bitte!" << std::endl;
    }
  }
};

void test_pubsub()
{
  MyPublisher pub;
  FrenchSubscriber thefrench{ &pub };
  auto thegerman = std::make_unique<GermanSubscriber>(&pub);

  REQUIRE(pub.subscribers().size() == 2);
  pub.addSubscriber(thegerman.get()); // no-op
  REQUIRE(pub.subscribers().size() == 2);

  REQUIRE(thefrench.publisher() == &pub);
  REQUIRE((std::is_same<decltype(thefrench.publisher()), MyPublisher*>::value));

  pub.greets();
  pub.haveLunch();

  thegerman.reset();
  REQUIRE(pub.subscribers().size() == 1);
  pub.greets();

  pub.removeSubscriber(&thefrench);
  REQUIRE(pub.subscribers().empty());
  pub.greets();
}

class SpinBox : public Object
{
public:
  void valueChanged(int n)
  {
    emit(&SpinBox::valueChanged, n);
  }
};

void test_object()
{
  SpinBox this_is_me;

  int n = 0;

  Object::connect(&this_is_me, &SpinBox::valueChanged, [&n](int a) {
    n += a;
  });

  REQUIRE(n == 0);
  this_is_me.valueChanged(3);
  REQUIRE(n == 3);
}

class Button : public Object
{
public:
  void clicked()
  {
    emit(&Button::clicked);
  }
};

class Dialog : public Object
{
private:
  bool m_visible = false;
public:
  bool visible() const { return m_visible;}
  void open() { m_visible = true; opened(); }
  void opened() { emit(&Dialog::opened); }
};

void test_two_objects()
{
  Button mybutton;

  int nopen = 0;

  {
    Dialog dialog;
    
    Object::connect(&mybutton, &Button::clicked, &dialog, &Dialog::open);
    Object::connect(&dialog, &Dialog::opened, [&nopen]() {
      ++nopen;
    });

    REQUIRE(nopen == 0);
    mybutton.clicked();
    REQUIRE(nopen == 1);
  }

  mybutton.clicked();
  REQUIRE(nopen == 1);
}

void run()
{
  test_invoke_relaxed();
  test_disconnect();
  test_two_events();
  test_partial_args();
  test_once();
  test_pubsub();
  test_object();
  test_two_objects();
}

int main()
{
  Person p;
  p.on(&Person::nameChanged, [](std::string n) {
    std::cout << "Hello " << n << "!" << std::endl;
  });
  p.setName("Homer Simpson");

  try
  {
    run();
    std::cout << "I see this as an absolute win!" << std::endl;
  }
  catch(const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return  1;
  }
  
  return 0;
}
