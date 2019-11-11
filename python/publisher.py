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
import image_view as Image_view
import io
import logging
import messages as Messages
import StringIO
from threading import Lock, Thread
import time

fractal_view = None
lock = Lock()

# Read in command-line parame:wqters
parser = argparse.ArgumentParser()
parser.add_argument("-e", "--endpoint", action="store", dest="host", default="aru6hv7r865y-ats.iot.us-east-2.amazonaws.com", help="Your AWS IoT custom endpoint")
parser.add_argument("-r", "--rootCA", action="store", dest="rootCAPath", default="root-ca-cert.pem", help="Root CA file path")
parser.add_argument("-c", "--cert", action="store", dest="certificatePath", default="cf188ae534.cert.pem", help="Certificate file path")
parser.add_argument("-k", "--key", action="store", dest="privateKeyPath", default="cf188ae534.private.key", help="Private key file path")
parser.add_argument("-p", "--port", action="store", dest="port", default="8883", type=int, help="Port number override")
parser.add_argument("-w", "--websocket", action="store_true", dest="useWebsocket", default=False,
                    help="Use MQTT over WebSocket")
parser.add_argument("-id", "--clientId", action="store", dest="clientId", default="arn:aws:iot:us-east-2:754066949100:thing/Publisher",
                    help="Targeted client id")
parser.add_argument("-t", "--topic", action="store", dest="topic", default="core/publisher",
                    help="Targeted topic")
parser.add_argument("-m", "--message", action="store", dest="message", default="", help="Message Payload")
parser.add_argument("-o", "--output", action="store", dest="output", default="mandlebrot.ppm", help="PPM file to write")

args = parser.parse_args()
host = args.host
rootCAPath = args.rootCAPath
certificatePath = args.certificatePath
privateKeyPath = args.privateKeyPath
port = args.port
useWebsocket = args.useWebsocket
clientId = args.clientId
topic = args.topic
message = args.message
output = args.output

# General message notification callback
def customOnMessage(message):
    decoded = repr(message.payload).strip('\'\"')
    
    print("Received a new message: ")
    print(decoded)
    print("from topic: ")
    print(message.topic)
    print("--------------\n\n")

    if len(decoded) == 0:
        return

    if int(decoded[:1]) != Messages.Response_message.ID:
        return

    stream = StringIO.StringIO(message.payload)
    response_message = Messages.Response_message.from_stream(stream)

    lock.acquire()

    # Place into main image
    for i in range(int(response_message.header.pixel_view.width())):
        view_x_offset = int(i + response_message.header.pixel_view.left);
        for j in range(int(response_message.header.pixel_view.height())):
            view_y_offset = int(j + response_message.header.pixel_view.top);
            fractal_view.buffer.argb_buffer[int(view_x_offset + view_y_offset * fractal_view.pixel_view.height())] \
                = response_message.argb_buffer.argb_buffer[int(i + j * response_message.header.pixel_view.height())]

    # write file
    file_contents = str(fractal_view.buffer)
    file = open("mandlebrot.ppm", "w")
    file.write(file_contents)
    file.close()
    lock.release()

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
myAWSIoTMQTTClient.subscribeAsync('core/publisher', 1, ackCallback=customSubackCallback)
time.sleep(2)

pixel_view = Messages.View(0, 0, 1024, 1024)
min_real = -2.0
max_real = 2.0
min_imaginary = -2.0
max_imaginary = min_imaginary + (max_real - min_real) * (float(pixel_view.height()) / float(pixel_view.width()))
complex_view = Messages.View(min_real, min_imaginary, max_real, max_imaginary)

header = Messages.Message_header(Messages.Request_message.ID, '0')
geo_header = Messages.Geo_message_header(header, pixel_view, complex_view)
request_message = Messages.Request_message(geo_header, 500)
message = repr(request_message)

print('Sending Message...')
print(message)

buffer = [0x00000000] * int(request_message.header.pixel_view.width() * request_message.header.pixel_view.height())
argb_buffer = Messages.ARGB_buffer(buffer, request_message.header.pixel_view.width(), request_message.header.pixel_view.height())
fractal_view = Image_view.Image_view(request_message.header.pixel_view, request_message.header.complex_view, argb_buffer)

myAWSIoTMQTTClient.publishAsync("publisher/core", message, 1, ackCallback=customPubackCallback)
# myAWSIoTMQTTClient.publishAsync("publisher/cloud", message, 1, ackCallback=customPubackCallback)

while True:
    time.sleep(1)

