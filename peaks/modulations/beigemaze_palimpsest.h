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
            Init();
        }

        ~MiniSequencer() { }
  
        void Init() {
            std::fill(&values_[0], &values_[16], 0);
            num_steps_ = 16;
            offset_ = 0;
            compose_ = 0;
            last_step_ = 0;
            decompose_ = 0;
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
        	/* The first knob sets how many steps the sequence has
        	 * Range: 9 ~ 16
        	 */
        	num_steps_ = (parameter[0] / 8192) + 9;

        	/* The second knob sets a bipolar offset
        	 * Range: -32878 ~ 32767
        	 */
        	offset_ = parameter[1] - 32768;

        	/* The third knob sets the amount added to each step while Trigger 2 is in
        	 * Range: 0 ~ 32767
        	 */
        	compose_ = parameter[2] / 2;

        	/* The fourth knob sets the amount subtracted from each step while Trigger 2 is out
        	 * Range: 0 ~ 32767
        	 */
        	decompose_ = parameter[3] / 2;
        }
  
        inline int16_t ProcessSingleSample(uint8_t control) {
        	int32_t step_value = 0;

            if (control & CONTROL_GATE_RISING) {
            	if (!last_step_) {
            		// Trigger 2 remained out during this step, so subtract the decompose_ value
            		add_value(step_, -decompose_);
            	}

            	++step_; /* Advance on Trigger 1 */
            	last_step_ = 0; /* Reset the last step flag */
                if (step_ >= num_steps_) step_ = 0;
            }
            step_value = values_[step_] + offset_;

            if (control & CONTROL_GATE_AUXILIARY) {
            	// Trigger 2 is in during this step, so add the compose_ value
            	if (!last_step_) {
            		// but only once per step
            		add_value(step_, compose_);
            		last_step_ = 1;
            		step_value = values_[step_] + offset_;
            	}
                if (compose_ > values_[step_]) {
                	// Make the interface more responsive when the knob is in motion
                	step_value = compose_ + offset_;
                }
            }

            CLIP(step_value);

            return static_cast<int32_t>(step_value) * 40960 >> 16;
        }
  
    private:
        uint8_t step_; /* The current step number */
        int16_t values_[16]; /* Values for each step */
        uint16_t num_steps_; /* Number of steps in the sequence */
        int16_t compose_; /* Value for compose voltage */
        int16_t decompose_; /* Value for decompose voltage */
        int8_t last_step_; /* Flag: Was the compose triggered during this step? */
        int16_t offset_; /* Value for offset voltage */

        DISALLOW_COPY_AND_ASSIGN(MiniSequencer);

}; // class MiniSequencer

}  // namespace peaks

#endif  // PEAKS_MODULATIONS_MINI_SEQUENCER_H_
