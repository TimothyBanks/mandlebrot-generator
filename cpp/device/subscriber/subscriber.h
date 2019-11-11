#pragma once

#include <unordered_map>

#include "distributed_generator.h"
#include "messages.h"
#include "mqtt/Client.hpp"
#include "NetworkConnection.hpp"
#include "task_parameters.h"

namespace Onboarding
{
class Subscriber final
{
public:
  Subscriber() = default;
  Subscriber(const Subscriber&) = default;
  Subscriber(Subscriber&&) = default;
  ~Subscriber() = default;

  Subscriber& operator=(const Subscriber&) = default;
  Subscriber& operator=(Subscriber&&) = default;

  awsiotsdk::ResponseCode run();

protected: 
  awsiotsdk::ResponseCode subscribe_();
  awsiotsdk::ResponseCode unsubscribe_();
  awsiotsdk::ResponseCode initialize_TLS_();

  awsiotsdk::ResponseCode publish_response_message_(const Fractal::Response_message& message);
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

  awsiotsdk::ResponseCode handle_request_message_(const awsiotsdk::util::String& payload);
  awsiotsdk::ResponseCode handle_cancel_message_(const awsiotsdk::util::String& payload); 

private:
  awsiotsdk::util::String m_topic;
  std::shared_ptr<awsiotsdk::MqttClient> m_iot_client;
  std::shared_ptr<awsiotsdk::NetworkConnection> m_network_connection;
  std::mutex m_mutex;
  std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> m_cancel_tokens;
  Fractal::Distributed_generator m_generator{3};
};
}