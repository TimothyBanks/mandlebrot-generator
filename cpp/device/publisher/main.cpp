#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#ifdef USE_WEBSOCKETS
#include "WebSocketConnection.hpp"
#elif defined USE_MBEDTLS
#include "MbedTLSConnection.hpp"
#else
#include "OpenSSLConnection.hpp"
#endif

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

#include "ConfigCommon.hpp"
#include "publisher.h"

namespace Onboarding
{
awsiotsdk::ResponseCode Publisher::publish_request_message_(const Fractal::Request_message& message)
{
  std::cout << "****** PUBLISHING REQUEST MESSAGE *******" << std::endl;
  Fractal::print(std::cout, message);

  // Serialize message to sent.
  auto ss = std::stringstream{};
  ss << message;

  return m_iot_client->Publish(awsiotsdk::Utf8String::Create(m_topic), 
                               false,
                               false, 
                               awsiotsdk::mqtt::QoS::QOS1,
                               ss.str(),
                               std::chrono::milliseconds{1000});
}    

awsiotsdk::ResponseCode Publisher::publish_cancel_message_(const Fractal::Cancel_message& message)
{
  std::cout << "****** PUBLISHING CANCEL MESSAGE *******" << std::endl;
  Fractal::print(std::cout, message);

  // Serialize message to sent.
  auto ss = std::stringstream{};
  ss << message;

  return m_iot_client->Publish(awsiotsdk::Utf8String::Create(m_topic), 
                               false,
                               false, 
                               awsiotsdk::mqtt::QoS::QOS1,
                               ss.str(),
                               std::chrono::milliseconds{1000});
}                                                           

awsiotsdk::ResponseCode Publisher::handle_response_message_(const awsiotsdk::util::String& payload)
{
  auto message = Fractal::Response_message{};
  auto ss = std::stringstream{payload};
  ss >> message;

  std::cout << "****** RECEIVED RESPONSE MESSAGE ******" << std::endl;
  Fractal::print(std::cout, message);

  // Write tile to the view
  for (size_t i = 0; i < message.header.device.width(); ++i)
  {
    auto view_x_offset = i + message.header.device.left;
    for (size_t j = 0; j < message.header.device.height(); ++j)
    {
      auto view_y_offset = j + message.header.device.top;
      m_current_view.buffer().get()[view_x_offset + view_y_offset * m_current_view.pixel_view().height()]
        = message.argb_buffer[i + j * message.header.device.height()];
    }
  }

  // Write current view to file
  {
    std::lock_guard<std::mutex> lk{m_mutex};
    auto out = std::ofstream{m_current_filename};
    Fractal::print(out, 
                   m_current_view.buffer(), 
                   m_current_view.pixel_view().width(), 
                   m_current_view.pixel_view().height());
  }

  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Publisher::subscribe_callback_(awsiotsdk::util::String topic_name,
                                                       awsiotsdk::util::String payload,
                                                       std::shared_ptr<awsiotsdk::mqtt::SubscriptionHandlerContextData> app_handler_data)
{
  if (payload.empty())
  {
    return awsiotsdk::ResponseCode::SUCCESS;
  }

  auto message_type = static_cast<uint8_t>(payload[0] - 48);
  switch (message_type)
  {
    case Fractal::Response_message::ID:
    {
      return handle_response_message_(payload);
    } break;
  }

  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Publisher::disconnect_callback_(awsiotsdk::util::String client_id,
                                                        std::shared_ptr<awsiotsdk::DisconnectCallbackContextData> app_handler_data) 
{
  std::cout << "*******************************************" << std::endl
            << client_id << " Disconnected!" << std::endl
            << "*******************************************" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Publisher::reconnect_callback_(awsiotsdk::util::String client_id,
                                                       std::shared_ptr<awsiotsdk::ReconnectCallbackContextData> app_handler_data,
                                                       awsiotsdk::ResponseCode reconnect_result) 
{
  std::cout << "*******************************************" << std::endl
            << client_id << " Reconnect Attempted. Result " << awsiotsdk::ResponseHelper::ToString(reconnect_result)
            << std::endl
            << "*******************************************" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Publisher::resubscribe_callback_(awsiotsdk::util::String client_id,
                                                         std::shared_ptr<awsiotsdk::ResubscribeCallbackContextData> app_handler_data,
                                                         awsiotsdk::ResponseCode resubscribe_result) 
{
  std::cout << "*******************************************" << std::endl
            << client_id << " Resubscribe Attempted. Result" << awsiotsdk::ResponseHelper::ToString(resubscribe_result)
            << std::endl
            << "*******************************************" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}


awsiotsdk::ResponseCode Publisher::subscribe_() 
{
  awsiotsdk::mqtt::Subscription::ApplicationCallbackHandlerPtr sub_handler 
    = std::bind(&Publisher::subscribe_callback_,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);
  auto subscription =
      awsiotsdk::mqtt::Subscription::Create(awsiotsdk::Utf8String::Create(m_topic), 
                                            awsiotsdk::mqtt::QoS::QOS0, 
                                            sub_handler, 
                                            nullptr);
  auto topic_vector = awsiotsdk::util::Vector<std::shared_ptr<awsiotsdk::mqtt::Subscription>>{};
  topic_vector.push_back(subscription);
  auto rc = m_iot_client->Subscribe(topic_vector, awsiotsdk::ConfigCommon::mqtt_command_timeout_);
  std::this_thread::sleep_for(std::chrono::seconds(3));
  return rc;
}

awsiotsdk::ResponseCode Publisher::unsubscribe_() 
{
  auto topic_vector = awsiotsdk::util::Vector<std::unique_ptr<awsiotsdk::Utf8String>>{};
  topic_vector.push_back(awsiotsdk::Utf8String::Create(m_topic));
  auto rc = m_iot_client->Unsubscribe(std::move(topic_vector), awsiotsdk::ConfigCommon::mqtt_command_timeout_);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return rc;
}

awsiotsdk::ResponseCode Publisher::initialize_TLS_() 
{
  auto rc = awsiotsdk::ResponseCode::SUCCESS;

#ifdef USE_WEBSOCKETS
  m_network_connection = std::shared_ptr<awsiotsdk::NetworkConnection>(
      new network::WebSocketConnection(awsiotsdk::ConfigCommon::endpoint_, 
                                       awsiotsdk::ConfigCommon::endpoint_https_port_,
                                       awsiotsdk::ConfigCommon::root_ca_path_, 
                                       awsiotsdk::ConfigCommon::aws_region_,
                                       awsiotsdk::ConfigCommon::aws_access_key_id_,
                                       awsiotsdk::ConfigCommon::aws_secret_access_key_,
                                       awsiotsdk::ConfigCommon::aws_session_token_,
                                       awsiotsdk::ConfigCommon::tls_handshake_timeout_,
                                       awsiotsdk::ConfigCommon::tls_read_timeout_,
                                       awsiotsdk::ConfigCommon::tls_write_timeout_, 
                                       true));
  if (!m_network_connection) 
  {
    AWS_LOG_ERROR(LOG_TAG_PUBSUB, 
                  "Failed to initialize Network Connection. %s",
                  awsiotsdk::ResponseHelper::ToString(rc).c_str());
    rc = awsiotsdk::ResponseCode::FAILURE;
  }
#elif defined USE_MBEDTLS
  m_network_connection 
    = std::make_shared<awsiotsdk::network::MbedTLSConnection>(awsiotsdk::awsiotsdk::ConfigCommon::endpoint_,
                                                              awsiotsdk::ConfigCommon::endpoint_mqtt_port_,
                                                              awsiotsdk::ConfigCommon::root_ca_path_,
                                                              awsiotsdk::ConfigCommon::client_cert_path_,
                                                              awsiotsdk::ConfigCommon::client_key_path_,
                                                              awsiotsdk::ConfigCommon::tls_handshake_timeout_,
                                                              awsiotsdk::ConfigCommon::tls_read_timeout_,
                                                              awsiotsdk::ConfigCommon::tls_write_timeout_,
                                                              true);
  if (!m_network_connection) 
  {
    AWS_LOG_ERROR(LOG_TAG_PUBSUB, 
                  "Failed to initialize Network Connection. %s",
                  awsiotsdk::ResponseHelper::ToString(rc).c_str());
    rc = awsiotsdk::ResponseCode::FAILURE;
  }
#else
  std::shared_ptr<awsiotsdk::network::OpenSSLConnection> network_connection =
      std::make_shared<awsiotsdk::network::OpenSSLConnection>(awsiotsdk::ConfigCommon::endpoint_,
                                                              awsiotsdk::ConfigCommon::endpoint_mqtt_port_,
                                                              awsiotsdk::ConfigCommon::root_ca_path_,
                                                              awsiotsdk::ConfigCommon::client_cert_path_,
                                                              awsiotsdk::ConfigCommon::client_key_path_,
                                                              awsiotsdk::ConfigCommon::tls_handshake_timeout_,
                                                              awsiotsdk::ConfigCommon::tls_read_timeout_,
                                                              awsiotsdk::ConfigCommon::tls_write_timeout_, 
                                                              true);
  rc = network_connection->Initialize();

  if (awsiotsdk::ResponseCode::SUCCESS != rc) 
  {
    AWS_LOG_ERROR("Onboarding::Publisher",
                  "Failed to initialize Network Connection. %s",
                  awsiotsdk::ResponseHelper::ToString(rc).c_str());
    rc = awsiotsdk::ResponseCode::FAILURE;
  } 
  else 
  {
    m_network_connection = std::dynamic_pointer_cast<awsiotsdk::NetworkConnection>(network_connection);
  }
#endif
  return rc;
}

awsiotsdk::ResponseCode Publisher::run()
{
  auto rc = initialize_TLS_();
  if (awsiotsdk::ResponseCode::SUCCESS != rc) 
  {
    return rc;
  }

  awsiotsdk::ClientCoreState::ApplicationDisconnectCallbackPtr disconnect_handler =
      std::bind(&Publisher::disconnect_callback_, this, std::placeholders::_1, std::placeholders::_2);

  awsiotsdk::ClientCoreState::ApplicationReconnectCallbackPtr reconnect_handler =
      std::bind(&Publisher::reconnect_callback_,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);

  awsiotsdk::ClientCoreState::ApplicationResubscribeCallbackPtr resubscribe_handler =
      std::bind(&Publisher::resubscribe_callback_,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);

  m_iot_client = std::shared_ptr<awsiotsdk::MqttClient>(awsiotsdk::MqttClient::Create(m_network_connection,
                                                                                      awsiotsdk::ConfigCommon::mqtt_command_timeout_,
                                                                                      disconnect_handler, 
                                                                                      nullptr,
                                                                                      reconnect_handler, 
                                                                                      nullptr,
                                                                                      resubscribe_handler, 
                                                                                      nullptr));
  if (!m_iot_client) 
  {
    return awsiotsdk::ResponseCode::FAILURE;
  }

  auto client_id_tagged = awsiotsdk::ConfigCommon::base_client_id_;
  client_id_tagged.append("_on_boarding_publisher_");
  client_id_tagged.append(std::to_string(rand()));

  rc = m_iot_client->Connect(awsiotsdk::ConfigCommon::mqtt_command_timeout_, 
                             awsiotsdk::ConfigCommon::is_clean_session_,
                             awsiotsdk::mqtt::Version::MQTT_3_1_1, 
                             awsiotsdk::ConfigCommon::keep_alive_timeout_secs_,
                             awsiotsdk::Utf8String::Create(client_id_tagged), 
                             nullptr, 
                             nullptr, 
                             nullptr);
  if (awsiotsdk::ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED != rc) 
  {
    return rc;
  }

  rc = subscribe_();
  if (awsiotsdk::ResponseCode::SUCCESS != rc) 
  {
    AWS_LOG_ERROR("Onboarding::Publisher", "Subscribe failed. %s", awsiotsdk::ResponseHelper::ToString(rc).c_str());
  } 

  std::cout << "Welcome to the Mandlebrot Generator!" << std::endl;
  std::cout << "(Enter 'q' and press return at any time to quit)" << std::endl;
  while (true)
  {
    std::cout << "Enter filename: ";
    std::getline(std::cin, m_current_filename);

    if (m_current_filename == "q" || m_current_filename == "Q")
    {
      break;
    }

    auto width = std::string{};
    std::cout << "Enter Image Width: ";
    std::getline(std::cin, width);

    if (width == "q" || width == "Q")
    {
      break;
    }

    auto height = std::string{};
    std::cout << "Enter Image Height: ";
    std::getline(std::cin, height);

    if (height == "q" || height == "Q")
    {
      break;
    }

    auto bounding_box = std::string{};
    std::cout << "Enter Complex Bounding Box (Left Top Right Bottom): ";
    std::getline(std::cin, bounding_box);

    if (bounding_box == "q" || bounding_box == "Q")
    {
      break;
    }

    auto max_iterations = std::string{};
    std::cout << "Enter Max Iterations: ";
    std::getline(std::cin, max_iterations);

    if (max_iterations == "q" || max_iterations == "Q")
    {
      break;
    }

    auto request_message = Fractal::Request_message{};
    request_message.header.header.type = Fractal::Request_message::ID;
    request_message.header.header.identifier = std::to_string(m_current_identifier.fetch_add(1));
    auto ss = std::stringstream{width};
    ss >> request_message.header.device.right;
    ss = std::stringstream{height};
    ss >> request_message.header.device.bottom;
    ss = std::stringstream{bounding_box};
    ss >> request_message.header.complex;
    ss = std::stringstream{max_iterations};
    ss >> request_message.max_iterations;

    std::cout << "***** GENERATED REQUEST MESSAGE ******" << std::endl;
    Fractal::print(std::cout, request_message);

    m_current_view = Fractal::Fractal_view{request_message.header.device, request_message.header.complex};
    publish_request_message_(request_message);

    // TODO:  Currently don't know when the fractal is done being generated.  Need to think about how that information will be provided.
  }

  rc = m_iot_client->Disconnect(awsiotsdk::ConfigCommon::mqtt_command_timeout_);
  if (awsiotsdk::ResponseCode::SUCCESS != rc) 
  {
    AWS_LOG_ERROR("Onboarding::Publisher", "Disconnect failed. %s", awsiotsdk::ResponseHelper::ToString(rc).c_str());
  }

  std::cout << "Exiting Subscriber!!!!" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}

}

int main(int argc, char **argv) 
{
  auto log_system = std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
  awsiotsdk::util::Logging::InitializeAWSLogging(log_system);
  auto publisher = std::make_unique<Onboarding::Publisher>();
  awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/onboarding_publisher.json");
  if (awsiotsdk::ResponseCode::SUCCESS == rc) 
  {
    rc = publisher->run();
  }
  awsiotsdk::util::Logging::ShutdownAWSLogging();
  return static_cast<int>(rc);
}
