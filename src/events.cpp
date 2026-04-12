#include "events.h"

#include <freertos/semphr.h>

#include "event_loop.h"

namespace reactesp {

// Event classes define the behaviour of each particular
// Event

bool TimedEvent::operator<(const TimedEvent& other) const {
  return (this->last_trigger_time + this->interval) <
         (other.last_trigger_time + other.interval);
}

void TimedEvent::add(EventLoop* event_loop) {
  xSemaphoreTakeRecursive(event_loop->timed_queue_mutex_, portMAX_DELAY);
  event_loop->timed_events_.insert(this);
  xSemaphoreGiveRecursive(event_loop->timed_queue_mutex_);
}

void TimedEvent::remove(EventLoop* event_loop) {
  xSemaphoreTakeRecursive(event_loop->timed_queue_mutex_, portMAX_DELAY);
  auto it = event_loop->timed_events_.find(this);
  if (it != event_loop->timed_events_.end()) {
    // Event is in the container — erase and delete immediately.
    event_loop->timed_events_.erase(it);
    xSemaphoreGiveRecursive(event_loop->timed_queue_mutex_);
    delete this;
  } else {
    // Event is currently being ticked (popped from the container but
    // not yet re-inserted). Signal tick() to delete it instead of
    // re-inserting.
    this->enabled = false;
    xSemaphoreGiveRecursive(event_loop->timed_queue_mutex_);
  }
}

DelayEvent::DelayEvent(uint32_t delay, react_callback callback)
    : TimedEvent(delay, callback) {
  this->last_trigger_time = micros64();
}

DelayEvent::DelayEvent(uint64_t delay, react_callback callback)
    : TimedEvent(delay, callback) {
  this->last_trigger_time = micros64();
}

void DelayEvent::tick(EventLoop* event_loop) {
  this->last_trigger_time = micros64();
  this->callback();
  delete this;
}

void RepeatEvent::tick(EventLoop* event_loop) {
  auto now = micros64();
  this->last_trigger_time = this->last_trigger_time + this->interval;
  if (this->last_trigger_time + this->interval < now) {
    // we're lagging more than one full interval; reset the time
    this->last_trigger_time = now;
  }
  this->callback();
  // The callback may have called remove() on this event. If so,
  // enabled is now false — delete instead of re-inserting.
  if (this->enabled) {
    xSemaphoreTakeRecursive(event_loop->timed_queue_mutex_, portMAX_DELAY);
    event_loop->timed_events_.insert(this);
    xSemaphoreGiveRecursive(event_loop->timed_queue_mutex_);
  } else {
    delete this;
  }
}

void UntimedEvent::add(EventLoop* event_loop) {
  xSemaphoreTakeRecursive(event_loop->untimed_list_mutex_, portMAX_DELAY);
  event_loop->untimed_list.push_back(this);
  xSemaphoreGiveRecursive(event_loop->untimed_list_mutex_);
}

void UntimedEvent::remove(EventLoop* event_loop) {
  event_loop->remove(this);
}

void StreamEvent::tick(EventLoop* event_loop) {
  if (0 != stream.available()) {
    this->callback();
  }
}

void TickEvent::tick(EventLoop* event_loop) { this->callback(); }

#ifdef ESP32
bool ISREvent::isr_service_installed = false;

void ISREvent::isr(void* this_ptr) {
  auto* this_ = static_cast<ISREvent*>(this_ptr);
  this_->callback();
}
#endif

void ISREvent::add(EventLoop* event_loop) {
  xSemaphoreTakeRecursive(event_loop->isr_event_list_mutex_, portMAX_DELAY);
#ifdef ESP32
  gpio_isr_handler_add((gpio_num_t)pin_number, ISREvent::isr, (void*)this);
#elif defined(ESP8266)
  attachInterrupt(digitalPinToInterrupt(pin_number), callback, mode);
#endif
  event_loop->isr_event_list.push_back(this);
  xSemaphoreGiveRecursive(event_loop->isr_event_list_mutex_);
}

void ISREvent::remove(EventLoop* event_loop) {
  event_loop->remove(this);
}

}  // namespace reactesp
