#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <greengrasssdk.h>
#include "messages.h"

struct Subscribers
{
  static std::vector<std::string> slaves;
  static std::string master;
};

std::vector<std::string> Subscribers::slaves;
std::string Subscribers::master;

template <typename T>
void print(const T& message)
{
    auto ss = std::stringstream{};
    Fractal::print(ss, message);
    gg_log(GG_LOG_DEBUG, ss.str().c_str());
}

template <typename T>
void print(const T& t, const std::string& message, gg_log_level log_level);

template <>
void print<std::vector<int8_t>>(const std::vector<int8_t>& buffer, const std::string& message, gg_log_level log_level)
{
  if (buffer.empty())
  {
    auto error_message = "[MESSAGE] " + message;
    gg_log(log_level, error_message.c_str());
    return;
  }

  auto error_message = "[MESSAGE]\n" + message + "\n" + std::string{std::begin(buffer), std::end(buffer)};
  gg_log(log_level, error_message.c_str());
}

template <>
void print<gg_error>(const gg_error& error, const std::string& message, gg_log_level log_level)
{
  auto error_code = std::string{};

  switch (error)
  {
    case GGE_SUCCESS:
    {
      error_code = "[SUCCESS]";
    } break;
    case GGE_OUT_OF_MEMORY:
    {
      error_code = "[OUT OF MEMORY]";
    } break;
    case GGE_INVALID_PARAMETER:
    {
      error_code = "[INVALID PARAMETER]";
    } break;
    case GGE_INVALID_STATE:
    {
      error_code = "[INVALID STATE]";
    } break;
    case GGE_INTERNAL_FAILURE:
    {
      error_code = "[INTERNAL FAILURE]";
    } break;
  }

  error_code += " " + message;
  gg_log(log_level, error_code.c_str());
}

template <>
void print<gg_request_result>(const gg_request_result& result, const std::string& message, gg_log_level log_level)
{
  auto result_message = std::string{};

  switch (result.request_status)
  {
    case GG_REQUEST_SUCCESS:
    {
      result_message = "[REQUEST SUCCESS]";
    } break; 
    case GG_REQUEST_HANDLED:
    {
      result_message = "[REQUEST HANDLED]";
    } break; 
    case GG_REQUEST_UNHANDLED:
    {
      result_message = "[REQUEST UNHANDLED]";
    } break; 
    case GG_REQUEST_UNKNOWN:
    {
      result_message = "[REQUEST UNKNOWN]";
    } break; 
    case GG_REQUEST_AGAIN:
    {
      result_message = "[REQUEST AGAIN]";
    } break; 
  }

  result_message += " " + message;
  gg_log(log_level, result_message.c_str());
}

class GG_request_ptr final
{
public:
  GG_request_ptr()
  {
    auto error = gg_request_init(&m_request);
    if (error != GGE_SUCCESS)
    {
      m_valid = false;
      print(error, "Failure initializing gg_request.", GG_LOG_DEBUG);
    }
  }
  GG_request_ptr(const GG_request_ptr&) = delete;
  GG_request_ptr(GG_request_ptr&& other) 
  : m_request{other.m_request}
  {
    other.m_valid = false;
  }
  ~GG_request_ptr()
  {
    if (m_valid)
    {
      gg_request_close(m_request);
    }
  }

  GG_request_ptr& operator=(const GG_request_ptr&) = delete;
  GG_request_ptr& operator=(GG_request_ptr&& other)
  {
    if (this == &other)
    {
      return *this;
    }

    m_request = other.m_request;
    other.m_valid = false;
  }

  operator gg_request() const 
  {
    return m_request;
  }

  operator gg_request*()
  {
    return &m_request;
  }

  operator const gg_request*() const
  {
    return &m_request;
  }
private:
  gg_request m_request;
  bool m_valid{true};
};

void lambda_callback(const gg_lambda_context* context)
{
  if (!context)
  {
    return;
  }

  auto read = [&](std::vector<int8_t>& buffer, size_t& bytes_read)
  {
    auto amount_read = size_t{0};
    auto current_index = size_t{0};

    while (true)
    {
      auto error = gg_lambda_handler_read(&buffer[0] + current_index, buffer.size() - current_index, &amount_read);

      if (error != GGE_SUCCESS)
      {
        print(error, "Failure while invoking gg_lambda_handler_read", GG_LOG_DEBUG);
        return error;
      }

      current_index += amount_read;

      if (amount_read == 0)
      {
        // Finished reading
        break;
      }
    }

    return GGE_SUCCESS;
  };

  auto handle_request_message = [](GG_request_ptr& request, const std::vector<int8_t>& buffer)
  {
    if (buffer.empty())
    {
      return;
    }

    auto ss = std::stringstream{std::string{std::begin(buffer), std::end(buffer)}};
    auto message = Fractal::Request_message{};
    ss >> message;
    print(message);

    auto device_step = ((message.header.device.bottom + message.header.device.top) / 2) / Subscribers::slaves.size();
    auto complex_step = ((message.header.complex.bottom + message.header.complex.top) / 2.0) / Subscribers::slaves.size();

    // Tile the request bounding box and send the requests onto the slaves
    // Each slave will be given a chunk of rows in the resulting image.
    // The slaves may further tile the requested area.
    for (size_t i = 0; i < Subscribers::slaves.size(); ++i)
    {
      auto new_message = message;  // Force a copy
      new_message.header.device.top = message.header.device.top + (i * device_step);
      new_message.header.device.bottom = new_message.header.device.top + device_step;
      new_message.header.complex.top = message.header.complex.top + (i * complex_step);
      new_message.header.complex.bottom = new_message.header.complex.top + complex_step;

      const auto& slave = Subscribers::slaves[i];
      auto request_result = gg_request_result{};
      print(gg_publish(request, 
                       slave.c_str(), 
                       reinterpret_cast<const void*>(&new_message),
                       sizeof(Fractal::Request_message), 
                       &request_result),
            " Sending Request request to slaves.",
            GG_LOG_DEBUG);
      print(request_result, "Result of cancel forwarding.", GG_LOG_DEBUG);
    }
  };

  auto handle_cancel_message = [](GG_request_ptr& request, const std::vector<int8_t>& buffer)
  {
    if (buffer.empty())
    {
      return;
    }

    auto ss = std::stringstream{std::string{std::begin(buffer), std::end(buffer)}};
    auto message = Fractal::Cancel_message{};
    ss >> message;
    print(message);
    
    // Just forward the message onto the slaves.
    for (const auto& slave : Subscribers::slaves)
    {
      auto request_result = gg_request_result{};
      print(gg_publish(request, 
                       slave.c_str(), 
                       reinterpret_cast<const void*>(&buffer[0]),
                       buffer.size(), 
                       &request_result),
            " Forwarding cancel request to slaves.",
            GG_LOG_DEBUG);
      print(request_result, "Result of cancel forwarding.", GG_LOG_DEBUG);
    }
  };

  auto handle_response_message = [](GG_request_ptr& request, const std::vector<int8_t>& buffer)
  {
    if (buffer.empty())
    {
      return;
    }    
    
    auto ss = std::stringstream{std::string{std::begin(buffer), std::end(buffer)}};
    auto message = Fractal::Response_message{};
    ss >> message;
    print(message);

    // Just forward the message onto the master
    auto request_result = gg_request_result{};
    print(gg_publish(request, 
                     Subscribers::master.c_str(), 
                     reinterpret_cast<const void*>(&buffer[0]),
                     buffer.size(), 
                     &request_result),
          " Forwarding cancel request to slaves.",
          GG_LOG_DEBUG);
    print(request_result, "Result of response forwarding.", GG_LOG_DEBUG);
  };

  auto handle_message = [&](const std::vector<int8_t>& buffer)
  {
    if (buffer.empty())
    {
      return;
    }

    auto request = GG_request_ptr{};

    switch (buffer[0])
    {
      case Fractal::Request_message::ID:
      {
        handle_request_message(request, buffer);
      } break;
      case Fractal::Cancel_message::ID:
      {
        handle_request_message(request, buffer);
      } break;
      case Fractal::Response_message::ID:
      {
        handle_request_message(request, buffer);
      } break;
    }
  };

  auto buffer = std::vector<int8_t>(1024 * 1024 * 128, 0); // Max message size is 128K
  auto bytes_read = size_t{0};

  if (read(buffer, bytes_read) != GGE_SUCCESS)
  {
    // Dump what was read into the log.
    print(buffer, "", GG_LOG_DEBUG);
    return;
  }

  print(buffer, "", GG_LOG_DEBUG);
  handle_message(buffer);
}

int main(int argc, const char* argv[])
{
  gg_global_init(0);
  gg_log(GG_LOG_INFO, "Starting runtime.");
  gg_runtime_start(lambda_callback, 0);
  return 0;
}

