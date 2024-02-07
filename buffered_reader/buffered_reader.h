/*

MIT License

Copyright (c) 2024 Erik Fast

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// Version 1.0

#pragma once

#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>

class BufferedReader {
public:
    BufferedReader() = default;

    BufferedReader(const BufferedReader &rhs) = delete;
    BufferedReader(BufferedReader &&rhs) = delete;

    ~BufferedReader() {
        close();
    }

    bool open(const char *path, size_t size_mb = 256) {
        std::unique_lock lock(mutex);

        if (state != state_uninitialized)
            return false;

        stream = std::ifstream(path, std::ios::binary);
        if (!stream)
            return false;

        buffer_size = size_mb * (1 << 20);
        buffer_size /= 2;
        if (buffer_size < 1024)
            buffer_size = 1024;

        buffer.memory = new uint8_t[buffer_size];
        back_buffer.memory = new uint8_t[buffer_size];

        state = state_running;
        read_thread = std::thread(&BufferedReader::thread_proc, this);

        return true;
    }

    void close() {
        std::unique_lock lock(mutex);

        if (state == state_uninitialized)
            return;

        state = state_canceled;
        read_thread_cv.notify_one();

        lock.unlock();
        read_thread.join();

        delete[] buffer.memory;
        delete[] back_buffer.memory;

        state = state_uninitialized;
    }

    uint8_t *swap(size_t &bytes_read) {
        std::unique_lock lock(mutex);

        if (state != state_running)
            return nullptr;

        uint8_t *result = nullptr;

        while (state == state_running && back_buffer.state != buffer_result_ready)
            main_thread_cv.wait(lock);

        if (state == state_running) {
            result = back_buffer.memory;
            bytes_read = back_buffer.bytes_read;

            if (!back_buffer.eof) {
                back_buffer.state = buffer_idle;
                std::swap(buffer, back_buffer);
            } else {
                back_buffer.bytes_read = 0;
            }

            read_thread_cv.notify_one();
        }

        return result;
    }

private:
    enum ReaderState {
        state_uninitialized,
        state_running,
        state_canceled,
        state_error
    };

    enum BufferState {
        buffer_idle,
        buffer_reading,
        buffer_result_ready,
    };

    struct Buffer {
        BufferState state = buffer_idle;
        size_t bytes_read = 0;
        uint8_t *memory = nullptr;
        bool eof = false;
    };

    void thread_proc() {
        while (true) {
            std::unique_lock lock(mutex);

            while (state == state_running && back_buffer.state != buffer_idle)
                read_thread_cv.wait(lock);

            if (state != state_running)
                break;

            back_buffer.state = buffer_reading;
            lock.unlock();

            stream.read((char *)back_buffer.memory, buffer_size);
            size_t bytes_read = stream.gcount();

            lock.lock();

            if (stream.fail() && !stream.eof()) {
                state = state_error;
            } else {
                back_buffer.bytes_read = bytes_read;
                back_buffer.state = buffer_result_ready;

                if (stream.eof())
                    back_buffer.eof = true;
            }

            main_thread_cv.notify_one();
            lock.unlock();
        }

        stream.close();
    }

    ReaderState state = state_uninitialized;
    std::ifstream stream;
    size_t buffer_size;
    Buffer buffer;
    Buffer back_buffer;
    std::thread read_thread;
    std::mutex mutex;
    std::condition_variable main_thread_cv;
    std::condition_variable read_thread_cv;
};
