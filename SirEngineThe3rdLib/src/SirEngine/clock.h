#pragma once
#include <chrono>

typedef std::chrono::time_point<std::chrono::high_resolution_clock> high_res_time_point;
namespace SirEngine {
/*
the clock class is used internally in the game for sampling time, luckily in
C++ 11 we get a cross platform way to access high resolution time with the
std::chrono includes. Starting and stopping the clock is not thread-safe
*/
template <typename T> class Clock final {
public:


  //The constructor, will just the tick at which the clock got created
  Clock();

  // returns the time stamp point, does not update internal clock
  // it is thread safe
  inline high_res_time_point now() const {
    return std::chrono::high_resolution_clock::now();
  }

  // instead of returning the time stamp as now() return the ticks
  inline long long getTicks() const;

  // Returns a delta from the last time stamp taken,meaning from
  // when last time the get_delta() was called
  long long getDelta();

  // Returns the delta from the provided time_stamp and now, this version
  // of the function IS thread safe, but does not update the internal time stamp
  long long getDelta(const high_res_time_point timeStamp) const;

  // This function returns a delta from the the creation of the clock
  long long getDeltaFromOrigin() const;

  /*
  This function implements a lock type sleep for the clock,
  means it keeps the game stuck in a while loop until
  is not ready to compute the next frame
  @param amount: how much to hold the sleep, the unit
                                  is related to the clock resolution,
                                  means if you have GAME_CLOCK_RESOLUTION at
                                  nanoseconds means you are inputting
  nanoseconds
  */
  void sleep(long long amount) const;

  inline void stop() { m_running = false; };

  inline bool isRunning() const { return m_running; }

  inline void start() { m_running = true; };

private:
  // The time stamp when the clock got created
  const std::chrono::high_resolution_clock::time_point m_start_time;
  // the time stamp that get stored when get_delta gets called
  std::chrono::high_resolution_clock::time_point m_past;
  // whether the timer is running or not
  bool m_running;
};

// templated functions implementations
template <typename T>
Clock<T>::Clock()
    : m_start_time(std::chrono::high_resolution_clock::now()), m_past(m_start_time),
      m_running(true) {}

template <typename T> inline long long Clock<T>::getTicks() const {
  // if the clock is running we returns the ticks from now to the beginning of
  // time, otherwise we use the time at which the clock has been stopped
  return m_running ? std::chrono::duration_cast<T>(now().time_since_epoch()).count()
                   : std::chrono::duration_cast<T>(m_past.time_since_epoch()).count();
}

template <typename T> long long Clock<T>::getDelta() {
  // return the current delta, using either the stored time
  // if the clock is stopped or from the time now.
  const high_res_time_point curr = m_running ? now() : m_past;
  const long long duration = std::chrono::duration_cast<T>(curr - m_past).count();
  m_past = curr;
  return duration;
}

template <typename T>
long long Clock<T>::getDelta(const high_res_time_point timeStamp) const {
  // get the time
  high_res_time_point n = m_running ? now() : m_past;
  // lets extract the duration and return it
  long long duration = std::chrono::duration_cast<T>(n - timeStamp).count();
  return duration;
}

template <typename T> long long Clock<T>::getDeltaFromOrigin() const {
  const high_res_time_point curr = m_running ? now() : m_past;
  const long long duration = std::chrono::duration_cast<T>(curr - m_start_time).count();
  return duration;
}

template <typename T> void Clock<T>::sleep(const long long amount) const {
  const high_res_time_point t = now();
  while (getDelta(t) < amount)
    ;
  {}
}
// generating a game clock using the default resolution set in the constants

using GAME_CLOCK_RESOLUTION = std::chrono::nanoseconds;
typedef Clock<GAME_CLOCK_RESOLUTION> GameClock;
} // namespace SirEngine
