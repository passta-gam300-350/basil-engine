#ifndef ENGINE_SINK_H
#define ENGINE_SINK_H

#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <iostream>
#include <streambuf>
#include <vector>
#include <string>

namespace Logger {

    // A streambuf that forwards std::ostream output into spdlog
    class LogStreamBuf : public std::streambuf {
    public:
        LogStreamBuf(std::shared_ptr<spdlog::logger> logger, spdlog::level::level_enum lvl)
            : logger_(std::move(logger)), level_(lvl) {
        }

    protected:
        int overflow(int c) override {
            if (c != EOF) {
                buffer_ += static_cast<char>(c);
                if (c == '\n') {
                    flushBuffer();
                }
            }
            return c;
        }

        int sync() override {
            flushBuffer();
            return 0;
        }

    private:
        void flushBuffer() {
            if (!buffer_.empty()) {
                if (buffer_.back() == '\n') {
                    buffer_.pop_back();
                }
                logger_->log(level_, buffer_);
                buffer_.clear();
            }
        }

        std::shared_ptr<spdlog::logger> logger_;
        spdlog::level::level_enum level_;
        std::string buffer_;
    };

    // Wrapper that manages logger, multiple sinks, and optional redirection
    class Sink {
    public:
        Sink(const std::string& name, const std::string& file_path = {}, spdlog::level::level_enum lvl = spdlog::level::info)
        {
            // Console sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(lvl);
            std::vector<spdlog::sink_ptr> sinks;

            if (!file_path.empty()) {
                // File sink
                auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, true);
                file_sink->set_level(lvl);
                // Combine sinks
                sinks = { console_sink, file_sink };
            }
            else {
                sinks = { console_sink };
            }
            logger_ = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
            logger_->set_level(lvl);

            spdlog::register_logger(logger_);
        }

        std::shared_ptr<spdlog::logger> logger() { return logger_; }

        // Redirect std::cout and/or std::cerr into the logger
        void redirectStdout(spdlog::level::level_enum lvl = spdlog::level::info) {
            cout_buf_ = std::make_unique<LogStreamBuf>(logger_, lvl);
            old_cout_buf_ = std::cout.rdbuf(cout_buf_.get());
        }

        void redirectStderr(spdlog::level::level_enum lvl = spdlog::level::err) {
            cerr_buf_ = std::make_unique<LogStreamBuf>(logger_, lvl);
            old_cerr_buf_ = std::cerr.rdbuf(cerr_buf_.get());
        }

        // Restore original buffers
        void restoreStdout() {
            if (old_cout_buf_) std::cout.rdbuf(old_cout_buf_);
        }

        void restoreStderr() {
            if (old_cerr_buf_) std::cerr.rdbuf(old_cerr_buf_);
        }

        ~Sink() {
            restoreStdout();
            restoreStderr();
        }

    private:
        std::shared_ptr<spdlog::logger> logger_;
        std::unique_ptr<LogStreamBuf> cout_buf_;
        std::unique_ptr<LogStreamBuf> cerr_buf_;
        std::streambuf* old_cout_buf_ = nullptr;
        std::streambuf* old_cerr_buf_ = nullptr;
    };

} // namespace logging



#endif