//
//  julia_function.h
//  fractal
//
//  Created by Banks, Timothy on 10/14/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#ifndef julia_function_h
#define julia_function_h

#include <complex>
#include <functional>

namespace Fractal
{

class Julia_function final
{
public:
  Julia_function() = default;
  Julia_function(std::complex<double> k, size_t max_iterations)
  : m_k{std::move(k)},
    m_max_iterations{max_iterations}
  {
  }
  Julia_function(const Julia_function&) = default;
  Julia_function(Julia_function&&) = default;
  ~Julia_function() = default;
  
  Julia_function& operator=(const Julia_function&) = default;
  Julia_function& operator=(Julia_function&&) = default;
  
  uint64_t invoke(std::complex<double> z, std::shared_ptr<std::atomic<bool>>& cancel_token) const
  {
    // Value is part of the Mandlebrot set, let's just represent that by the color black
    static constexpr auto black = uint64_t{0xFF000000};
    
    auto iterations = m_max_iterations;
    for (size_t i = 0; i < iterations; ++i)
    {
      if (z.real() * z.real() + z.imag() * z.imag() > 4)
      {
        // This value doesn't belong in the Mandlebrot set.  Apparently we are checking if Z is divergent
        // and if the distance of Z is more than 2 units from the origin then it will inevitably go to infinity.
        auto t = static_cast<double>(i) / static_cast<double>(m_max_iterations);
        
        // Use smooth polynomials for r, g, b
        auto r = static_cast<int8_t>(9*(1-t)*t*t*t*255);
        auto g = static_cast<int8_t>(15*(1-t)*(1-t)*t*t*255);
        auto b = static_cast<int8_t>(8.5*(1-t)*(1-t)*(1-t)*t*255);
        return (0xff << 24) | (r << 16) | (g << 8) | b;
      }
      
      z = z * z + m_k;
      
      if (*cancel_token)
      {
        return black;
      }
    }
    
    return black;
  }
  
  uint64_t operator()(std::complex<double> z, std::shared_ptr<std::atomic<bool>>& cancel_token) const
  {
    return invoke(std::move(z), cancel_token);
  }

  size_t max_iterations() const
  {
    return m_max_iterations;
  }
  
  void set_max_iterations(size_t iterations)
  {
    m_max_iterations = iterations;
  }
  
  std::complex<double> k() const
  {
    return m_k;
  }
  
  void set_k(std::complex<double> k)
  {
    m_k = std::move(k);
  }

private:
  std::complex<double> m_k;
  size_t m_max_iterations{64};
};

}

#endif /* julia_function_h */
