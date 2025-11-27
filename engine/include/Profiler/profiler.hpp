#ifndef ENGINE_PROFILER_H
#define ENGINE_PROFILER_H

#define ENABLE_PROFILING 1 //hardcoding this for now, in the future change the cmake to compile with defines instead for each build configuration

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <iomanip>
#include <deque>
#include "sink.hpp"
#include "Engine.hpp"


using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

enum class Category : uint8_t {
    Frame = 0,
    System,
    Function
};

struct Event {
    const char* name;          // static string or persisted externally
    Category category;
    uint64_t start_ns;
    uint64_t duration_ns;
    uint32_t depth;            // nesting depth per thread
    std::thread::id tid;
};

struct FrameAggregate {
    uint64_t frameIndex = 0;
    double frameMs = 0.0;
    std::unordered_map<std::string, double> systemMs; // totals per system per frame
};

class Profiler {
public:
    // Singleton accessor (for backward compatibility - uses engine profiler)
    static Profiler& instance() {
        static Profiler inst;
        return inst;
    }

    // Constructor made public for creating multiple instances
    Profiler() = default;

#if ENABLE_PROFILING
    void beginFrame(uint64_t index) {
        _frameStart = Clock::now();
        _currentFrameIndex = index;
        // Optional: clear per-frame aggregation
        _currentSystemTotals.clear();

		std::lock_guard<std::mutex> lock(_eventsMutex);
        _currentEvents.clear();
    }

    void endFrame() {
        auto end = Clock::now();
        auto frameNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - _frameStart).count();
        FrameAggregate agg;
        agg.frameIndex = _currentFrameIndex;
        agg.frameMs = frameNs / 1e6;



        // finalize system totals for this frame (copy atomics snapshot)
        {
            std::lock_guard<std::mutex> lock(_systemMutex);
            for (auto& kv : _systemTotalsAtomic) {
                agg.systemMs[kv.first] = kv.second.exchange(0.0); // consume for this frame
            }
        }

        // store frame aggregate into ring buffer
        {
            std::lock_guard<std::mutex> lock(_framesMutex);
            _frames.push_back(std::move(agg));
            if (_frames.size() > _frameHistory)
                _frames.erase(_frames.begin());
        }

        // Snapshot completed frame events for editor (double buffering)
        {
            std::lock_guard<std::mutex> lock(_eventsMutex);
            _lastCompleteFrameEvents = _currentEvents;  // Copy completed frame
        }
    }

    // Record an event (called by RAII scope)
    void recordEvent(const char* name, Category cat, uint64_t startNs, uint64_t durNs, uint32_t depth) {
        Event e{ name, cat, startNs, durNs, depth, std::this_thread::get_id() };
        {
            std::lock_guard<std::mutex> lock(_eventsMutex);

            // Circular buffer write - O(1) instead of O(n) erase!
            _events[_eventWriteIndex] = e;
            _eventWriteIndex = (_eventWriteIndex + 1) % _eventHistory;

            // Track if we've filled the buffer at least once
            if (!_eventsBufferFull && _eventWriteIndex == 0) {
                _eventsBufferFull = true;
            }

            // Current frame events still use push_back (cleared each frame)
            _currentEvents.push_back(e);
        }

        // Aggregate per-system totals each frame
        if (cat == Category::System) {
            std::lock_guard<std::mutex> lock(_systemMutex);
            _systemTotalsAtomic[std::string(name)] += durNs / 1e6; // ms
        }
    }

    // Optional: snapshot events for visualization/debug overlay
    std::vector<Event> getEventsSnapshot() const {
        std::lock_guard<std::mutex> lock(_eventsMutex);

        // Return events in chronological order (oldest to newest)
        std::vector<Event> snapshot;

        if (!_eventsBufferFull) {
            // Buffer not full yet - just return [0, _eventWriteIndex)
            snapshot.assign(_events.begin(), _events.begin() + _eventWriteIndex);
        } else {
            // Buffer is full - oldest event is at _eventWriteIndex
            snapshot.reserve(_eventHistory);
            // Copy from oldest to end of buffer
            snapshot.insert(snapshot.end(), _events.begin() + _eventWriteIndex, _events.end());
            // Copy from start to newest
            snapshot.insert(snapshot.end(), _events.begin(), _events.begin() + _eventWriteIndex);
        }

        return snapshot;
    }

    std::vector<Event> getEventCurrentFrame()
    {
        std::lock_guard<std::mutex> lock(_eventsMutex);
        return _lastCompleteFrameEvents;  // Return stable, completed frame (not in-progress)
    }

    // Optional: snapshot last N frame aggregates
    std::vector<FrameAggregate> getFramesSnapshot() const {
        std::lock_guard<std::mutex> lock(_framesMutex);
        return _frames;
    }

    FrameAggregate Get_Last_Frame() const
    {
        std::lock_guard<std::mutex> lock(_framesMutex);
        if (_frames.empty()) return FrameAggregate{};
        return _frames.back();
	}

    // Simple debug print (call occasionally)
    void printLastFrameSummary() const {
        FrameAggregate last;
        {
            std::lock_guard<std::mutex> lock(_framesMutex);
            if (_frames.empty()) return;
            last = _frames.back();
        }
        const double fps = last.frameMs > 0.0 ? 1000.0 / last.frameMs : 0.0;
        const double avgFps = getLastFps();
        Engine::GetSink()->logger()->trace("Frame {} | {:.3f} ms | {:.1f} fps (avg {:.1f} fps)", last.frameIndex, last.frameMs, fps, avgFps);

        std::ostringstream oss{};
        for (auto& kv : last.systemMs) {
            oss << "  - " << kv.first << ": " << std::setprecision(3) << kv.second << " ms\n";
        }
        Engine::GetSink()->logger()->trace("{}", oss.view());
    }

    double getLastFps(size_t window = 60) const {
        if (window == 0) {
            window = 1;
        }

        std::lock_guard<std::mutex> lock(_framesMutex);
        if (_frames.empty()) {
            return 0.0;
        }

        if (_fpsWindowSize != window) {
            _fpsWindowSize = window;
            _fpsSamples.clear();
            _fpsSum = 0.0;
            _fpsInitialized = false;
            _lastFpsFrameIndex = 0;
        }

        std::vector<const FrameAggregate*> pending;
        pending.reserve(window);

        for (auto it = _frames.rbegin(); it != _frames.rend(); ++it) {
            if (_fpsInitialized && it->frameIndex <= _lastFpsFrameIndex) {
                break;
            }
            pending.push_back(&(*it));
        }

        for (auto rit = pending.rbegin(); rit != pending.rend(); ++rit) {
            double ms = (*rit)->frameMs;
            _fpsSamples.push_back(ms);
            _fpsSum += ms;

            if (_fpsSamples.size() > _fpsWindowSize) {
                _fpsSum -= _fpsSamples.front();
                _fpsSamples.pop_front();
            }

            _lastFpsFrameIndex = (*rit)->frameIndex;
            _fpsInitialized = true;
        }

        if (_fpsSamples.empty() || _fpsSum <= 0.0) {
            double ms = _frames.back().frameMs;
            return ms > 0.0 ? 1000.0 / ms : 0.0;
        }

        double avgMs = _fpsSum / static_cast<double>(_fpsSamples.size());
        return avgMs > 0.0 ? 1000.0 / avgMs : 0.0;
    }

    // Configuration
    void setFrameHistory(size_t n) { _frameHistory = n; }
    void setEventHistory(size_t n) {
        std::lock_guard<std::mutex> lock(_eventsMutex);
        _eventHistory = n;
        _events.resize(n);  // Resize circular buffer
        _eventWriteIndex = 0;  // Reset position
        _eventsBufferFull = false;  // Reset full flag
    }

#else
    void beginFrame(uint64_t) {}
    void endFrame() {}
    void recordEvent(const char*, Category, uint64_t, uint64_t, uint32_t) {}
    std::vector<Event> getEventsSnapshot() const { return {}; }
    std::vector<FrameAggregate> getFramesSnapshot() const { return {}; }
    void setFrameHistory(size_t) {}
    void setEventHistory(size_t) {}
    void printLastFrameSummary() const {}
    double getLastFps(size_t) const { return 0.0; }
#endif

private:
#if ENABLE_PROFILING
    // Time conversion helpers
    static uint64_t toNs(TimePoint tp) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
    }

    // Frame state
    TimePoint _frameStart = Clock::now();
    uint64_t _currentFrameIndex = 0;

    // Event storage (true circular buffer - O(1) writes)
    mutable std::mutex _eventsMutex;
    std::vector<Event> _events = std::vector<Event>(100000);  // Pre-allocated circular buffer
    size_t _eventWriteIndex = 0;         // Current write position
    bool _eventsBufferFull = false;      // Whether we've wrapped around at least once
    std::vector<Event> _currentEvents;   // Per-frame events (cleared each frame, being built)
    std::vector<Event> _lastCompleteFrameEvents;  // Completed previous frame (stable for reading)
    size_t _eventHistory = 100000;

    // Frame aggregates
    mutable std::mutex _framesMutex;
    std::vector<FrameAggregate> _frames;
    size_t _frameHistory = 240;
    mutable std::deque<double> _fpsSamples;
    mutable double _fpsSum = 0.0;
    mutable size_t _fpsWindowSize = 60;
    mutable bool _fpsInitialized = false;
    mutable uint64_t _lastFpsFrameIndex = 0;

    // System aggregates for current frame
    mutable std::mutex _systemMutex;
    std::unordered_map<std::string, std::atomic<double>> _systemTotalsAtomic;
    std::unordered_map<std::string, double> _currentSystemTotals;
#endif

    friend class Scope;
    friend class SystemScope;
};

// Thread-local depth for nesting visualization
inline thread_local uint32_t t_scopeDepth = 0;

// RAII scope for arbitrary blocks/functions
class Scope {
public:
    Scope(const char* name, Category cat, Profiler& profiler)
#if ENABLE_PROFILING
        : _name(name), _cat(cat), _profiler(profiler), _start(Clock::now()), _depth(t_scopeDepth++)
#endif
    {
    }

    // Backward compatibility constructor (uses singleton)
    Scope(const char* name, Category cat)
#if ENABLE_PROFILING
        : _name(name), _cat(cat), _profiler(Profiler::instance()), _start(Clock::now()), _depth(t_scopeDepth++)
#endif
    {
    }

    ~Scope() {
#if ENABLE_PROFILING
        auto end = Clock::now();
        auto durNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - _start).count();
        _profiler.recordEvent(_name, _cat, toNs(_start), durNs, _depth);
        t_scopeDepth--;
#endif
    }
private:
#if ENABLE_PROFILING
    const char* _name;
    Category _cat;
    Profiler& _profiler;
    TimePoint _start;
    uint32_t _depth;
    static uint64_t toNs(TimePoint tp) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
    }
#endif
};

// Specialization for system-level aggregation (Category::System)
class SystemScope : public Scope {
public:
    explicit SystemScope(const char* systemName, Profiler& profiler)
        : Scope(systemName, Category::System, profiler) {
    }

    // Backward compatibility constructor (uses singleton)
    explicit SystemScope(const char* systemName)
        : Scope(systemName, Category::System) {
    }
};


// ============================================================================
// Global Profiler Instances
// ============================================================================
#if ENABLE_PROFILING
// Editor profiler instance (declared inline for header-only definition)
inline Profiler g_EditorProfiler;

// Helper to get editor profiler (for use in editor code)
inline Profiler& GetEditorProfiler() {
    return g_EditorProfiler;
}
#endif

// ============================================================================
// Convenience Macros (compile out when profiling disabled)
// ============================================================================
#if ENABLE_PROFILING
// Engine profiler macros (use singleton - backward compatible)
#define PF_BEGIN_FRAME(i) ::Profiler::instance().beginFrame(i)
#define PF_END_FRAME()    ::Profiler::instance().endFrame()
#define PF_SCOPE(name)    ::Scope scope_##__LINE__{ name, ::Category::Function }
#define PF_FUNC()         ::Scope scope_##__LINE__{ __func__, ::Category::Function }
#define PF_SYSTEM(name)   ::SystemScope sys_##__LINE__{ name }

// Editor profiler macros (use g_EditorProfiler)
#define PF_EDITOR_BEGIN_FRAME(i) ::GetEditorProfiler().beginFrame(i)
#define PF_EDITOR_END_FRAME()    ::GetEditorProfiler().endFrame()
#define PF_EDITOR_SCOPE(name)    ::Scope scope_##__LINE__{ name, ::Category::Function, ::GetEditorProfiler() }
#define PF_EDITOR_FUNC()         ::Scope scope_##__LINE__{ __func__, ::Category::Function, ::GetEditorProfiler() }
#define PF_EDITOR_SYSTEM(name)   ::SystemScope sys_##__LINE__{ name, ::GetEditorProfiler() }
#else
#define PF_BEGIN_FRAME(i) (void)0
#define PF_END_FRAME()    (void)0
#define PF_SCOPE(name)    (void)0
#define PF_FUNC()         (void)0
#define PF_SYSTEM(name)   (void)0

#define PF_EDITOR_BEGIN_FRAME(i) (void)0
#define PF_EDITOR_END_FRAME()    (void)0
#define PF_EDITOR_SCOPE(name)    (void)0
#define PF_EDITOR_FUNC()         (void)0
#define PF_EDITOR_SYSTEM(name)   (void)0
#endif

#endif