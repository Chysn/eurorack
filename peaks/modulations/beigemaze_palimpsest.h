/*
 * Beige Maze Palimpsest Accent Sequencer
 *
 * To activate Palimpsest, include this file instead of mini_sequencer.h in processors.h
 */

#ifndef PEAKS_MODULATIONS_MINI_SEQUENCER_H_
#define PEAKS_MODULATIONS_MINI_SEQUENCER_H_

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
            offset_ = 0;
            in_ = 0;
            last_step_ = num_steps_;
            out_ = 0;
            step_ = 0;
        }
  
        inline void set_value(uint8_t index, int16_t value) {
            values_[index] = value;
        }

        inline void add_value(uint8_t index, int16_t accent) {
        	int32_t new_value = values_[index] + accent;
        	CLIP(new_value);
        	if (new_value < 0) {new_value = 0;}
        	values_[index] = new_value;
        }
  
        void Configure(uint16_t* parameter, ControlMode control_mode) {
        	/* The first knob sets how many steps the sequence has. All the way to the left, 10
        	 * steps; in the center, 12 steps; all the way to the right, 16 steps.
        	 */
        	num_steps_ = 16;
        	if (parameter[0] < 48864) num_steps_ = 14;
        	if (parameter[0] < 32768) num_steps_ = 12;
        	if (parameter[0] < 16384) num_steps_ = 10;

        	/* The second knob sets a bipolar offset */
        	offset_ = parameter[1] - 32768;

        	/* The third knob sets the amount added to each step while Trigger 2 is in */
        	in_ = parameter[2] / 2;

        	/* The fourth knob sets the amount subtracted from each step while Trigger 2 is out */
        	out_ = parameter[3] / 2;
        }
  
        inline int16_t ProcessSingleSample(uint8_t control) {
        	int32_t step_value = 0;

            if (control & CONTROL_GATE_RISING) {
            	++step_; /* Advance on Trigger 1 */
                if (step_ >= num_steps_) {
                	step_ = 0;
                	last_step_ = num_steps_;
                }

                if (!(control & CONTROL_GATE_AUXILIARY)) {
                	// Trigger 2 is out during this step, so substract the out_ value
                	add_value(step_, -out_);
                }
            }
            step_value = values_[step_] + offset_;

            if (control & CONTROL_GATE_AUXILIARY) {
            	// Trigger 2 is in during this step, so add the in_ value
            	if (last_step_ != step_) {
            		// but only once per step
            		add_value(step_, in_);
            		last_step_ = step_;
            	}
            	// Make the interface more responsive when the knob is in motion
            	if (in_ > values_[step_]) step_value = in_ + offset_;
            }

            CLIP(step_value);

            return static_cast<int32_t>(step_value) * 40960 >> 16;
        }
  
    private:
        uint8_t step_;
        int16_t values_[16];
        uint16_t num_steps_;
        int16_t in_;
        int16_t out_;
        int16_t last_step_;
        int16_t offset_;

        DISALLOW_COPY_AND_ASSIGN(MiniSequencer);

}; // class MiniSequencer

}  // namespace peaks

#endif  // PEAKS_MODULATIONS_MINI_SEQUENCER_H_
