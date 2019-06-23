# Adafruit Arcada Library [![Build Status](https://travis-ci.com/adafruit/Adafruit_TFLite_Micro_Speech.svg?branch=master)](https://travis-ci.com/adafruit/Adafruit_TFLite_Micro_Speech)

This is a 'Arduino-ified' version of the TensorFlow Lite (experimental) micro speech example. Some changes were made to the generated Arduino project file, and then we built some examples on top of it. We require pressing a button to record audio that is then analyzed - there is no example for continuous record/analyze (yet)

The Arcada/Tron demo woks best with PyGamer or PyBadge but any other Arcada board will work. Other demos will work fine on a breadboard.

SAMD51 works best, we recommend running at 200MHz and -Ofast for perky performance. 


/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
