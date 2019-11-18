//
//  fractal_view.h
//  fractal
//
//  Created by Banks, Timothy on 10/14/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#ifndef fractal_view_h
#define fractal_view_h

#include <complex>
#include <cstddef>
#include <functional>
#include <vector>

#include "messages.h"

namespace Fractal
{

class Fractal_view final
{
public:
  using Pixel_view = View<std::size_t>;
  using Complex_view = View<double>;

  Fractal_view() = default;
  Fractal_view(Pixel_view pixel_view, Complex_view complex_view, std::shared_ptr<uint32_t> buffer)
  : m_pixel_view{std::move(pixel_view)},
    m_complex_view{std::move(complex_view)},
    m_buffer{std::move(buffer)}
  {
  }
  Fractal_view(Pixel_view pixel_view, Complex_view complex_view)
  : m_pixel_view{std::move(pixel_view)},
    m_complex_view{std::move(complex_view)},
    m_buffer{std::shared_ptr<uint32_t>(new uint32_t[m_pixel_view.width() * m_pixel_view.height()], std::default_delete<uint32_t[]>())}
  {
  }
  Fractal_view(const Fractal_view&) = default;
  Fractal_view(Fractal_view&&) = default;
  ~Fractal_view() = default;

  Fractal_view& operator=(const Fractal_view&) = default;
  Fractal_view& operator=(Fractal_view&&) = default;

  Pixel_view& pixel_view()
  {
    return m_pixel_view;
  }
  
  const Pixel_view& pixel_view() const
  {
    return m_pixel_view;
  }
  
  Complex_view& complex_view()
  {
    return m_complex_view;
  }
  
  const Complex_view& complex_view() const
  {
    return m_complex_view;
  }
  
  std::shared_ptr<uint32_t>& buffer()
  {
    return m_buffer;
  }
  
  const std::shared_ptr<uint32_t>& buffer() const
  {
    return m_buffer;
  }
  
private:
  Pixel_view m_pixel_view;
  Complex_view m_complex_view;
  std::shared_ptr<uint32_t> m_buffer;
};

}

#endif /* fractal_view_h */
