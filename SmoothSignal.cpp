#include "SmoothSignal.hpp"

void SignalAdd(SignalAccumulator& signal, float newValue) {
  signal.accumulator += newValue;
}

float SignalClose(SignalAccumulator& signal, int count) {
  float result = signal.accumulator / count;
  signal.accumulator = 0;
  return result;
}

bool CounterIncrease(SmoothCounter& counter) {
  bool result = false;
  counter.count += 1;
  if (counter.count == counter.targetCount) {
    result = true;
    counter.count = 0;
  }
  return result;
}
