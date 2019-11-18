//
//  ppm.h
//  fractal
//
//  Created by Banks, Timothy on 10/14/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#ifndef ppm_h
#define ppm_h

#include <iostream>
#include <fstream>
#include <string>

namespace Fractal
{

class PPM_stream final
{
public:
  PPM_stream() = default;
  PPM_stream(std::string file_name)
  : m_stream{file_name, std::ios::out | std::ios::trunc},
    m_filename{std::move(file_name)}
  {
  }
  PPM_stream(const PPM_stream&) = default;
  PPM_stream(PPM_stream&&) = default;
  
  ~PPM_stream()
  {
    if (m_stream.is_open())
    {
      m_stream.flush();
      m_stream.close();
    }
  }
  
  PPM_stream& operator=(const PPM_stream&) = default;
  PPM_stream& operator=(PPM_stream&&) = default;
  
  template <typename T>
  PPM_stream& write(const T& t)
  {
    m_stream << t;
    return *this;
  }
  
  void open(std::string filename)
  {
    if (m_stream.is_open())
    {
      m_stream.flush();
      m_stream.close();
    }
    
    m_stream = std::ofstream{filename, std::ios::out | std::ios::trunc};
    m_filename = std::move(filename);
  }
  
  void save()
  {
    m_stream.flush();
  }
  
  void close()
  {
    m_stream.close();
  }
  
private:
  std::ofstream m_stream;
  std::string m_filename;
};

template <typename T>
PPM_stream& operator<<(PPM_stream& stream, const T& t)
{
  return stream.write(t);
}

}

#endif /* ppm_h */
