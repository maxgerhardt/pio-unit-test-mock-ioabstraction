..  Copyright (c) 2014-present PlatformIO <contact@platformio.org>
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


PlatformIO IoAbstraction + ArduinoFake demonstration project
============================================================

See https://community.platformio.org/t/getting-started-with-unit-testing/18946 for reference.

The unit tests in `test/test_desktop` feature examples for unit tests that mock the IoAbstraction library, the Wire library and the Arduino core in general.

A small business logic class is tested that reacts on a switch, using the IoAbstraction library. It is shown how that class can be unit-tested using mocking techniques.

How to test PlatformIO based project
====================================

1. `Install PlatformIO Core <http://docs.platformio.org/page/core.html>`_
2. Download `examples source code <https://github.com/platformio/platformio-examples/archive/develop.zip>`_
3. Extract ZIP archive
4. Run these commands:

.. code-block:: bash

    # Change directory to example
    > cd platformio-examples/unit-testing/calculator

    # Test project
    > platformio test

    # Test specific environment
    > platformio test -e uno

    # Process test on native desktop machine
    > platformio test -e native
