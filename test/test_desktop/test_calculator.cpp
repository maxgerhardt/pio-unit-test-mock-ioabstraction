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

Calculator calc;

//we need this function now for reset
void setUp(void) {
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

    ImportantBusinessLogicSwitch(IoAbstractionRef ioDevice, int pinNumber) {
        m_switch = SwitchInput(); 
        m_switch.initialise(ioDevice);
        m_pin = pinNumber;
    }

    String checkAndReact() {
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
    MockedIoAbstraction mockedInput(6);
    mockedInput.setValueForReading(0, 0x0001);
    mockedInput.setValueForReading(1, 0x0000);

    //business logic creation. 
    //assume that the input device is controllable via constructor 
    //or function.
    ImportantBusinessLogicSwitch mySwitch(&mockedInput, 0);

    //execute business logic once
    String output1 = mySwitch.checkAndReact();

    //tick
    mockedInput.runLoop();

    //execute next business logic call 
    String output2 = mySwitch.checkAndReact();

    //Verify that business logic has received correct values
    //and reacted.
    //switches are pulled by default, meaning if we return a "1" the switch is not pressed.
    //when the input goes to GND / 0, the switch is recognized as pressed.
    TEST_ASSERT_EQUAL_STRING(output1.c_str(), "PRESSED");
    TEST_ASSERT_EQUAL_STRING(output1.c_str(), "UNPRESSED");
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
    UNITY_END();

    return 0;
}
