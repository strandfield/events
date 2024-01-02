
# `events`

Collection of experiments on "events" in C++.

## What's in there ?

### Basic implementation of the Observer design pattern

The folder `examples/observable` contains a small program featuring a basic 
implementation of the Observer design pattern.

For more information of this design pattern, see the [Observer](https://refactoring.guru/design-patterns/observer) 
page on [refactoring.guru](https://refactoring.guru).

### A more generic implementation with a Publisher / Subscriber system

The `pubsub.h` header file defines two class templates, `Publisher` and `Subscriber`, 
that can be used to quickly implement an Observer design pattern.

Theses classes can help reduce code duplication when the design pattern is implemented 
at multiple locations in the code base.
They also offer a more systematic lifecycle management with automatic unsubscribing 
when either the publisher or the subscriber is destroyed.

Definition of both a "publisher" and a "subscriber" class is facilitated.

Publisher:

```cpp
class MyPublisher : public Publisher<MyPublisher, MySubscriber>
{
public:
  void greets();
};
```

Subscriber:

```cpp
class MySubscriber : public Subscriber<MyPublisher, MySubscriber>
{
public:
  explicit MySubscriber(MyPublisher* pub = nullptr);

  virtual void sayHello() = 0;
};
```

Definition of `greets()`:

```cpp
void MyPublisher::greets()
{
  notify(&MySubscriber::sayHello);
}
```

Usage (assuming `GermanSubscriber` is derived from `MySubscriber`):

```cpp
MyPublisher pub;
GermanSubscriber sub;
pub.addSubscriber(&sub);
pub.greets(); // prints "Guten tag!", probably
```

### EventEmitter

An `EventEmitter` class inspired by [Node.js class EventEmitter](https://nodejs.org/api/events.html#class-eventemitter).

The idea here is to allow emitting and listening to events without prior event registration.
While the above methods require the definition of various classes describing the sets 
of events that can happen, the `EventEmitter` can be used as-is and supports any event.
The reduces the amount of code that needs to be written ! 😄 

The class can be used both using inheritance or as a member of a class.

Example (as a member):

```cpp
class Person
{
public:
  EventEmitter events;
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
    events.emit(&Person::nameChanged, name);
  }
};
```

Usage:

```cpp
int main() {
  Person p;
  p.events.on(&Person::nameChanged, [](std::string n) {
    std::cout << "Hello " << n << "!" << std::endl;
  });
  p.setName("Homer Simpson");
}
```

The `EventEmitter` class supports partial use of the signal's parameters, so the following
also works:

```cpp
Person p;
p.on(&Person::nameChanged, [&p](/* std::string */) {
  std::cout << "Hello " << p.name() << "!" << std::endl;
});
p.setName("Homer Simpson");
```

### Limitations & disclaimers

**Thread-safety:** 🧶

Code is NOT thread-safe ; was designed to be used in a single-threaded environment. 

## Building the code

Requirements:
- a compiler with C++17 support
- CMake

**Step-by-step build instructions:**

Create a build directory:

```bash
mkdir build && cd build
```

Generate the project:

```bash
cmake ..
```

**Build (linux):**

```bash
make
```

**Build (Windows):**

```bash
cmake --build . --config Release --target ALL_BUILD
```
