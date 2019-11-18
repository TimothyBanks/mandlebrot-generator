//
//  messages.h
//  fractal
//
//  Created by Banks, Timothy on 10/28/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#ifndef messages_h
#define messages_h

#include <iostream>
#include <memory>

namespace Fractal
{

using float32 = float;
using float64 = double;

struct Message_header
{
  uint8_t type;
  std::string identifier;
};

template <typename T>
struct View
{
  T left;
  T top;
  T right;
  T bottom;
  
  View() = default;
  View(const View&) = default;
  View(View&&) = default;
  
  View& operator=(const View&) = default;
  View& operator=(View&&) = default;
  
  View(T left_, T top_, T right_, T bottom_)
  : left{std::move(left_)},
    top{std::move(top_)},
    right{std::move(right_)},
    bottom{std::move(bottom_)}
  {
  }
  
  T width() const
  {
    if (right > left)
    {
      return right - left;
    }
    return left - right;
  }
  
  T height() const
  {
    if (top > bottom)
    {
      return top - bottom;
    }
    return bottom - top;
  }
  
  T width_factor() const
  {
    return (right - left) / width();
  }
  
  T height_factor() const
  {
    return (bottom - top) / height();
  }
};

struct Geo_message_header
{
  Message_header header;
  View<size_t> device;
  View<float64> complex;
};

struct Request_message
{
 static constexpr uint8_t ID = 0;
 
 Geo_message_header header;
 uint16_t max_iterations;
};

struct Response_message
{
  static constexpr uint8_t ID = 1;
  
  Geo_message_header header;
  std::vector<uint32_t> argb_buffer;
};

struct Cancel_message
{
  static constexpr uint8_t ID = 2;
  
  Message_header header;
};

std::ostream& print(std::ostream& os, const Message_header& obj)
{
  os << "Type: " << std::to_string(obj.type) << std::endl;
  os << "Identifier: " << obj.identifier << std::endl;
  return os;
}

template <typename T>
std::ostream& print(std::ostream& os, const View<T>& obj)
{
  os << "Left: " << obj.left << std::endl;
  os << "Top: " << obj.top << std::endl;
  os << "Right: " << obj.right << std::endl;
  os << "Bottom: " << obj.bottom << std::endl;
  return os;
}

std::ostream& print(std::ostream& os, const Geo_message_header& obj)
{
  print(os, obj.header);
  os << "Device View: " << std::endl;
  print(os, obj.device);
  os << "Complex View: " << std::endl;
  print(os, obj.complex);
  return os;
}

std::ostream& print(std::ostream& os, const Request_message& obj)
{
  os << "Request Message" << std::endl;
  print(os, obj.header);
  os << "Max Iterations: " << obj.max_iterations << std::endl;
  return os;
}

std::ostream& print(std::ostream& os, const uint32_t* buffer, size_t width, size_t height)
{
  if (!buffer || width == 0 || height == 0)
  {
    return os;
  }
  
  os << "P3\n" << width << " " << height << " 255" << std::endl;
  for (int64_t i = height - 1; i >= 0; --i)
  {
    auto factor = i * width;
    for (size_t j = 0; j < width; ++j)
    {
      auto color = buffer[j + factor];
      os << ((color >> 16) & 0xFF) << " " << ((color >> 8) & 0xFF) << " " << (color & 0xFF) << "\n";
    }
  }
  return os;
}

std::ostream& print(std::ostream& os, const std::shared_ptr<uint32_t>& buffer, size_t width, size_t height)
{
  return print(os, buffer.get(), width, height);
}

std::ostream& print(std::ostream& os, const std::vector<uint32_t>& buffer, size_t width, size_t height)
{
  if (buffer.empty())
  {
    return os;
  }

  return print(os, &buffer[0], width, height);
}

std::ostream& print(std::ostream& os, const Response_message& obj)
{
  os << "Response Message" << std::endl;
  print(os, obj.header);
  print(os, obj.argb_buffer, obj.header.device.width(), obj.header.device.height());
  return os;
}

std::ostream& print(std::ostream& os, const Cancel_message& obj)
{
  os << "Cancel Message" << std::endl;
  print(os, obj.header);
  return os;
}

bool operator==(const Message_header& left, const Message_header& right)
{
  if (left.type != right.type)
  {
    return false;
  }
  
  if (left.identifier != right.identifier)
  {
    return false;
  }
  
  return true;
}

bool operator!=(const Message_header& left, const Message_header& right)
{
  return !(left == right);
}

template <typename T>
bool operator==(const View<T>& left, const View<T>& right)
{
  if (left.left != right.left)
  {
    return false;
  }
  
  if (left.top != right.top)
  {
    return false;
  }
  
  if (left.right != right.right)
  {
    return false;
  }
  
  if (left.bottom != right.bottom)
  {
    return false;
  }
  
  return true;
}

template <typename T>
bool operator!=(const View<T>& left, const View<T>& right)
{
  return !(left == right);
}

bool operator==(const Geo_message_header& left, const Geo_message_header& right)
{
  if (left.header != right.header)
  {
    return false;
  }
  
  if (left.device != right.device)
  {
    return false;
  }
  
  if (left.complex != right.complex)
  {
    return false;
  }
  
  return true;
}

bool operator!=(const Geo_message_header& left, const Geo_message_header& right)
{
  return !(left == right);
}

bool operator==(const Request_message& left, const Request_message& right)
{
  if (left.header != right.header)
  {
    return false;
  }
  
  if (left.max_iterations != right.max_iterations)
  {
    return false;
  }
  
  return true;
}

bool operator!=(const Request_message& left, const Request_message& right)
{
  return !(left == right);
}

bool operator==(const Response_message& left, const Response_message& right)
{
  if (left.header != right.header)
  {
    return false;
  }
  
  if (left.argb_buffer != right.argb_buffer)
  {
    return false;
  }
  
  return true;
}

bool operator!=(const Response_message& left, const Response_message& right)
{
  return !(left == right);
}

bool operator==(const Cancel_message& left, const Cancel_message& right)
{
  if (left.header != right.header)
  {
    return false;
  }
  
  return true;
}

bool operator!=(const Cancel_message& left, const Cancel_message& right)
{
  return !(left == right);
}

std::ostream& operator<<(std::ostream& os, const Message_header& obj)
{
  os << obj.type;
  os << std::endl;
  os << obj.identifier;
  os << std::endl;
  
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const View<T>& obj)
{
  os << obj.left;
  os << std::endl;
  os << obj.top;
  os << std::endl;
  os << obj.right;
  os << std::endl;
  os << obj.bottom;
  os << std::endl;
  
  return os;
}

std::ostream& operator<<(std::ostream& os, const Geo_message_header& obj)
{
  os << obj.header;
  os << obj.device;
  os << obj.complex;
  
  return os;
}

std::ostream& operator<<(std::ostream& os, const Request_message& obj)
{
  const_cast<Request_message&>(obj).header.header.type = Request_message::ID;
  os << obj.header;
  os << obj.max_iterations;
  os << std::endl;
  
  return os;
}

std::ostream& operator<<(std::ostream& os, const Response_message& obj)
{
  const_cast<Response_message&>(obj).header.header.type = Response_message::ID;
  os << obj.header;
  
  for (const auto& argb : obj.argb_buffer)
  {
    os << argb;
    os << std::endl;
  }
  
  return os;
}

std::ostream& operator<<(std::ostream& os, const Cancel_message& obj)
{
  const_cast<Cancel_message&>(obj).header.type = Cancel_message::ID;
  os << obj.header;
  
  return os;
}

std::istream& operator>>(std::istream& is, Message_header& obj)
{
  is >> obj.type;
  is >> obj.identifier;
  
  return is;
}

template <typename T>
std::istream& operator>>(std::istream& is, View<T>& obj)
{
  is >> obj.left;
  is >> obj.top;
  is >> obj.right;
  is >> obj.bottom;
  
  return is;
}

std::istream& operator>>(std::istream& is, Geo_message_header& obj)
{
  is >> obj.header;
  is >> obj.device;
  is >> obj.complex;
  
  return is;
}

std::istream& operator>>(std::istream& is, Request_message& obj)
{
  is >> obj.header;
  is >> obj.max_iterations;
  
  return is;
}

std::istream& operator>>(std::istream& is, Response_message& obj)
{
  is >> obj.header;
  
  auto width = obj.header.device.right - obj.header.device.left;
  auto height = obj.header.device.bottom - obj.header.device.top;
  auto size = width * height;
  obj.argb_buffer.resize(size, 0);
  for (size_t i = 0; i < size; ++i)
  {
    is >> obj.argb_buffer[i];
  }
  
  return is;
}

std::istream& operator>>(std::istream& is, Cancel_message& obj)
{
  is >> obj.header;
  
  return is;
}

}

#endif /* messages_h */
