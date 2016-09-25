/*
 * Beige Maze Palimpsest Accent Sequencer
 *
 * To activate Palimpsest, include this file instead of mini_sequencer.h in processors.h
 */

#ifndef PEAKS_MODULATIONS_MINI_SEQUENCER_H_
#define PEAKS_MODULATIONS_MINI_SEQUENCER_H_

#define PALIMPSEST_FN_LOCK 1
#define PALIMPSEST_FN_SET 2
#define PALIMPSEST_FN_WRITE 3

#include "stmlib/stmlib.h"
#include <algorithm>
#include "peaks/gate_processor.h"

namespace peaks {

class MiniSequencer {
    public:
		MiniSequencer() {
            step_ = 0;
        }

        ~MiniSequencer() { }
  
        void Init() {
            std::fill(&values_[0], &values_[16], 0);
            num_steps_ = 16;
            strength_ = 0;
            metronome_ = 0;
            function_ = PALIMPSEST_FN_WRITE;
            step_ = 0;
        }
  
        inline void set_value(uint8_t index, int16_t value) {
            values_[index] = value;
        }

        inline void add_value(uint8_t index, int16_t accent) {
        	int32_t new_value = values_[index] + accent;
        	CLIP(new_value);
        	values_[index] = new_value;
        }
  
        void Configure(uint16_t* parameter, ControlMode control_mode) {
        	/* The first knob sets how many steps the sequence has. All the way to the left, 10
        	 * steps; in the center, 12 steps; all the way to the right, 16 steps.
        	 */
        	num_steps_ = 12;
        	if (parameter[0] > 45000) num_steps_ = 16;
        	if (parameter[0] < 20000) num_steps_ = 10;

        	/* The second knob sets the function. All the way to the left, each step will be set
        	 * to a value of the third knob; in the center, each step may be written by an incoming trigger from
        	 * Trigger 2; all the way to the right, the sequence is locked for writing.
        	 */
        	function_ = PALIMPSEST_FN_WRITE;
        	if (parameter[1] > 45000) function_ = PALIMPSEST_FN_LOCK;
        	if (parameter[1] < 20000) function_ = PALIMPSEST_FN_SET;

        	/* The third knob sets the amount added to each step for an incoming Trigger 2. This amount
        	 * is bi-polar.
        	 */
        	strength_ = parameter[2] - 32768;

        	/* The fourth knob sets the strength of a pulse, on the first beat of each half of the sequence,
        	 * that can serve as a time reference.
        	 */
        	metronome_ = parameter[3] - 32878;
        }
  
        inline int16_t ProcessSingleSample(uint8_t control) {
            if (control & CONTROL_GATE_RISING) {
            	++step_; /* Advance on Trigger 1 */
                if (step_ >= num_steps_) step_ = 0;
            }

            if (control & CONTROL_GATE_RISING_AUXILIARY) {
            	/* Set accent value on Trigger 2 from the value of the second knob */
            	if (function_ == PALIMPSEST_FN_WRITE) {
            		/* If the Function knob is in the WRITE position, add the accent value to the step value */
            		add_value(step_, strength_ / 10);
            	}

                if (function_ == PALIMPSEST_FN_SET) {
                	/* If the Function knob is in the SET position, set the step value directly */
                	set_value(step_, strength_);
                }
            }

            int16_t step_pulse = (step_ % (num_steps_ / 2) == 1) ? (metronome_ / 2) : 0;
            if (step_ == 0) step_pulse = metronome_;
            int32_t step_value = values_[step_] + step_pulse;
            CLIP(step_value);

            return static_cast<int32_t>(step_value) * 40960 >> 16;
        }
  
    private:
        uint8_t step_;
        int16_t values_[16];
        uint16_t num_steps_;
        int16_t strength_;
        int16_t metronome_;
        uint16_t function_;

        DISALLOW_COPY_AND_ASSIGN(MiniSequencer);

}; // class MiniSequencer

}  // namespace peaks

#endif  // PEAKS_MODULATIONS_MINI_SEQUENCER_H_
