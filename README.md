# buell-temp-controller
C-based arudino program for controlling engine temp on a 2000 Buell X1 Lightning ECM

## Operational basics
This arduino program is designed to use an arduino's built-in serial interface to communicate with a Buell motorcycle's ECM (or Engine Control Module, the "engine computer") and pull down runtime diagnostic information, as well as turn a pair of fans (that are pointed at the two engine cylinders) on and off depending on what the current engine temperature is.

Communication (and overall program operation) can be distilled down to the following:
- Initialize the serial I/O and the LCD
- Send a runtime data header to the ECM
- Wait for a reply, read one if available or retry if we got nothing from the ECM within a timeout period
- Process the reply, printing the outputs to the LCD
- Repeat the above from the second step onwards

## Hardware used for testing and operation
The arduino in my possession is an Adafruit Pro Trinket 5V. I selected this one for a handful of reasons:
- It's tiny, so much so that it can sit right behind the LCD and not consume more overall space than the LCD itself
- It has enough I/O to handle powering a 16x2 LCD, a PWM circuit, and basic serial communication
- It has a wide voltage input range that easily handles an automotive application, and eliminates the need for additional circuits
- It's inexpensive, by far one of the cheapest options for an application such as this one

The only other notable bits are a PCF8574-based 16x2 LCD screen, which comes ready to go with the controller on-board and only requires two data wires, making it a great candidate for an arduino with minimal I/O available.

Note: I did have to increase the default size of the serial I/O buffer, otherwise the reply from the ECM gets cut off. There isn't an easy way to do this from application code, and I ran into issues attempting to pass `SERIAL_RX_BUFFER_SIZE` as a build-time override flag, so I simply modified the relevant HardwareSerial.h header file for my hardware in the arduino libraries.

## Sample output
When running, the output on the LCD looks roughly like so:
```
*----------------*
|T:185   43  41  |
|B:1250 O:49    *|
*----------------*
```

I opted to leave out the Fuel Pulsewidth labels so that there was enough room to show meaningful output. The `*` in the bottom right corner means that the fan is turned on.

There will be a flashing `X` in the bottom right corner if there is an error communicating with the ECM.

## More information
Lots of great details regarding Buell's DDFI ECMs can be found at https://ecmspy.com . This website played a large part in determining how to communicate with the ECM and making this program possible.
