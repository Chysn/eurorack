#ifndef BEIGEMAZE_RIZM_H_
#define BEIZEMZE_RIZM_H_

#include "stmlib/stmlib.h"
#include <algorithm>
#include "peaks/gate_processor.h"

namespace peaks {

class BeigeMazeRizm {
	public:
		BeigeMazeRizm() {
			step_ = 0;
		}

		~BeigeMazeRizm() { }
  
		void Init() {
			std::fill(&values_[0], &values_[4], 0);
			step_ = 0;
		}
  
		inline void set_value(uint8_t index, uint16_t value) {
			values_[index] = value;
		}
  
		void Configure(uint16_t* parameter, ControlMode control_mode) {
			if (control_mode == CONTROL_MODE_HALF) {
				set_value(0, parameter[0]); /* Pattern A */
				set_value(1, parameter[1]); /* Pattern B */
				set_value(2, 8192); /* Balance A */
				set_value(3, 48192); /* Level B  */
			} else {
				set_value(0, parameter[0]); /* Pattern A */
				set_value(1, parameter[1]); /* Pattern B */
				set_value(2, parameter[2]); /* Balance A */
				set_value(3, parameter[3]); /* Level B */
			}
		}
  
		inline int16_t ProcessSingleSample(uint8_t control) {
			if (control & CONTROL_GATE_RISING) ++step_; /* Advance on Trigger 1 */
			if (control & CONTROL_GATE_RISING_AUXILIARY) step_ = 0; /* Reset on Trigger 2 */
			if (step_ >= 8) step_ = 0;

			int32_t sum = 0;

			uint16_t patternA = values_[0] / 2622; /* 2622 = 65536 / 25 patterns */
			uint16_t patternB = values_[1] / 2622;
			int16_t balanceA = values_[2] / 2;
			int16_t levelB = values_[3] - 32768;

			/*
			 * Calculate value of Pattern A for this step
			 *
			 * When the Balance knob is to the left, more of the level is placed in the
			 * first half of the sequence. When the knob is to the right, more of the
			 * level is placed in the second half of the sequence.
			 */
			if ((0x01 << step_) & accent_pattern(patternA, 0)) {
				if (step_ < 4) sum += (32767 - balanceA);
				else sum += balanceA;
			}

			/* Calculate value of Pattern B for this step
			 *
			 * Pattern B's knob is a bipolar level. While Pattern A's value is always
			 * added to the sum, Pattern B's value can add or subtract. Pattern B's
			 * level is the same throughout the pattern.
			 */
			if ((0x01 << step_) & accent_pattern(patternB, 1)) {
				sum += levelB / 2;
			}

			/* Range-checking */
			if (sum > 32767) {sum = 32767;}
			if (sum < -32768) {sum = -32768;}

		    return static_cast<int16_t>(sum);
		}
  
	private:
		uint8_t step_;
		uint16_t values_[4];

		uint16_t accent_pattern(uint16_t value, uint8_t set)
		{
			/* Convert the parameter value to a base-5 number from 0-24 */
			uint16_t high = value / 5;
			uint16_t low = value - (5 * high);
			uint16_t p1 = get_half_pattern(high, set);
			uint16_t p2 = get_half_pattern(low, set);

            uint16_t pattern = ((p2 * 16) + p1);
			return pattern;
		}

		uint16_t get_half_pattern(uint16_t value, uint8_t set)
		{
			/*
			 * There are five half-note-value patterns per set, and one of these can be selected
			 * for each half of the bar. The patterns are:
			 *
			 * SET 0:
			 *
			 * #  hex  bin
			 * 0   0  (0000): rest-rest-rest-rest
			 * 1   1  (0001): 1-rest-rest-rest
			 * 2   3  (0011): 1-and-rest-rest
			 * 3   5  (0101): 1-2
			 * 4   f  (1111): 1-and-2-and
			 *
			 * SET 1:
			 *
			 * #  hex  bin
			 * 0   0  (0000): rest-rest-rest-rest
			 * 1   8  (1000): rest-rest-rest-and
			 * 2   b  (1011): 1-and-rest-and
			 * 3   c  (1100): rest-rest-2-and
			 * 4   d  (1101): 1-rest-2-and
			 *
			 */
			uint16_t pattern = 0x00;

			if (set == 0) {
				if (value == 1) {pattern = 0x01;}
				if (value == 2) {pattern = 0x03;}
				if (value == 3) {pattern = 0x05;}
				if (value == 4) {pattern = 0x0f;}
			} else {
				if (value == 1) {pattern = 0x08;}
				if (value == 2) {pattern = 0x0b;}
				if (value == 3) {pattern = 0x0c;}
				if (value == 4) {pattern = 0x0d;}
			}

            return pattern;
		}

		DISALLOW_COPY_AND_ASSIGN(BeigeMazeRizm);

}; // class BeigeMazeRizm

}  // namespace peaks

#endif  // BEIGEMAZE_RIZM_H_
