#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <taskflow/taskflow.hpp>

// --------------------------------------------------------
// Testcase: CriticalSection
// --------------------------------------------------------

void critical_section(size_t W) {

  tf::Taskflow taskflow;
  tf::Executor executor(W);
  tf::Semaphore sema(1);
  
  REQUIRE(sema.value() == 1);
  REQUIRE(sema.max_value() == 1);

  int N = 1000;
  int counter = 0;

  for(int i=0; i<N; ++i) {
    taskflow.emplace([&](){ counter++; })
            .name(std::to_string(i))
            .acquire(sema)
            .release(sema);
  }

  executor.run(taskflow).wait();

  REQUIRE(counter == N);

  executor.run(taskflow);
  executor.run(taskflow);
  executor.run(taskflow);

  executor.wait_for_all();

  REQUIRE(counter == 4*N);
  REQUIRE(sema.value() == 1);
  REQUIRE(sema.max_value() == 1);
}

TEST_CASE("CriticalSection.1thread") {
  critical_section(1);
}

TEST_CASE("CriticalSection.2threads") {
  critical_section(2);
}

TEST_CASE("CriticalSection.3threads") {
  critical_section(3);
}

TEST_CASE("CriticalSection.7threads") {
  critical_section(7);
}

TEST_CASE("CriticalSection.11threads") {
  critical_section(11);
}

TEST_CASE("CriticalSection.16threads") {
  critical_section(16);
}

// --------------------------------------------------------
// Testcase: Semaphore
// --------------------------------------------------------

void semaphore(size_t W) {

  tf::Executor executor(W);
  tf::Taskflow taskflow;
  tf::Semaphore semaphore(1);

  int N = 1000;
  int counter = 0;

  for(int i=0; i<N; i++) {
    auto f = taskflow.emplace([&](){ counter++; });
    auto t = taskflow.emplace([&](){ counter++; });
    f.precede(t);
    f.acquire(semaphore);
    t.release(semaphore);
  }

  executor.run(taskflow).wait();

  REQUIRE(counter == 2*N);

}

TEST_CASE("Semaphore.1thread") {
  semaphore(1);
}

TEST_CASE("Semaphore.2threads") {
  semaphore(2);
}

TEST_CASE("Semaphore.4threads") {
  semaphore(4);
}

TEST_CASE("Semaphore.8threads") {
  semaphore(8);
}

// --------------------------------------------------------
// Testcase: OverlappedSemaphore
// --------------------------------------------------------

void overlapped_semaphores(size_t W) {

  tf::Executor executor(W);
  tf::Taskflow taskflow;
  tf::Semaphore semaphore1(1);
  tf::Semaphore semaphore4(4);

  int N = 1000;
  int counter = 0;

  for(int i=0; i<N; i++) {
    auto task = taskflow.emplace([&](){ counter++; });
    task.acquire(semaphore1);
    task.acquire(semaphore4);
    task.release(semaphore1);
    task.release(semaphore4);
  }

  executor.run(taskflow).wait();

  REQUIRE(counter == N);
  REQUIRE(semaphore1.value() == 1);
  REQUIRE(semaphore4.value() == 4);
}

TEST_CASE("OverlappedSemaphore.1thread") {
  overlapped_semaphores(1);
}

TEST_CASE("OverlappedSemaphore.2threads") {
  overlapped_semaphores(2);
}

TEST_CASE("OverlappedSemaphore.4threads") {
  overlapped_semaphores(4);
}

TEST_CASE("OverlappedSemaphore.8threads") {
  overlapped_semaphores(8);
}

// --------------------------------------------------------
// Testcase: Conflict Graph
// --------------------------------------------------------

void conflict_graph(size_t W) {

  tf::Executor executor(W);
  tf::Taskflow taskflow;
  tf::Semaphore conflict_AB(1);
  tf::Semaphore conflict_AC(1);

  int counter {0};
  std::mutex mutex;

  tf::Task A = taskflow.emplace([&](){ counter++; });

  // B and C can run together
  tf::Task B = taskflow.emplace([&](){
    std::lock_guard<std::mutex> lock(mutex);
    counter++;
  });
  tf::Task C = taskflow.emplace([&](){
    std::lock_guard<std::mutex> lock(mutex);
    counter++;
  });

  // describe the conflict between A and B
  A.acquire(conflict_AB).release(conflict_AB);
  B.acquire(conflict_AB).release(conflict_AB);

  // describe the conflict between A and C
  A.acquire(conflict_AC).release(conflict_AC);
  C.acquire(conflict_AC).release(conflict_AC);

  executor.run(taskflow).wait();

  REQUIRE(counter == 3);

  for(size_t i=0; i<10; i++) {
    executor.run_n(taskflow, 10);
  }
  executor.wait_for_all();

  REQUIRE(counter == 303);
}

TEST_CASE("ConflictGraph.1thread") {
  conflict_graph(1);
}

TEST_CASE("ConflictGraph.2threads") {
  conflict_graph(2);
}

TEST_CASE("ConflictGraph.3threads") {
  conflict_graph(3);
}

TEST_CASE("ConflictGraph.4threads") {
  conflict_graph(4);
}


