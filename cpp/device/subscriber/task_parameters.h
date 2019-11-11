//
//  task.h
//  fractal
//
//  Created by Banks, Timothy on 10/28/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#ifndef task_h
#define task_h

#include <atomic>
#include <functional>

#include "fractal_view.h"

namespace Fractal
{
struct Task_parameters
{
  std::string identifier;
  std::shared_ptr<std::atomic<bool>> cancel_token{std::make_shared<std::atomic<bool>>(false)};
  std::function<void(const Task_parameters&)> on_task_completed;
  std::function<void(const Task_parameters&)> on_task_canceled;
};

struct Generator_task_parameters : Task_parameters
{
  Fractal_view fractal_view;
  Fractal_view::Pixel_view pixel_tile_view;
  Fractal_view::Complex_view complex_tile_view;
  
  template <typename Completed_callback, typename Canceled_callback>
  static std::vector<Generator_task_parameters> distribute(std::string task_identifier,
                                                           std::shared_ptr<std::atomic<bool>> cancel_token,
                                                           Completed_callback on_task_completed,
                                                           Canceled_callback on_task_canceled,	
                                                           Fractal_view view,
                                                           size_t tile_width,
                                                           size_t tile_height)
  {
    auto tasks = std::vector<Generator_task_parameters>{};
    
    if (tile_width == 0 || tile_height == 0 || tile_width >= view.pixel_view().width() || tile_height >= view.pixel_view().height())
    {
      tasks.emplace_back();
      auto& task = tasks.back();
      task.fractal_view = view;
      task.pixel_tile_view = view.pixel_view();
      task.complex_tile_view = view.complex_view();
      task.identifier = task_identifier;
      task.cancel_token = cancel_token;
      task.on_task_completed = on_task_completed;
      task.on_task_canceled = on_task_canceled;
      return tasks;
    }
  
    auto rows = view.pixel_view().height() / tile_height;
    if (view.pixel_view().height() % tile_height != 0)
    {
      ++rows;
    }
    auto columns = view.pixel_view().width() / tile_width;
    if (view.pixel_view().width() % tile_width != 0)
    {
      ++columns;
    }
    auto complex_width_interval = view.complex_view().width() / static_cast<double>(columns);
    auto complex_height_interval = view.complex_view().height() / static_cast<double>(rows);
  
    auto pixel_view = Fractal_view::Pixel_view{};
    auto complex_view = Fractal_view::Complex_view{};
    
    for (size_t i = 0; i < columns; ++i)
    {
      pixel_view.left = view.pixel_view().left + i * tile_width;
      pixel_view.right = (i == columns - 1) ? view.pixel_view().right : pixel_view.left + tile_width;
      complex_view.left = view.complex_view().left + i * complex_width_interval;
      complex_view.right = (i == columns - 1) ? view.complex_view().right : complex_view.left + complex_width_interval;
      
      for (size_t j = 0; j < rows; ++j)
      {
        pixel_view.top = view.pixel_view().top + j * tile_height;
        pixel_view.bottom = (j == rows - 1) ? view.pixel_view().bottom : pixel_view.top + tile_height;
        complex_view.top = view.complex_view().top + j * complex_height_interval;
        complex_view.bottom = (j == rows - 1) ? view.complex_view().bottom : complex_view.top + complex_height_interval;
      
        tasks.emplace_back();
        auto& task = tasks.back();
        task.fractal_view = view;
        task.pixel_tile_view = pixel_view;
        task.complex_tile_view = complex_view;
        task.identifier = task_identifier;
        task.cancel_token = cancel_token;
        task.on_task_completed = on_task_completed;
        task.on_task_canceled = on_task_canceled;
      }
    }
    
    return tasks;
  }
};

}

#endif /* task_h */
