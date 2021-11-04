
#include <delude88/ringbuffer.h>
#include <iostream>
#include <thread>
#include <memory>
#include <chrono>

int testNonBlockingRingBuffer() {
  auto rb = std::make_shared<NonBlockingRingBuffer>(10);

  // Write single values
  for (int i = 0; i < 100; i++) {
    float value = (float) i * 0.1f;
    rb->write(value);
    float result = rb->get();
    std::cout << result << std::endl;
    assert(result == value);
  }

  float in[4] = {1.0f, 1.1f, 1.2f, 1.3f};
  rb->write(in, 4);
  float out[4];
  rb->get(out, 4);
  assert(out[0] == 1.0f);
  assert(out[1] == 1.1f);
  assert(out[2] == 1.2f);
  assert(out[3] == 1.3f);

  // Test reset
  rb->write(5.0f);
  rb->write(5.0f);
  rb->write(5.0f);
  rb->get();
  rb->reset();
  for (int i = 0; i < 100; i++) {
    assert(rb->get() == 0);
  }
  rb->reset();
  rb->write(1.1f);
  rb->write(1.2f);
  rb->write(1.3f);
  rb->write(1.4f);
  assert(rb->get() == 1.1f);
  assert(rb->get() == 1.2f);
  assert(rb->get() == 1.3f);
  assert(rb->get() == 1.4f);
  assert(rb->get() == 0);
  assert(rb->get() == 0);
  assert(rb->get() == 0);
  assert(rb->get() == 0);
  assert(rb->get() == 0);
  assert(rb->get() == 0);

  // Write buffer, that are larger than buffer_size
  rb->reset();
  float large_in[12] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f};
  rb->write(large_in, 12);
  assert(rb->get() == 1.1f);
  assert(rb->get() == 1.2f);
  assert(rb->get() == 0.3f);
  assert(rb->get() == 0.4f);
  assert(rb->get() == 0.5f);
  assert(rb->get() == 0.6f);
  assert(rb->get() == 0.7f);
  assert(rb->get() == 0.8f);
  assert(rb->get() == 0.9f);
  assert(rb->get() == 1.0f);
  assert(rb->get() == 1.1f);
  assert(rb->get() == 1.2f);

  // Threading test
  std::thread t1([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[4] = {0.1f, 0.2f, 0.3f, 0.4f};
      rb->write(arr, 4);
    }
  });

  for (int i = 0; i < 50000; i++) {
    float arr[2];
    rb->get(arr, 2);
    //foo(arr, 2);
  }

  t1.join();

  // Threading test 2
  std::thread t2([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[4] = {0.1f, 0.2f, 0.3f, 0.4f};
      rb->write(arr, 4);
    }
  });

  std::thread t3([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[4] = {0.1f, 0.2f, 0.3f, 0.4f};
      rb->write(arr, 4);
    }
  });

  std::thread t4([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[4] = {0.1f, 0.2f, 0.3f, 0.4f};
      rb->write(arr, 4);
    }
  });

  for (int i = 0; i < 50000; i++) {
    float arr[2];
    rb->get(arr, 2);
  }

  t2.join();
  t3.join();
  t4.join();

  return 0;
}

int testThreadsafeRingBuffer() {
  auto rb = std::make_shared<ThreadsafeRingBuffer>(10);

  // Threading test
  // Writing with 3 threads and reading with 3 threads, also reading in main thread
  std::thread writer1([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[4] = {0.1f, 0.2f, 0.3f, 0.4f};
      rb->write(arr, 4);
    }
  });

  std::thread writer2([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[5] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
      rb->write(arr, 4);
    }
  });

  std::thread writer3([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[6] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
      rb->write(arr, 4);
    }
  });

  std::thread reader1([&rb]() {
    for (int i = 0; i < 50000; i++) {
      rb->get();
    }
  });

  std::thread reader2([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[2];
      rb->get(arr, 2);
    }
  });

  std::thread reader3([&rb]() {
    for (int i = 0; i < 50000; i++) {
      float arr[3];
      rb->get(arr, 3);
    }
  });

  std::thread resetter([&rb]() {
    for (int i = 0; i < 100; i++) {
      rb->reset();
    }
  });

  for (int i = 0; i < 50000; i++) {
    float arr[4];
    rb->get(arr, 4);
  }

  resetter.join();
  writer1.join();
  writer2.join();
  writer3.join();
  reader1.join();
  reader2.join();
  reader3.join();

  return 0;
}

void runBenchmark(RingBuffer &ring_buffer) {
  const int repeats = 1000;
  const std::size_t buffer_size = ring_buffer.size();

  long long duration = 0;
  long long max = 0;
  for (int i = 0; i < repeats; ++i) {
    auto t1 = std::chrono::high_resolution_clock::now();
    for (float data = 0.0; data < 10 * buffer_size; data += 1.0) { // NOLINT(cert-flp30-c,cppcoreguidelines-narrowing-conversions)
      ring_buffer.write(data);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    duration += delta;
    if (max == 0 || max < delta) {
      max = delta;
    }
  }
  std::cout << " - write: it took an average of " << duration / (10 * buffer_size * repeats) << "ns and a max of "
            << max << " ns to write " << (10 * buffer_size) << " single floats" << std::endl;

  duration = 0;
  max = 0;
  for (int i = 0; i < repeats; ++i) {
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int j = 0.0; j < 10 * buffer_size; j += 1) {
      ring_buffer.get();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    duration += delta;
    if (max == 0 || max < delta) {
      max = delta;
    }
  }
  std::cout << " - read: it took an average of " << duration / (10 * buffer_size * repeats) << "ns and a max of " << max
            << " ns to read " << (10 * buffer_size) << " single floats" << std::endl;

  duration = 0;
  max = 0;
  const std::size_t arr_size = buffer_size * 0.8;
  float arr[arr_size];
  for (int i = 0; i < arr_size; i++) {
    arr[i] = i * 0.1f;
  }
  for (int i = 0; i < repeats; i += 1) {
    auto t1 = std::chrono::high_resolution_clock::now();

    ring_buffer.write(arr, arr_size);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    duration += delta;
    if (max == 0 || max < delta) {
      max = delta;
    }
  }
  std::cout << " - write array: it took an average of " << duration / repeats << "ns and a max of " << max
            << " ns to write an array of size " << arr_size
            << std::endl;

  duration = 0;
  max = 0;
  for (int i = 0; i < repeats; i += 1) {
    auto t1 = std::chrono::high_resolution_clock::now();

    ring_buffer.get(arr, arr_size);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    duration += delta;
    if (max == 0 || max < delta) {
      max = delta;
    }
  }
  std::cout << " - read array: it took an average of " << duration / repeats << "ns and a max of " << max
            << " ns to read an array of size " << arr_size
            << std::endl;

  duration = 0;
  max = 0;
  float out[arr_size];
  for (int i = 0; i < repeats; i += 1) {
    auto t1 = std::chrono::high_resolution_clock::now();

    std::thread write([&ring_buffer, &arr, &arr_size]() {
      ring_buffer.write(arr, arr_size);
    });

    std::thread read([&ring_buffer, &out, &arr_size]() {
      ring_buffer.get(out, arr_size);
    });

    write.join();
    read.join();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    duration += delta;
    if (max == 0 || max < delta) {
      max = delta;
    }
  }
  std::cout << " - parallel: it took an average of " << duration / repeats << "ns and a max of " << max
            << " ns to read an write an array of size " << arr_size
            << " in parallel threads"
            << std::endl;
};

int main(int, char *[]) {
  int result = testNonBlockingRingBuffer();
  result += testThreadsafeRingBuffer();

  const std::size_t buffer_size = 50000;
  std::shared_ptr<RingBuffer> rb = std::make_shared<NonBlockingRingBuffer>(buffer_size);
  std::cout << "Benchmark of non-blocking ringbuffer:" << std::endl;
  runBenchmark(*rb);
  rb = std::make_shared<ThreadsafeRingBuffer>(buffer_size);
  std::cout << "Benchmark of thread-safe ringbuffer:" << std::endl;
  runBenchmark(*rb);

  return result;
}
