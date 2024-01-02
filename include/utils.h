// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#ifndef EVENTS_UTILS_H
#define EVENTS_UTILS_H

#include <functional>
#include <tuple>

template<typename F, typename Tuple,  std::size_t... I>
void apply_relaxed_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
  // The index_sequence is used to list the elements of the tuple that should be 
  // used for calling f.

  if constexpr(sizeof...(I) == 0) // if we are told to use none of the elements of the tuple
  {
    f(); // we simply call f without any argument.
  }
  // otherwise, we test wether f can be called with the elements referenced by the index_sequence...
  else if constexpr(std::is_invocable<F, std::tuple_element_t<I, Tuple>...>::value) 
  {
    // ... and if that's the case we call f with these elements.
    std::invoke(std::forward<F>(f), std::get<I>(std::forward<Tuple>(t))...);
  }
  // otherwise, we remove the last element of the index_sequence and perform a recursive call
  else
  {
    apply_relaxed_impl(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<sizeof...(I) - 1>());
  }
}

/**
 * @brief invoke a callable using arguments provided in a tuple-like object
 * @param f  the callable object
 * @param t  the arguments for the call (in a tuple-like object)
 * 
 * If more arguments than necessary to call @a f are provided in the tuple, 
 * extra arguments are ignored.
 * 
 * @sa std::apply().
 */
template<typename F, typename Tuple>
void apply_relaxed(F&& f, Tuple&& t)
{
  apply_relaxed_impl(std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<std::tuple_size_v<Tuple>>());
}

/**
 * @brief invoke a callable using the provided arguments
 * @param f     a callable object
 * @param args  the arguments to use (provided as a parameter pack)
 * 
 * If more arguments than necessary to call @a f are provided, extra arguments 
 * are ignored.
 * 
 * The implementation relies on apply_relaxed().
 * 
 * @sa std::invoke().
 */
template<typename F, typename ...Args>
void invoke_relaxed(F&& f, Args&&...args)
{
  apply_relaxed(std::forward<F>(f), std::forward_as_tuple(std::forward<Args>(args)...));
}

#endif // EVENTS_UTILS_H