#pragma once

#include <mutex>
#include <unordered_map>

#include "fractal_view.h"
#include "messages.h"
#include "mqtt/Client.hpp"
#include "NetworkConnection.hpp"
#include "task_parameters.h"

namespace Onboarding
{
class Publisher final
{
public:
  Publisher() = default;
  Publisher(const Publisher&) = default;
  Publisher(Publisher&&) = default;
  ~Publisher() = default;

  Publisher& operator=(const Publisher&) = default;
  Publisher& operator=(Publisher&&) = default;

  awsiotsdk::ResponseCode run();

protected: 
  awsiotsdk::ResponseCode subscribe_();
  awsiotsdk::ResponseCode unsubscribe_();
  awsiotsdk::ResponseCode initialize_TLS_();

  awsiotsdk::ResponseCode publish_request_message_(const Fractal::Request_message& message);
  awsiotsdk::ResponseCode publish_cancel_message_(const Fractal::Cancel_message& message);
  awsiotsdk::ResponseCode subscribe_callback_(awsiotsdk::util::String topic_name,
                                              awsiotsdk::util::String payload,
                                              std::shared_ptr<awsiotsdk::mqtt::SubscriptionHandlerContextData> app_handler_data);
  awsiotsdk::ResponseCode disconnect_callback_(awsiotsdk::util::String topic_name,
                                               std::shared_ptr<awsiotsdk::DisconnectCallbackContextData> app_handler_data);
  awsiotsdk::ResponseCode reconnect_callback_(awsiotsdk::util::String client_id,
                                              std::shared_ptr<awsiotsdk::ReconnectCallbackContextData> app_handler_data,
                                              awsiotsdk::ResponseCode reconnect_result);
  awsiotsdk::ResponseCode resubscribe_callback_(awsiotsdk::util::String client_id,
                                                std::shared_ptr<awsiotsdk::ResubscribeCallbackContextData> app_handler_data,
                                                awsiotsdk::ResponseCode resubscribe_result);

  awsiotsdk::ResponseCode handle_response_message_(const awsiotsdk::util::String& payload);

private:
  awsiotsdk::util::String m_topic;
  std::shared_ptr<awsiotsdk::MqttClient> m_iot_client;
  std::shared_ptr<awsiotsdk::NetworkConnection> m_network_connection;
  std::mutex m_mutex;
  Fractal::Fractal_view m_current_view;
  std::string m_current_filename;
  std::atomic<size_t> m_current_identifier;
};
}