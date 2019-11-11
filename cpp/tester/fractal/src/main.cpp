//
//  main.cpp
//  fractal
//
//  Created by Banks, Timothy on 10/14/19.
//  Copyright © 2019 Banks, Timothy. All rights reserved.
//

#include <fstream>
#include <iostream>
#include <sstream>

#include <fractal/distributed_generator.h>
#include <fractal/fractal_view.h>
#include <fractal/julia_function.h>
#include <fractal/mandlebrot_function.h>
#include <fractal/messages.h>
#include <fractal/task_parameters.h>

int main(int argc, const char * argv[])
{
  //auto pixel_view = Fractal::View<std::size_t>{0, 0, 8192, 8192};
  auto pixel_view = Fractal::View<std::size_t>{0, 0, 1463, 1315};
  
  auto min_real = double{-2.0};
  auto max_real = double{2.0};
  auto min_imaginary = double{-2.0};
  auto max_imaginary = min_imaginary + (max_real - min_real) * (static_cast<double>(pixel_view.height()) / static_cast<double>(pixel_view.width()));
  auto complex_view = Fractal::View<double>{min_real, min_imaginary, max_real, max_imaginary};

  auto device = Fractal::View<size_t>{0, 0, 256, 256};
  auto complex = Fractal::View<Fractal::float64>{min_real, min_imaginary, max_real, max_imaginary};

  auto argb_buffer = std::vector<uint32_t>(device.right * device.bottom, 0);
  for (uint32_t i = 0; i < device.right * device.bottom; ++i)
  {
    argb_buffer[i] = i;
  }

  auto request_message = Fractal::Request_message{};
  request_message.header.header.type = Fractal::Request_message::ID;
  request_message.header.header.identifier = "00000000000000000000000000000001";
  request_message.header.device = device;
  request_message.header.complex = complex;
  request_message.max_iterations = 256;
  print(std::cout, request_message);

  auto response_message = Fractal::Response_message{};
  response_message.header.header.type = Fractal::Response_message::ID;
  response_message.header.header.identifier = "00000000000000000000000000000002";
  response_message.header.device = device;
  response_message.header.complex = complex;
  response_message.argb_buffer = argb_buffer;
  print(std::cout, response_message);

  auto cancel_message = Fractal::Cancel_message{};
  cancel_message.header.type = Fractal::Cancel_message::ID;
  cancel_message.header.identifier = "00000000000000000000000000000003";
  print(std::cout, cancel_message);

  auto string_stream = std::stringstream{};
  string_stream << request_message;
  auto new_request_message = Fractal::Request_message{};
  string_stream >> new_request_message;
  std::cout << "Request messages equal: " << (request_message == new_request_message) << std::endl;

  string_stream = std::stringstream{};
  string_stream << response_message;
  auto new_response_message = Fractal::Response_message{};
  string_stream >> new_response_message;
  std::cout << "Respoinse messages equal: " << (response_message == new_response_message) << std::endl;

  string_stream = std::stringstream{};
  string_stream << cancel_message;
  auto new_cancel_message = Fractal::Cancel_message{};
  string_stream >> new_cancel_message;
  std::cout << "Cancel messages equal: " << (cancel_message == new_cancel_message) << std::endl;

  auto fractal_view = Fractal::Fractal_view{std::move(pixel_view), std::move(complex_view)};
  auto function = Fractal::Mandlebrot_function{1000};
  //auto function = Fractal::Julia_function{{-0.8, 0.156}, 1000};
  
  auto on_task_completed = [](const Fractal::Task_parameters&){};
  auto on_task_canceled = [](const Fractal::Task_parameters&){};
  auto cancel_token = std::make_shared<std::atomic<bool>>(false);
  auto tasks = Fractal::Generator_task_parameters::distribute("1",
                                                              cancel_token,
                                                              on_task_completed,
                                                              on_task_canceled,
                                                              fractal_view,
                                                              512,
                                                              512);
  
  auto generator = Fractal::Distributed_generator{3};
  generator(std::move(function), tasks);

  auto stream = std::ofstream{"/Users/banksti/Documents/projects/fractal/fractal/mandelbrot.ppm"};
  auto width = fractal_view.pixel_view().width();
  auto height = fractal_view.pixel_view().height();
  auto& result = fractal_view.buffer();
  Fractal::print(stream, result, width, height);

  return 0;
}
