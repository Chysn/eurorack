// Copyright 2013 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// This is the common entry points for all types of modulation sources!

#ifndef PEAKS_PROCESSORS_H_
#define PEAKS_PROCESSORS_H_

#include "stmlib/stmlib.h"

#include <algorithm>

#include "peaks/drums/bass_drum.h"
#include "peaks/drums/fm_drum.h"
#include "peaks/drums/snare_drum.h"
#include "peaks/drums/high_hat.h"
#include "peaks/modulations/bouncing_ball.h"
#include "peaks/modulations/lfo.h"
//#include "peaks/modulations/mini_sequencer.h"
#include "peaks/modulations/multistage_envelope.h"
#include "peaks/number_station/number_station.h"
#include "peaks/pulse_processor/pulse_shaper.h"
#include "peaks/pulse_processor/pulse_randomizer.h"

/* Beige Maze Processors */
#include "peaks/modulations/beigemaze_palimpsest.h"
#include "peaks/modulations/beigemaze_sequencer.h"

#include "peaks/gate_processor.h"

namespace peaks {

enum ProcessorFunction {
  PROCESSOR_FUNCTION_ENVELOPE,
  PROCESSOR_FUNCTION_LFO,
  PROCESSOR_FUNCTION_TAP_LFO,
  PROCESSOR_FUNCTION_BASS_DRUM,
  PROCESSOR_FUNCTION_SNARE_DRUM,
  PROCESSOR_FUNCTION_HIGH_HAT,
  PROCESSOR_FUNCTION_FM_DRUM,
  PROCESSOR_FUNCTION_PULSE_SHAPER,
  PROCESSOR_FUNCTION_PULSE_RANDOMIZER,
  PROCESSOR_FUNCTION_BOUNCING_BALL,
  PROCESSOR_FUNCTION_MINI_SEQUENCER,
  PROCESSOR_FUNCTION_BEIGEMAZE_SEQUENCER,
  PROCESSOR_FUNCTION_NUMBER_STATION,
  PROCESSOR_FUNCTION_LAST
};

#define DECLARE_BUFFERED_PROCESSOR(ClassName, variable) \
  void ClassName ## Init() { \
    variable.Init(); \
  } \
  void ClassName ## FillBuffer() { \
    variable.FillBuffer(&input_buffer_, &output_buffer_); \
  } \
  void ClassName ## Configure(uint16_t* p, ControlMode control_mode) { \
    variable.Configure(p, control_mode); \
  } \
  ClassName variable;

#define DECLARE_UNBUFFERED_PROCESSOR(ClassName, variable) \
  void ClassName ## Init() { \
    variable.Init(); \
  } \
  int16_t ClassName ## ProcessSingleSample(uint8_t control) { \
    return variable.ProcessSingleSample(control); \
  } \
  void ClassName ## Configure(uint16_t* p, ControlMode control_mode) { \
    variable.Configure(p, control_mode); \
  } \
  ClassName variable;
  

class Processors {
 public:
  Processors() { }
  ~Processors() { }
  
  void Init(uint8_t index);
  
  typedef void (Processors::*InitFn)(); 
  typedef int16_t (Processors::*ProcessSingleSampleFn)(uint8_t); 
  typedef void (Processors::*FillBufferFn)(); 
  typedef void (Processors::*ConfigureFn)(uint16_t*, ControlMode);
  
  struct ProcessorCallbacks {
    InitFn init_fn;
    ProcessSingleSampleFn process_single_sample;
    FillBufferFn fill_buffer;
    ConfigureFn configure;
  };
  
  inline void set_control_mode(ControlMode control_mode) {
    control_mode_ = control_mode;
    Configure();
  }
  
  inline void set_parameter(uint8_t index, uint16_t parameter) {
    parameter_[index] = parameter;
    Configure();
  }
  
  inline void CopyParameters(uint16_t* parameters, uint16_t size) {
    std::copy(&parameters[0], &parameters[size], &parameter_[0]);
  }
  
  inline void set_function(ProcessorFunction function) {
    function_ = function;
    lfo_.set_sync(function == PROCESSOR_FUNCTION_TAP_LFO);
    callbacks_ = callbacks_table_[function];
    if (function != PROCESSOR_FUNCTION_TAP_LFO) {
      (this->*callbacks_.init_fn)();
    }
    Configure();
  }
  
  inline ProcessorFunction function() const { return function_; }

  inline int16_t Process(uint8_t control) {
    if (callbacks_.process_single_sample) {
      return (this->*callbacks_.process_single_sample)(control);
    } else {
      input_buffer_.Overwrite(control);
      return output_buffer_.ImmediateRead();
    }
  }
  
  inline bool Buffer() {
    if (callbacks_.fill_buffer) {
      if (output_buffer_.writable() < kBlockSize) {
        return false;
      } else {
        (this->*callbacks_.fill_buffer)();
        return true;
      }
    } else {
      return true;
    }
  }
  
  inline const NumberStation& number_station() const { return number_station_; }
  inline BeigeMazeSequencer& beigemaze_sequencer() { return beigemaze_sequencer_; } /* BeigeMaze */
  
 private:
  void Configure() {
    if (function_ == PROCESSOR_FUNCTION_SNARE_DRUM ||
        function_ == PROCESSOR_FUNCTION_HIGH_HAT) {
      /* The commented lines below serve to remove the high hat functionality from the Drum mode,
       * because it is incompatible with the calibration routine added to this branch:
       *
       * pichenettes writes on Mutable Instruments forum:
       * <<QUOTE
	   * "Peaks’ drum synthesis code is so close to filling up the CPU that small changes in compiler versions
	   * are enough to make the difference between 99.7% and 100.3% CPU used :)
       *
       * The “old” firmware (8173ac5c57d7c2c8bb04bbedb6696414b75b4cd2) does not do software offset compensation,
       * the more recent does (6165129cd99416c762da3087594d9148e4c29222) – so the older version has more “margin”
       * to tolerate variations in generated code efficiency.
       *
	   * To be on the safe side, stick with the exact same version I’m using for the production firmware,
	   * 4.5.2. Or try different optimizer flags.
	   *
	   * I know it’s silly, but that’s what happens when I try to cram as many features as possible :)
	   * UNQUOTE>>
       */
//        uint16_t tone_parameter = control_mode_ == CONTROL_MODE_FULL
//            ? parameter_[1] : parameter_[0];
//        uint16_t snappy_parameter = control_mode_ == CONTROL_MODE_FULL
//            ? parameter_[2] : parameter_[1];
//      if (tone_parameter >= 65000 && snappy_parameter >= 65000) {
//    	  if (function_ != PROCESSOR_FUNCTION_HIGH_HAT) {
//          set_function(PROCESSOR_FUNCTION_HIGH_HAT);
//        }
//      } else if (tone_parameter <= 64500 || snappy_parameter <= 64500) {
        if (function_ != PROCESSOR_FUNCTION_SNARE_DRUM) {
          set_function(PROCESSOR_FUNCTION_SNARE_DRUM);
        }
//      }
    }
    (this->*callbacks_.configure)(&parameter_[0], control_mode_);
  }
  
  InputBuffer input_buffer_;
  OutputBuffer output_buffer_;
  
  ControlMode control_mode_;
  ProcessorFunction function_;
  uint16_t parameter_[4];
  
  ProcessorCallbacks callbacks_;
  static const ProcessorCallbacks callbacks_table_[PROCESSOR_FUNCTION_LAST];
  
  DECLARE_UNBUFFERED_PROCESSOR(MultistageEnvelope, envelope_);
  DECLARE_BUFFERED_PROCESSOR(Lfo, lfo_);
  DECLARE_UNBUFFERED_PROCESSOR(BassDrum, bass_drum_);
  DECLARE_UNBUFFERED_PROCESSOR(SnareDrum, snare_drum_);
  DECLARE_UNBUFFERED_PROCESSOR(HighHat, high_hat_);
  DECLARE_BUFFERED_PROCESSOR(FmDrum, fm_drum_);
  DECLARE_BUFFERED_PROCESSOR(PulseShaper, pulse_shaper_);
  DECLARE_BUFFERED_PROCESSOR(PulseRandomizer, pulse_randomizer_);
  DECLARE_UNBUFFERED_PROCESSOR(BouncingBall, bouncing_ball_);
  DECLARE_UNBUFFERED_PROCESSOR(MiniSequencer, mini_sequencer_); /* BeigeMaze */
  DECLARE_UNBUFFERED_PROCESSOR(BeigeMazeSequencer, beigemaze_sequencer_); /* BeigeMaze */
  DECLARE_BUFFERED_PROCESSOR(NumberStation, number_station_);
  
  DISALLOW_COPY_AND_ASSIGN(Processors);
};

extern Processors processors[2];

}  // namespace peaks

#endif  // PEAKS_PROCESSORS_H_
