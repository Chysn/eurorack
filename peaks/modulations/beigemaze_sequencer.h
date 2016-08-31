#ifndef BEIGEMAZE_SEQUENCER_H_
#define BEIZEMZE_SEQUENCER_H_

#include "stmlib/stmlib.h"
#include <algorithm>
#include "peaks/gate_processor.h"

namespace peaks {

const uint8_t beigemaze_MaxNumSteps = 4;

/* How many steps (positive and negative)? */
const uint8_t beigemaze_ChromaticRange = 13;

/* Control value at one volt (positive and negative) */
const uint16_t beigemaze_ChromaticVolt = 4200;

class BeigeMazeSequencer {
	public:
		BeigeMazeSequencer() { }

		~BeigeMazeSequencer() { }
  
		void Init() {
			std::fill(&steps_[0], &steps_[beigemaze_MaxNumSteps], 0);
			step_ = 0;
			octave_ = 1;
		}
  
		inline void set_step(uint8_t index, int16_t value) {
			steps_[index] = value;
		}
  
		void Configure(uint16_t* parameter, ControlMode control_mode) {
			if (control_mode == CONTROL_MODE_HALF) {
				set_step(0, parameter[0] - 32768);
				set_step(1, parameter[1] - 32768);
				set_step(2, -(parameter[0] - 32768));
				set_step(3, -(parameter[1] - 32768));
			} else {
				set_step(0, parameter[0] - 32768);
				set_step(1, parameter[1] - 32768);
				set_step(2, parameter[2] - 32768);
				set_step(3, parameter[3] - 32768);
			}
		}
  
		inline int16_t ProcessSingleSample(uint8_t control) {
			if (control & CONTROL_GATE_RISING) {
				++step_;
				octave_ = 1;
			}

			if (control & CONTROL_GATE_RISING_AUXILIARY) {
				if (octave_ < 0) step_++; /* Advance step every other aux trigger */
				octave_ = -octave_;
			}

			/* If the fourth knob is turned all the way to the left, use only three steps */
			int16_t num_steps = (steps_[step_] < -32700) ? 3 : 4;
			if (step_ >= num_steps) step_ = 0;

			return quantize(steps_[step_] * octave_);
		}
  
	private:
		uint8_t step_;
		int16_t steps_[beigemaze_MaxNumSteps];
		int16_t octave_;

		DISALLOW_COPY_AND_ASSIGN(BeigeMazeSequencer);

		int16_t quantize(int16_t raw) {
			int8_t ix = raw / (32768 / beigemaze_ChromaticRange);
			int32_t note; // Make it big for the fixed-point arithmetic to follow
			if (ix < 0) {ix = -ix;}
			if (ix > 12) {ix = 12;} // Range-checking

			/* (((12th root of 2) ^ n) - 1) * 100000, where n = note, 0 = tonic */
			uint32_t coeff [] = {     0,   5946,  12246,  18921,
								  25992,  33484,  41421,  48930,
								  58740,  68179,  78180,  88775,
								 100000};
			note = (coeff[ix] * beigemaze_ChromaticVolt) / coeff[12];
			if (raw < 0) note = -note;

			return static_cast<int16_t>(note);
		}

}; // class BeigeMazeSequencer

}  // namespace peaks

#endif  // BEIGEMAZE_SEQUENCER_H_
