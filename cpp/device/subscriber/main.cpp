#include <chrono>
#include <cstring>
#include <memory>

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
#include "fractal_view.h"
#include "mandlebrot_function.h"
#include "subscriber.h"

namespace Onboarding
{
awsiotsdk::ResponseCode Subscriber::publish_response_message_(const Fractal::Response_message& message)
{
  std::cout << "****** PUBLISHING RESPONSE MESSAGE *******" << std::endl;
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

awsiotsdk::ResponseCode Subscriber::handle_request_message_(const awsiotsdk::util::String& payload)
{
  auto message = Fractal::Request_message{};
  auto ss = std::stringstream{payload};
  ss >> message;

  std::cout << "****** RECEIVED REQUEST MESSAGE ******" << std::endl;
  Fractal::print(std::cout, message);

  auto fractal_view = Fractal::Fractal_view{message.header.device, message.header.complex};
  auto cancel_token = std::make_shared<std::atomic<bool>>(false);
  auto on_task_canceled = [](const Fractal::Task_parameters&){};      
  auto on_task_completed = [&](const Fractal::Task_parameters& parameters)
  {
    const auto& generator_parameters = static_cast<const Fractal::Generator_task_parameters&>(parameters);

    auto response_message = Fractal::Response_message{};
    response_message.header.header.type = Fractal::Response_message::ID;
    response_message.header.header.identifier = generator_parameters.identifier;
    response_message.header.complex = generator_parameters.complex_tile_view;
    response_message.header.device = generator_parameters.pixel_tile_view;
    response_message.argb_buffer 
      = std::vector<uint32_t>(generator_parameters.pixel_tile_view.width() * generator_parameters.pixel_tile_view.height(), 0);

    for (size_t i = 0; i < generator_parameters.pixel_tile_view.width(); ++i)
    {
      auto view_x_offset = i + generator_parameters.pixel_tile_view.left;
      for (size_t j = 0; j < generator_parameters.pixel_tile_view.height(); ++j)
      {
        auto view_y_offset = j + generator_parameters.pixel_tile_view.top;
        response_message.argb_buffer[i + j * generator_parameters.pixel_tile_view.height()]
          = fractal_view.buffer().get()[view_x_offset + view_y_offset * fractal_view.pixel_view().height()];
      }
    }

    publish_response_message_(response_message);
  };

  auto task_parameters = Fractal::Generator_task_parameters::distribute(message.header.header.identifier,
                                                                        cancel_token,
                                                                        on_task_completed,
                                                                        on_task_canceled,
                                                                        fractal_view,
                                                                        128,
                                                                        128);

  {
    std::lock_guard<std::mutex> lk{m_mutex};
    m_cancel_tokens.emplace(std::make_pair(message.header.header.identifier, cancel_token));
  }

  auto function = Fractal::Mandlebrot_function{message.max_iterations};
  m_generator(function, task_parameters);

  {
    std::lock_guard<std::mutex> lk{m_mutex};
    m_cancel_tokens.erase(message.header.header.identifier);
  }                                                              

  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Subscriber::handle_cancel_message_(const awsiotsdk::util::String& payload)
{
  auto message = Fractal::Cancel_message{};
  auto ss = std::stringstream{payload};
  ss >> message;

  std::cout << "****** RECEIVED CANCEL MESSAGE ******" << std::endl;
  Fractal::print(std::cout, message);

  auto it = m_cancel_tokens.find(message.header.identifier);
  if (it != std::end(m_cancel_tokens))
  {
    it->second->store(true);
    m_cancel_tokens.erase(it);
  }

  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Subscriber::subscribe_callback_(awsiotsdk::util::String topic_name,
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
    case Fractal::Request_message::ID:
    {
      return handle_request_message_(payload);
    } break;
    case Fractal::Cancel_message::ID:
    {
      return handle_cancel_message_(payload);
    } break;
  }

  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Subscriber::disconnect_callback_(awsiotsdk::util::String client_id,
                                                         std::shared_ptr<awsiotsdk::DisconnectCallbackContextData> app_handler_data) 
{
  std::cout << "*******************************************" << std::endl
            << client_id << " Disconnected!" << std::endl
            << "*******************************************" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Subscriber::reconnect_callback_(awsiotsdk::util::String client_id,
                                                        std::shared_ptr<awsiotsdk::ReconnectCallbackContextData> app_handler_data,
                                                        awsiotsdk::ResponseCode reconnect_result) 
{
  std::cout << "*******************************************" << std::endl
            << client_id << " Reconnect Attempted. Result " << awsiotsdk::ResponseHelper::ToString(reconnect_result)
            << std::endl
            << "*******************************************" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}

awsiotsdk::ResponseCode Subscriber::resubscribe_callback_(awsiotsdk::util::String client_id,
                                                          std::shared_ptr<awsiotsdk::ResubscribeCallbackContextData> app_handler_data,
                                                          awsiotsdk::ResponseCode resubscribe_result) 
{
  std::cout << "*******************************************" << std::endl
            << client_id << " Resubscribe Attempted. Result" << awsiotsdk::ResponseHelper::ToString(resubscribe_result)
            << std::endl
            << "*******************************************" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}


awsiotsdk::ResponseCode Subscriber::subscribe_() 
{
  awsiotsdk::mqtt::Subscription::ApplicationCallbackHandlerPtr sub_handler 
    = std::bind(&Subscriber::subscribe_callback_,
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

awsiotsdk::ResponseCode Subscriber::unsubscribe_() 
{
  auto topic_vector = awsiotsdk::util::Vector<std::unique_ptr<awsiotsdk::Utf8String>>{};
  topic_vector.push_back(awsiotsdk::Utf8String::Create(m_topic));
  auto rc = m_iot_client->Unsubscribe(std::move(topic_vector), awsiotsdk::ConfigCommon::mqtt_command_timeout_);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  return rc;
}

awsiotsdk::ResponseCode Subscriber::initialize_TLS_() 
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
    AWS_LOG_ERROR("Onboarding::Subscriber",
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

awsiotsdk::ResponseCode Subscriber::run()
{
  auto rc = initialize_TLS_();
  if (awsiotsdk::ResponseCode::SUCCESS != rc) 
  {
    return rc;
  }

  awsiotsdk::ClientCoreState::ApplicationDisconnectCallbackPtr disconnect_handler =
      std::bind(&Subscriber::disconnect_callback_, this, std::placeholders::_1, std::placeholders::_2);

  awsiotsdk::ClientCoreState::ApplicationReconnectCallbackPtr reconnect_handler =
      std::bind(&Subscriber::reconnect_callback_,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);

  awsiotsdk::ClientCoreState::ApplicationResubscribeCallbackPtr resubscribe_handler =
      std::bind(&Subscriber::resubscribe_callback_,
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
  client_id_tagged.append("_on_boarding_subscriber_");
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
    AWS_LOG_ERROR("Onboarding::Subscriber", "Subscribe failed. %s", awsiotsdk::ResponseHelper::ToString(rc).c_str());
  } 

  std::cout << "Press any key to continue!!!!" << std::endl;
  getchar();

  rc = m_iot_client->Disconnect(awsiotsdk::ConfigCommon::mqtt_command_timeout_);
  if (awsiotsdk::ResponseCode::SUCCESS != rc) 
  {
    AWS_LOG_ERROR("Onboarding::Subscriber", "Disconnect failed. %s", awsiotsdk::ResponseHelper::ToString(rc).c_str());
  }

  std::cout << "Exiting Subscriber!!!!" << std::endl;
  return awsiotsdk::ResponseCode::SUCCESS;
}

}

int main(int argc, char **argv) 
{
  auto log_system = std::make_shared<awsiotsdk::util::Logging::ConsoleLogSystem>(awsiotsdk::util::Logging::LogLevel::Info);
  awsiotsdk::util::Logging::InitializeAWSLogging(log_system);
  auto subscriber = std::make_unique<Onboarding::Subscriber>();
  awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("config/onboarding_subscriber.json");
  if (awsiotsdk::ResponseCode::SUCCESS == rc) 
  {
    rc = subscriber->run();
  }
  awsiotsdk::util::Logging::ShutdownAWSLogging();
  return static_cast<int>(rc);
}
