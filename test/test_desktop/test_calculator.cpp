/*
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/

#include <calculator.h>
#include <unity.h>
#include <MockIoAbstraction.h>
using namespace fakeit;

Calculator calc;

//we need this function now for reset
void setUp(void) {
    ArduinoFakeReset(); // for the two ArduinoFake based tests
    //WE NEED THIS for the TaskManagerIO in the IoAbstraction library
    //otherwise the TaskMangerIO logic will crash when it does 
    //an atomic write/read with interrupts/noInterrupts
    When(Method(ArduinoFake(), sei)).Return();
    When(Method(ArduinoFake(), cli)).Return();
}

// void tearDown(void) {
// // clean stuff up here
// }

//Simple Unit test using mocked IoAbstraction object.
//For business logic to be testable, they need to be able 
//to accept the general "BasicIoAbstraction" type.
//(or EepromAbstraction if that's used).
//highler level abstractions / objects can be created and then fed the mocked IO object with usuall  an initialise function
//example from SwitchInput
//void initialise(IoAbstractionRef ioDevice, bool usePullUpSwitching = false)
//here we're using the mock object directly.
void test_ioabstraction_mock() {
    //created mocked IO abstraction object that has
    //the same base type as BasicIoAbstraction.
    //make space for 6 timesteps. 
    MockedIoAbstraction abstr(6);

    //prepare mocked object. the first read should return that pin 0 is 0, then 1.
    //need to give the full value of the 16-bit GPIO bank for each run number
    abstr.setValueForReading(0, 0x0001);
    abstr.setValueForReading(1, 0x0000);


    //execute business logic. 
    //set pins to input, read pin 0 and react to it
    uint8_t output_value = 0x00;
    abstr.pinDirection(0, INPUT);
    bool val1 = abstr.readValue(0);
    if(val1 == true) {
        output_value = 0xff;
    }
    //simulate one tick. the readValue() function will not 
    //automatically jump to the next read value that we set, 
    //we need to 'tick' it once. practically "advance time" step.
    abstr.runLoop();
    //execute buisiness logic again..
    bool val2 = abstr.readValue(0);

    //check results of buisiness logic
    TEST_ASSERT_EQUAL(val1, true);
    TEST_ASSERT_EQUAL(output_value, 0xff);
    TEST_ASSERT_EQUAL(val2, false);
}

/* Import business logic implementation 
 * that is unit-testable
 */
class ImportantBusinessLogicSwitch {
public:
    SwitchInput m_switch;
    int m_pin;
    String outputText = "";

    //needs to be static to be usable as a function pointer..
    //doesn't accept std::function
    static void switch_pressed_cb(pinid_t key, bool heldDown) {
        //do nothing, use polling mode.
        (void)key;
        (void)heldDown;
    }

    ImportantBusinessLogicSwitch(IoAbstractionRef ioDevice, int pinNumber) {
        m_switch.initialise(ioDevice, true); /* use pullup switching */
        m_switch.addSwitch(pinNumber, &ImportantBusinessLogicSwitch::switch_pressed_cb);
        m_pin = pinNumber;
    }

    String checkAndReact() {
        //read current input and propagte to object
        //usually called by task manager but we need to call it manually here
        m_switch.runLoop();
        if(m_switch.isSwitchPressed(m_pin)) {
            outputText = "PRESSED";
        } else {
            outputText = "UNPRESSED";
        }

        return outputText;
    }
};

void test_switchinput_mock() {
    //mocked object creation
    MockedIoAbstraction mockedInput(30);
    mockedInput.resetIo();
    //input stimulation: first two reads are high,
    //next 28 are are low
    mockedInput.setValueForReading(0, 0x0001);
    mockedInput.setValueForReading(1, 0x0001);
    for(int i=2; i < 30; i++)
        mockedInput.setValueForReading(i, 0x0000);

    //business logic creation. 
    //assume that the input device is controllable via constructor 
    //or function.
    ImportantBusinessLogicSwitch mySwitch(&mockedInput, 0);

    //execute business logic once 
    String outputStart = mySwitch.checkAndReact();

    //although we did return that the input is now LOW, indicating "pressed",
    //the library internally does DEBOUNCING with a state machine.
    //we need to trigger reading a few more times (at least 3, but no more than 20 (HOLD_LIMIT) because it will recognize as "held") 
    //before it will be recognized as "pressed".
    String outputEnd = "";
    for(int i=0; i < 3; i++) {
        outputEnd = mySwitch.checkAndReact();
    }
    //now the button should definitely read as pressed after being polled through some long

    //Verify that business logic has received correct values
    //and reacted.
    //switches are pulled by default, meaning if we return a "1" the switch is not pressed.
    //when the input goes to GND / 0, the switch is recognized as pressed.
    //we can do that because the String class has overloaded equal sign operators for char*
    TEST_ASSERT_TRUE(outputStart == "UNPRESSED");
    TEST_ASSERT_TRUE(outputEnd == "PRESSED");
}

/* == Unit tests that fake input from the Arduino core and Wire library for the business logic == */

void test_simple_arduino_mock(void) {
    //setup Mock so that if pinMode() is called, we record it and return.
    When(Method(ArduinoFake(), pinMode)).Return();

    // call business logic (simplified)
    pinMode(LED_BUILTIN, OUTPUT);

    //verify
    Verify(Method(ArduinoFake(), pinMode).Using(LED_BUILTIN, OUTPUT)).Once();
}

void test_i2c_wire_mock(void) {
    //Setup
    //Mock Wire.begin() to return.
    When(OverloadedMethod(ArduinoFake(TwoWire), begin, void())).AlwaysReturn();
    //Mock Wire.available() to return 1 then 0.
    When(OverloadedMethod(ArduinoFake(TwoWire), available, int())).Return(1, 0);
    //Mock Wire.read to return 0xA once.
    When(OverloadedMethod(ArduinoFake(TwoWire), read, int())).Return(0xA);

    //execute business logic
    Wire.begin();
    int availableBytes = Wire.available(); 
    int readPayload = -1;
    while(availableBytes > 0) {
        readPayload = Wire.read(); 
        availableBytes = Wire.available();
    }

    //Check that value is as expected
    TEST_ASSERT_TRUE(readPayload == 0xA);
    //check that mocks were called expected (begin once, available check twice, read once.)
    Verify(OverloadedMethod(ArduinoFake(TwoWire), begin, void())).Once();
    Verify(OverloadedMethod(ArduinoFake(TwoWire), available, int())).Exactly(2_Times);
    Verify(OverloadedMethod(ArduinoFake(TwoWire), read, int())).Once();
}

void test_function_calculator_addition(void) {
    TEST_ASSERT_EQUAL(32, calc.add(25, 7));
}

void test_function_calculator_subtraction(void) {
    TEST_ASSERT_EQUAL(20, calc.sub(23, 3));
}

void test_function_calculator_multiplication(void) {
    TEST_ASSERT_EQUAL(50, calc.mul(25, 2));
}

void test_function_calculator_division(void) {
    TEST_ASSERT_EQUAL(32, calc.div(100, 3));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_function_calculator_addition);
    RUN_TEST(test_function_calculator_subtraction);
    RUN_TEST(test_function_calculator_multiplication);
    RUN_TEST(test_ioabstraction_mock);
    RUN_TEST(test_switchinput_mock);
    RUN_TEST(test_simple_arduino_mock);
    RUN_TEST(test_i2c_wire_mock);

    UNITY_END();

    return 0;
}
