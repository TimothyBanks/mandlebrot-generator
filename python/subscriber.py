'''
/*
 * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
 '''

from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
import argparse
import generator as Generator
import image_view as Image_view
import io
import logging
import mandlebrot_function as Mandlebrot_function
import messages as Messages
import StringIO
import task_parameters as Task_parameters
import time

fractal_view = None

# Read in command-line parameters
parser = argparse.ArgumentParser()
parser.add_argument("-e", "--endpoint", action="store", dest="host", default="aru6hv7r865y-ats.iot.us-east-2.amazonaws.com", help="Your AWS IoT custom endpoint")
parser.add_argument("-r", "--rootCA", action="store", dest="rootCAPath", default="root-ca-cert.pem", help="Root CA file path")
parser.add_argument("-c", "--cert", action="store", dest="certificatePath", default="b6cf3d2601.cert.pem", help="Certificate file path")
parser.add_argument("-k", "--key", action="store", dest="privateKeyPath", default="b6cf3d2601.private.key", help="Private key file path")
parser.add_argument("-p", "--port", action="store", dest="port", default="8883", type=int, help="Port number override")
parser.add_argument("-w", "--websocket", action="store_true", dest="useWebsocket", default=False,
        help="Use MQTT over WebSocket")
parser.add_argument("-id", "--clientId", action="store", dest="clientId", default="arn:aws:iot:us-east-2:754066949100:thing/Subscriber",
                    help="Targeted client id")
parser.add_argument("-t", "--topic", action="store", dest="topic", default="core/subscriber", help="Targeted topic")

args = parser.parse_args()
host = args.host
rootCAPath = args.rootCAPath
certificatePath = args.certificatePath
privateKeyPath = args.privateKeyPath
port = args.port
useWebsocket = args.useWebsocket
clientId = args.clientId
topic = args.topic

def onTaskCompleted(parameters):
    message_header = Messages.Message_header(Messages.Response_message.ID, parameters.identifier)
    geo_message_header = Messages.Geo_message_header(message_header, parameters.tile_pixel_view, parameters.tile_complex_view)
    argb_buffer \
        = Messages.ARGB_buffer([0x00000000] * int(parameters.tile_pixel_view.width() * parameters.tile_pixel_view.height()),
                               parameters.tile_pixel_view.width(),
                               parameters.tile_pixel_view.height())
    response_message = Messages.Response_message(geo_message_header, argb_buffer)

    for i in range(int(parameters.tile_pixel_view.width())):
        view_x_offset = i + parameters.tile_pixel_view.left
        for j in range(int(parameters.tile_pixel_view.height())):
            view_y_offset = j + parameters.tile_pixel_view.top
            response_message.argb_buffer.argb_buffer[int(i + j * parameters.tile_pixel_view.width())] \
                = fractal_view.buffer.argb_buffer[int(view_x_offset + view_y_offset * fractal_view.pixel_view.width())]

    message = repr(response_message)
    myAWSIoTMQTTClient.publishAsync("subscriber/core", message, 1)


# General message notification callback
def customOnMessage(message):
    print("Received a new message: ")
    print(message.payload)
    print("from topic: ")
    print(message.topic)
    print("--------------\n\n")

    decoded = message.payload
    message_type = int(decoded[:1])
    stream = StringIO.StringIO(decoded)

    if message_type == Messages.Request_message.ID:
        request_message = Messages.Request_message.from_stream(stream)

        argb_buffer = Messages.ARGB_buffer([0x00000000] * int(request_message.header.pixel_view.width() * request_message.header.pixel_view.height()),
                                           request_message.header.pixel_view.width(),
                                           request_message.header.pixel_view.height())
        global fractal_view
        fractal_view = Image_view.Image_view(request_message.header.pixel_view,
                                             request_message.header.complex_view,
                                             argb_buffer)
        function = Mandlebrot_function.Mandlebrot(request_message.max_iterations)
        cancel_token = Task_parameters.Token()

        tasks = Task_parameters.Generator_task_parameters.distribute(request_message.header.header.identifier,
                                                                     cancel_token,
                                                                     onTaskCompleted,
                                                                     None,
                                                                     fractal_view,
                                                                     256,
                                                                     256)
        generator = Generator.Generator()
        generator.invoke(function, tasks)
        return

    if message_type == Messages.Cancel_message.ID:
        cancel_message = Messages.Cancel_message.from_stream(stream)
        return

# Suback callback
def customSubackCallback(mid, data):
    print("Received SUBACK packet id: ")
    print(mid)
    print("Granted QoS: ")
    print(data)
    print("++++++++++++++\n\n")


# Puback callback
def customPubackCallback(mid):
    print("Received PUBACK packet id: ")
    print(mid)
    print("++++++++++++++\n\n")

if args.useWebsocket and args.certificatePath and args.privateKeyPath:
    parser.error("X.509 cert authentication and WebSocket are mutual exclusive. Please pick one.")
    exit(2)

if not args.useWebsocket and (not args.certificatePath or not args.privateKeyPath):
    parser.error("Missing credentials for authentication.")
    exit(2)

# Port defaults
if args.useWebsocket and not args.port:  # When no port override for WebSocket, default to 443
    port = 443
if not args.useWebsocket and not args.port:  # When no port override for non-WebSocket, default to 8883
    port = 8883

# Configure logging
logger = logging.getLogger("AWSIoTPythonSDK.core")
logger.setLevel(logging.DEBUG)
streamHandler = logging.StreamHandler()
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
streamHandler.setFormatter(formatter)
logger.addHandler(streamHandler)

# Init AWSIoTMQTTClient
myAWSIoTMQTTClient = None
if useWebsocket:
    myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId, useWebsocket=True)
    myAWSIoTMQTTClient.configureEndpoint(host, port)
    myAWSIoTMQTTClient.configureCredentials(rootCAPath)
else:
    myAWSIoTMQTTClient = AWSIoTMQTTClient(clientId)
    myAWSIoTMQTTClient.configureEndpoint(host, port)
    myAWSIoTMQTTClient.configureCredentials(rootCAPath, privateKeyPath, certificatePath)

# AWSIoTMQTTClient connection configuration
myAWSIoTMQTTClient.configureAutoReconnectBackoffTime(1, 32, 20)
myAWSIoTMQTTClient.configureOfflinePublishQueueing(-1)  # Infinite offline Publish queueing
myAWSIoTMQTTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
myAWSIoTMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
myAWSIoTMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec
myAWSIoTMQTTClient.onMessage = customOnMessage

# Connect and subscribe to AWS IoT
myAWSIoTMQTTClient.connect()
# Note that we are not putting a message callback here. We are using the general message notification callback.
myAWSIoTMQTTClient.subscribeAsync(topic, 1, ackCallback=customSubackCallback)
time.sleep(2)

while True:
    time.sleep(1)

