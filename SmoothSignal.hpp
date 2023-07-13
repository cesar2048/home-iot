struct SignalAccumulator { float accumulator; float lastValue; };
struct SmoothCounter { int targetCount; int count; };

void SignalAdd(SignalAccumulator& signal, float newValue);
float SignalClose(SignalAccumulator& signal, int count);
bool CounterIncrease(SmoothCounter& counter);
