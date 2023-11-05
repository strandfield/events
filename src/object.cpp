// Copyright (C) 2023 Vincent Chambrin
// This file is part of the 'events' project.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "object.h"

Object::~Object()
{
  m_connection_list.clear();
}
