#ifndef BEIGEMAZE_SEQUENCER_H_
#define BEIZEMZE_SEQUENCER_H_

#include "stmlib/stmlib.h"
#include <algorithm>
#include "peaks/gate_processor.h"
#include "peaks/chromatic_quantize.h"

namespace peaks {

class BeigeMazeSequencer {
	public:
		BeigeMazeSequencer() { }

		~BeigeMazeSequencer() { }
  
		void Init() {
			std::fill(&steps_[0], &steps_[8], 0);
			std::fill(&last_parameter_[0], &last_parameter_[4], 0);
			step_ = 0;
			frame_ = 0;
		}
  
		inline void set_step(uint8_t index, int16_t value) {
			steps_[index] = value;
		}
  
		void Configure(uint16_t* parameter, ControlMode control_mode) {
			for (int i = 0; i < 4; i++)
			{
				if (last_parameter_[i] != parameter[i]) {
					int16_t step = (frame_ * 4) + i;
					set_step(step, parameter[i] - 32768);
					last_parameter_[i] = parameter[i];
				}
			}
		}
  
		inline int16_t ProcessSingleSample(uint8_t control) {
			if (control & CONTROL_GATE_RISING) {
				++step_;
				if (step_ >= 8) step_ = 0;
			}

			if (control & CONTROL_GATE_RISING_AUXILIARY) {
				frame_++;
				if (frame_ >= 2) frame_ = 0;
			}

			if (steps_[step_] < -32700) step_ = 0;

			return peaks::chromatic_quantize(steps_[step_]);
		}
  
	private:
		uint8_t step_;
		int16_t steps_[8];
		uint16_t last_parameter_[4];
		int16_t frame_;

		DISALLOW_COPY_AND_ASSIGN(BeigeMazeSequencer);

}; // class BeigeMazeSequencer

}  // namespace peaks

#endif  // BEIGEMAZE_SEQUENCER_H_
