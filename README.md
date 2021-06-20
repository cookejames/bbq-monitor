# BBQ Monitor
![Barbeque](https://user-images.githubusercontent.com/2211370/122684544-ceb79180-d1fd-11eb-8525-48d4287a4ccb.jpg)

An IoT project to automatically control the temperature of a kamado style BBQ, uploading the data to an AWS timestream database and visualising it with Grafana.

## Up close
![Up close](https://user-images.githubusercontent.com/2211370/122684543-cd866480-d1fd-11eb-8af9-584bec12553e.jpg)
The project makes use of an ESP32 microprocessor with an e-ink display (TTGO T5 2.13) connecting to a bluetooth temperature probe (Inkbird  IBT-4XS - other Inkbird thermometers should work). The temperature is controlled with a damper consisting of a 3D printed housing containing a servo and a 40mm fan.

It runs a simple PID algorithm to control the temperature but has some advanced features such as lid open detection and a startup mode to boost the light up speed.

## IoT
![Grafana](https://user-images.githubusercontent.com/2211370/122684627-60270380-d1fe-11eb-9af9-718fcb49646f.png)
The controller connects using wifi to the AWS IoT MQTT service. Data is stored in the AWS timestream database and visualised using Grafana though the controller can be fully controlled using its two buttons.

## In use
The system is quite stable when used with a powerful enough fan. Over a long period the temperature is held steady within +/- 3C. This is accurate enough for my uses so I have not tried to adjust the PID values further.

The longest cook I have performed using this was approximately 18 hours at 105C. After about 10 hours the coal basket needed fettling to get an even burn but otherwise the temperature was held steady the whole time.

## Reproduction
I have no detailed instructions at present. The code for the ESP32 is provided, it is written in C++ using the Arduino framework and is reasonably commented. 

To get started the AWS IoT infrastructure must be deployed. This uses the AWS CDK to deploy some IoT topics and the Timestream databases. All of the infrastructure is serverless and is only charged for when being used. In practice this is only when the controller is running and data is being logged to Timestream and costs less than a cent per hour. Be careful when querying Timestream to limit the time period being queried as it is easier to create multi GB queries that cost more but using the Grafana dashboard I have created which does this the cost is negligable.

Once the infrastructure has been created you must manually create an AWS IoT "Thing" with permissions and a certificate. This plus your wifi credentials must be added to a "secrets.h" file an example of which is included.

The grafana dashboard is available and runs on the free Grafana Cloud service but could equally be self hosted. Follow the Grafana Cloud documentation on creating credentials for accessing AWS Timestream using the native plugin.