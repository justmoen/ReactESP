#include "event_loop.h"

#include <freertos/semphr.h>

namespace reactesp {

void EventLoop::tickTimed() {
  xSemaphoreTakeRecursive(timed_queue_mutex_, portMAX_DELAY);
  const uint64_t now = micros64();

  while (!timed_events_.empty()) {
    auto it = timed_events_.begin();
    TimedEvent* event = *it;
    if (now < event->getTriggerTimeMicros()) {
      break;
    }
#if __cplusplus >= 201703L
    // Extract the node to avoid heap deallocation. RepeatEvent::tick()
    // will update the trigger time; we reinsert the same node afterward.
    auto node = timed_events_.extract(it);
#else
    // C++14 fallback: erase + reinsert, one allocation per RepeatEvent tick.
    timed_events_.erase(it);
#endif
    xSemaphoreGiveRecursive(timed_queue_mutex_);
    // tick() may call back into the event loop. For RepeatEvent, tick()
    // updates the trigger time. For DelayEvent, tick() sets enabled=false.
    // Either type's callback may call remove(), which also sets
    // enabled=false.
    event->tick(this);
    timed_event_counter++;
    xSemaphoreTakeRecursive(timed_queue_mutex_, portMAX_DELAY);
#if __cplusplus >= 201703L
    if (event->isEnabled() && !node.empty()) {
      // RepeatEvent: reinsert the extracted node without allocation.
      timed_events_.insert(std::move(node));
    } else {
      // DelayEvent (done) or RepeatEvent removed from within its
      // callback: drop the node and delete the event.
      delete event;
    }
#else
    if (event->isEnabled()) {
      timed_events_.insert(event);
    } else {
      delete event;
    }
#endif
  }
  xSemaphoreGiveRecursive(timed_queue_mutex_);
}

void EventLoop::tickUntimed() {
  xSemaphoreTakeRecursive(untimed_list_mutex_, portMAX_DELAY);
  for (UntimedEvent* re : this->untimed_list) {
    re->tick(this);
    untimed_event_counter++;
  }
  xSemaphoreGiveRecursive(untimed_list_mutex_);
}

void EventLoop::tick() {
  tickUntimed();
  tickTimed();
  tick_counter++;
}

DelayEvent* EventLoop::onDelay(uint32_t delay, react_callback callback) {
  auto* dre = new DelayEvent(delay, callback);
  dre->add(this);
  return dre;
}

DelayEvent* EventLoop::onDelayMicros(uint64_t delay, react_callback callback) {
  auto* dre = new DelayEvent(delay, callback);
  dre->add(this);
  return dre;
}

RepeatEvent* EventLoop::onRepeat(uint32_t interval, react_callback callback) {
  auto* rre = new RepeatEvent(interval, callback);
  rre->add(this);
  return rre;
}

RepeatEvent* EventLoop::onRepeatMicros(uint64_t interval,
                                       react_callback callback) {
  auto* rre = new RepeatEvent(interval, callback);
  rre->add(this);
  return rre;
}

StreamEvent* EventLoop::onAvailable(Stream& stream, react_callback callback) {
  auto* sre = new StreamEvent(stream, callback);
  sre->add(this);
  return sre;
}

ISREvent* EventLoop::onInterrupt(uint8_t pin_number, int mode,
                                 react_callback callback) {
  auto* isrre = new ISREvent(pin_number, mode, callback);
  isrre->add(this);
  return isrre;
}

TickEvent* EventLoop::onTick(react_callback callback) {
  auto* tre = new TickEvent(callback);
  tre->add(this);
  return tre;
}

void EventLoop::remove(TimedEvent* event) { event->remove(this); }

void EventLoop::remove(UntimedEvent* event) {
  xSemaphoreTakeRecursive(this->untimed_list_mutex_, portMAX_DELAY);
  auto it = std::find(this->untimed_list.begin(), this->untimed_list.end(), event);
  if (it != this->untimed_list.end()) {
    this->untimed_list.erase(it);
  }
  delete event;
  xSemaphoreGiveRecursive(this->untimed_list_mutex_);
}
void EventLoop::remove(ISREvent* event) {
  xSemaphoreTakeRecursive(this->isr_event_list_mutex_, portMAX_DELAY);
  auto it = std::find(this->isr_event_list.begin(), this->isr_event_list.end(), event);
  if (it != this->isr_event_list.end()) {
    this->isr_event_list.erase(it);
  }
  delete event;
  xSemaphoreGiveRecursive(this->isr_event_list_mutex_);
}

void EventLoop::remove(Event* event) { event->remove(this); }

}  // namespace reactesp
