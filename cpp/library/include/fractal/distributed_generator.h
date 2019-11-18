//
//  distributed_generator.h
//  fractal
//
//  Created by Banks, Timothy on 10/14/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#ifndef distributed_generator_h
#define distributed_generator_h

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "fractal_view.h"
#include "messages.h"
#include "task_parameters.h"

namespace Fractal
{

class Distributed_generator final
{
public:
  Distributed_generator() = delete;
  Distributed_generator(std::size_t max_thread_count)
  : m_max_thread_count{max_thread_count}
  {
    construct_thread_pool_();
  }
  Distributed_generator(const Distributed_generator&) = delete;
  Distributed_generator(Distributed_generator&&) = delete;
  ~Distributed_generator()
  {
    destruct_thread_pool_();
  }
  
  Distributed_generator& operator=(const Distributed_generator&) = delete;
  Distributed_generator& operator=(Distributed_generator&&) = delete;
  
  template <typename Function>
  void invoke(Function function, Generator_task_parameters& task_parameters)
  {
    {
      std::lock_guard<std::mutex> lock{m_tasks_mutex};
      invoke_(std::move(function), task_parameters);
      m_task_ready_condition.notify_all();
    }
    
    auto lock = std::unique_lock<std::mutex>{m_tasks_mutex};
    while (!m_task_done_condition.wait_for(lock, std::chrono::milliseconds(1000), [&] () { return (m_tasks.empty() && m_executing_task_count == 0) || m_destructing; })){}
  }
  
  template <typename Function>
  void invoke(Function function, std::vector<Generator_task_parameters>& task_parameters)
  {
    {
      std::lock_guard<std::mutex> lock{m_tasks_mutex};
      for (auto& parameter : task_parameters)
      {
        invoke_(function, parameter);
      }
      
      m_task_ready_condition.notify_all();
    }
    
    auto lock = std::unique_lock<std::mutex>{m_tasks_mutex};
    while (!m_task_done_condition.wait_for(lock, std::chrono::milliseconds(1000), [&] () { return (m_tasks.empty() && m_executing_task_count == 0) || m_destructing; })){}
  }
  
  template <typename Function>
  void operator()(Function function, Generator_task_parameters& task_parameters)
  {
    invoke(std::move(function), task_parameters);
  }
  
  template <typename Function>
  void operator()(Function function, std::vector<Generator_task_parameters>& task_parameters)
  {
    invoke(std::move(function), task_parameters);
  }
  
protected:
  template <typename Function>
  void invoke_(Function function, Generator_task_parameters& task_parameters)
  {
    m_tasks.emplace([function = std::move(function), tp = task_parameters]() mutable
    {
      auto real_factor = tp.fractal_view.complex_view().width() / static_cast<double>(tp.fractal_view.pixel_view().width());
      auto imaginary_factor = tp.fractal_view.complex_view().height() / static_cast<double>(tp.fractal_view.pixel_view().height());

      auto c = std::complex<double>{};
      
      for (size_t i = tp.pixel_tile_view.left; i < tp.pixel_tile_view.right; ++i)
      {
        c.real(tp.fractal_view.complex_view().left + i * real_factor);
        
        for (size_t j = tp.pixel_tile_view.top; j < tp.pixel_tile_view.bottom; ++j)
        {
          c.imag(tp.fractal_view.complex_view().top + j * imaginary_factor);
          tp.fractal_view.buffer().get()[i + j * tp.fractal_view.pixel_view().width()] = function(c, tp.cancel_token);
          
          if (tp.cancel_token->load())
          {
            break;
          }
        }
        
        if (tp.cancel_token->load())
        {
          break;
        }
      }
      
      if (tp.cancel_token->load())
      {
        tp.on_task_canceled(tp);
      }
      else
      {
        tp.on_task_completed(tp);
      }
    });
  }
  
private:
  void construct_thread_pool_()
  {
    m_thread_pool.reserve(m_max_thread_count);
    for (size_t i = 0; i < m_max_thread_count; ++i)
    {
      m_thread_pool.emplace_back([&]()
      {
        while (!m_destructing)
        {
          auto lock = std::unique_lock<std::mutex>{m_tasks_mutex};
          while (!m_task_ready_condition.wait_for(lock, std::chrono::milliseconds(1000), [&] () { return !m_tasks.empty() || m_destructing; })){}
        
          if (m_destructing)
          {
            return;
          }
        
          auto task = m_tasks.front();
          m_tasks.pop();
          lock.unlock();
        
          ++m_executing_task_count;
          try
          {
            task();
          }
          catch (...)
          {
          }
          --m_executing_task_count;
          m_task_done_condition.notify_one();
        }
      });
    }
  }
  
  void destruct_thread_pool_()
  {
    m_destructing = true;
    m_task_ready_condition.notify_all();
    for (auto& thread : m_thread_pool)
    {
      thread.join();
    }
  }

private:
  using Task_predicate = std::function<void()>;
  std::queue<Task_predicate> m_tasks;
  std::mutex m_tasks_mutex;
  std::atomic<size_t> m_executing_task_count{0};
  std::condition_variable m_task_ready_condition;
  std::condition_variable m_task_done_condition;
  
  std::vector<std::thread> m_thread_pool;
  std::size_t m_max_thread_count{7};
  std::atomic<bool> m_destructing{false};
};

}

#endif /* distributed_generator_h */
