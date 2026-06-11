#pragma once

#include <stdint.h>
#include <Arduino.h>

class RandomNumberLogic {
public:
    RandomNumberLogic() : min_(1), max_(100), result_(0), rolled_(false), editingMin_(true), editing_(true) {}

    void reset() {
        editing_ = true;
        editingMin_ = true;
        rolled_ = false;
    }

    void update(uint32_t deltaMs, bool click, bool longPress) {
        if (editing_) {
            if (click) {
                if (editingMin_) {
                    min_ = (min_ % 999) + 1;
                    if (min_ >= max_) max_ = min_ + 1;
                } else {
                    max_ = (max_ % 999) + 1;
                    if (max_ <= min_) max_ = min_ + 1;
                }
            }
            if (longPress) {
                if (editingMin_) {
                    editingMin_ = false;
                } else {
                    editing_ = false;
                    roll();
                }
            }
        } else {
            if (click) roll();
            if (longPress) editing_ = true;
        }
    }

    void roll() {
        result_ = random(min_, max_ + 1);
        rolled_ = true;
    }

    int getMin() const { return min_; }
    int getMax() const { return max_; }
    int getResult() const { return result_; }
    bool isRolled() const { return rolled_; }
    bool isEditing() const { return editing_; }
    bool isEditingMin() const { return editingMin_; }

private:
    int min_;
    int max_;
    int result_;
    bool rolled_;
    bool editingMin_;
    bool editing_;
};
