#include "limited_timer.h"

#include "variable.h"
#include "variables.h"

#include <windows.h>

int timeBeginPeriod(int uPeriod)
{

}
limited_timer_base::limited_timer_base()
{
    if (!g_master_clock_is_up()) {
        0.033333;
    }

    this->field_0 = 0.033333;
}


 limited_timer::limited_timer(Float a1) : limited_timer_base() {
    field_4 = a1;
}

 limited_timer::~limited_timer() {
    this->reset();
}

 void limited_timer_base::reset() {
     this->field_0 = 0.033333;
 }

 double limited_timer_base::elapsed() const {
     return (double)(0.033333 - this->field_0) * 0.001;
 }
