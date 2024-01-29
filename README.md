# -ESP8266_Microcontroller
The project involves building a smart device using an ESP8266 microcontroller, various sensors
(DHT11 and MQ2 sensors), and MQTT protocol. The device collects data from sensors, such as
temperature, humidity, gas, and Wi-Fi information, and publishes this data to an MQTT broker.
It also subscribes to a topic to receive commands for controlling the intensity of an LED
connected to it. The project leverages Node-RED, an open-source flow-based programming tool,
to create a visual interface for monitoring and controlling the device.
##Functionalities
- [x]Sensor Data: the DHT11 measures temperature and humidity, while the MQ2 sensor
detects gas presence such as smoke, butane, propane, methane, alcohol and hydrogen in
the air.
- [x]Wi-Fi Connectivity: the device connects to a Wi-Fi network using the SSID and
password to establish communication with the MQTT broker.
- [x]MQTT Communication: The device uses the PubSubClient library to connect to an
MQTT broker, publish sensor data, and receive LED intensity commands.