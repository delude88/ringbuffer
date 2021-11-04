//
// Created by Tobias Hegemann on 02.11.21.
//
#pragma once

#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <atomic>

class RingBuffer {
 public:
  virtual void reset() = 0;
  virtual void write(float f) = 0;
  virtual void write(const float *in, std::size_t length) = 0;
  virtual float get() = 0;
  virtual void get(float *buffer, std::size_t length) = 0;
  virtual std::size_t size() = 0;
};

class NonBlockingRingBuffer : public RingBuffer {
 public:
  explicit NonBlockingRingBuffer(std::size_t buffer_size) :
      buffer_size_(buffer_size), read_pos_(0), write_pos_(0) {
    buffer_ptr_ = (float *) malloc(3 * buffer_size_ * sizeof(float));
    index_ptr_ = &buffer_ptr_[buffer_size_ * sizeof(float)];
  }
  ~NonBlockingRingBuffer() {
    free((void *) buffer_ptr_);
  }
  inline void reset() override {
    memset((void *) index_ptr_, 0, buffer_size_ * sizeof(float));
    write_pos_ = 0;
    read_pos_ = 0;
  }
  inline void write(float f) override {
    index_ptr_[write_pos_] = f;
    write_pos_ = (write_pos_ + 1) % buffer_size_;
  }
  inline void write(const float *in, std::size_t length) override {
    memcpy((void *) &index_ptr_[write_pos_], (void *) in, length * sizeof(float));
    memcpy((void *) &index_ptr_[write_pos_ - buffer_size_], (void *) in, length * sizeof(float));
    write_pos_ = (write_pos_ + length) % buffer_size_;
  }
  inline float get() override {
    auto f = index_ptr_[read_pos_];
    read_pos_ = (read_pos_ + 1) % buffer_size_;
    return f;
  }
  inline void get(float *buffer, std::size_t length) override {
    if (length >= buffer_size_) {
      memcpy(buffer, &index_ptr_, buffer_size_ * sizeof(float));
      read_pos_ = 0;
    } else {
      auto new_pos = (read_pos_ + length) % buffer_size_;
      if (new_pos < read_pos_) {
        // Split
        auto till_end = (buffer_size_ - read_pos_);
        memcpy(buffer, (void *) &index_ptr_[read_pos_], till_end * sizeof(float));
        memcpy(&buffer[till_end], (void *) &index_ptr_[read_pos_], new_pos * sizeof(float));
      } else {
        memcpy(buffer, (void *) &index_ptr_[read_pos_], length * sizeof(float));
      }
      read_pos_ = new_pos;
    }
  }
  inline std::size_t size() override {
    return buffer_size_;
  }
 private:
  const std::atomic<std::size_t> buffer_size_;
  volatile float *buffer_ptr_;
  volatile float *index_ptr_;
  std::atomic<std::size_t> write_pos_;
  std::atomic<std::size_t> read_pos_;
};

class ThreadsafeRingBuffer : public RingBuffer {
 public:
  explicit ThreadsafeRingBuffer(std::size_t buffer_size) :
      buffer_size_(buffer_size), read_pos_(0), write_pos_(0) {
    buffer_ptr_ = (float *) malloc(3 * buffer_size_ * sizeof(float));
    index_ptr_ = &buffer_ptr_[buffer_size_ * sizeof(float)];
  }
  ~ThreadsafeRingBuffer() {
    free((void *) buffer_ptr_);
  }
  inline void reset() override {
    memset((void *) index_ptr_, 0, buffer_size_ * sizeof(float));
    mutex_.lock();
    write_pos_ = 0;
    read_pos_ = 0;
    mutex_.unlock();
  }
  inline void write(float f) override {
    index_ptr_[write_pos_] = f;
    write_pos_ = (write_pos_ + 1) % buffer_size_;
  }
  inline void write(const float *in, std::size_t length) override {
    memcpy((void *) &index_ptr_[write_pos_], (void *) in, length * sizeof(float));
    memcpy((void *) &index_ptr_[write_pos_ - buffer_size_], (void *) in, length * sizeof(float));
    write_pos_ = (write_pos_ + length) % buffer_size_;
  }
  inline float get() override {
    mutex_.lock();
    auto f = index_ptr_[read_pos_];
    read_pos_ = (read_pos_ + 1) % buffer_size_;
    mutex_.unlock();
    return f;
  }
  inline void get(float *buffer, std::size_t length) override {
    if (length >= buffer_size_) {
      mutex_.lock();
      memcpy(buffer, &index_ptr_, buffer_size_ * sizeof(float));
      read_pos_ = 0;
      mutex_.unlock();
    } else {
      mutex_.lock();
      auto new_pos = (read_pos_ + length) % buffer_size_;
      if (new_pos < read_pos_) {
        // Split
        auto till_end = (buffer_size_ - read_pos_);
        memcpy(buffer, (void *) &index_ptr_[read_pos_], till_end * sizeof(float));
        memcpy(&buffer[till_end], (void *) &index_ptr_[read_pos_], new_pos * sizeof(float));
      } else {
        memcpy(buffer, (void *) &index_ptr_[read_pos_], length * sizeof(float));
      }
      read_pos_ = new_pos;
      mutex_.unlock();
    }
  }
  inline std::size_t size() override {
    return buffer_size_;
  }
 private:
  std::mutex mutex_;
  const std::atomic<std::size_t> buffer_size_;
  volatile float *buffer_ptr_;
  volatile float *index_ptr_;
  std::atomic<std::size_t> write_pos_;
  std::size_t read_pos_;
};