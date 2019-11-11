import generator as Generator
import greengrasssdk
import image_view as Image_view
import StringIO
import logging
import mandlebrot_function as Mandlebrot_function
import messages as Messages
import task_parameters as Task_parameters
import time

client = greengrasssdk.client('iot-data')

publisher_topic = 'core/publisher'


def send_to_publisher(message):
    client.publish(topic=publisher_topic, payload=message)
    print(message)

    return


subscriber_topics = \
    [
        'core/subscriber'
    ]


def send_to_subscriber(index, message):
    client.publish(topic=subscriber_topics[index], payload=message)
    print(message)

    return


def handle_message(message):
    message_type = int(message[:1])
    stream = StringIO.StringIO(message)

    if message_type == Messages.Request_message.ID:
        message_instance = Messages.Request_message.from_stream(stream)
        # tile up the request across the available subscribers
        device_step = (message_instance.header.pixel_view.bottom - message_instance.header.pixel_view.top) / len(
            subscriber_topics)
        complex_step = (message_instance.header.complex_view.bottom - message_instance.header.complex_view.top) / len(
            subscriber_topics)

        for i in range(len(subscriber_topics)):
            subscriber = subscriber_topics[i]
            new_message = message_instance.clone()
            new_message.header.pixel_view.top = message_instance.header.pixel_view.top + (i * device_step);
            new_message.header.pixel_view.bottom = new_message.header.pixel_view.top + device_step;
            new_message.header.complex_view.top = message_instance.header.complex_view.top + (i * complex_step);
            new_message.header.complex_view.bottom = new_message.header.complex_view.top + complex_step;
            send_to_subscriber(i, repr(new_message))

        return

    if message_type == Messages.Response_message.ID:
        send_to_publisher(message)
        return


#     if message_type == Messages.Cancel_message.ID:
#         message_instance = Messages.Cancel_message.from_stream(stream)
#         message = repr(message_instance).encode()
#         # pass the message onto the subscribers
#         for subscriber in subscriber_topics:
#             client.publish(subscriber, message)
#         return


def function_handler(event, context):
    # This is where the messages come in
    try:
        handle_message(str(event))
    except Exception as e:
        send_to_publisher("error: " + str(e))
        logging.info(e)

# function_handler("0\n0\n0\n0\n256\n256\n-2\n-2\n2\n2\n64\n", None)