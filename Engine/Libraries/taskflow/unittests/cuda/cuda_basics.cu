#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>
#include <taskflow/taskflow.hpp>

// ----------------------------------------------------------------------------
// kernel helper
// ----------------------------------------------------------------------------
template <typename T>
__global__ void k_set(T* ptr, size_t N, T value) {
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  if (i < N) {
    ptr[i] = value;
  }
}

template <typename T>
__global__ void k_single_set(T* ptr, int i, T value) {
  ptr[i] = value;
}

template <typename T>
__global__ void k_add(T* ptr, size_t N, T value) {
  int i = blockIdx.x*blockDim.x + threadIdx.x;
  if (i < N) {
    ptr[i] += value;
  }
}

template <typename T>
__global__ void k_single_add(T* ptr, int i, T value) {
  ptr[i] += value;
}

// --------------------------------------------------------
// Testcase: Builder
// --------------------------------------------------------
TEST_CASE("Builder" * doctest::timeout(300)) {

  tf::cudaGraph G;
  tf::cudaFlow cf(G, [](){ return true; });

  int source = 1;
  int target = 1;

  auto copy1 = cf.copy(&target, &source, 1).name("copy1");
  auto copy2 = cf.copy(&target, &source, 1).name("copy2");
  auto copy3 = cf.copy(&target, &source, 1).name("copy3");

  REQUIRE(copy1.name() == "copy1");
  REQUIRE(copy2.name() == "copy2");
  REQUIRE(copy3.name() == "copy3");

  REQUIRE(!copy1.empty());
  REQUIRE(!copy2.empty());
  REQUIRE(!copy3.empty());
  
  copy1.precede(copy2);
  copy2.succeed(copy3);

  REQUIRE(copy1.num_successors() == 1);
  REQUIRE(copy2.num_successors() == 0);
  REQUIRE(copy3.num_successors() == 1);
}

// --------------------------------------------------------
// Testcase: Empty
// --------------------------------------------------------

TEST_CASE("Empty" * doctest::timeout(300)) {

  std::atomic<int> counter{0};
  
  tf::Taskflow taskflow;
  tf::Executor executor;

  taskflow.emplace([&](tf::cudaFlow&){ 
    ++counter; 
  });
  
  taskflow.emplace([&](tf::cudaFlow&){ 
    ++counter; 
  });
  
  taskflow.emplace([&](tf::cudaFlow&){ 
    ++counter; 
  });

  executor.run_n(taskflow, 100).wait();

  REQUIRE(counter == 300);
}

// --------------------------------------------------------
// Testcase: Set
// --------------------------------------------------------
template <typename T>
void set() {

  for(unsigned n=1; n<=123456; n = n*2 + 1) {

    tf::Taskflow taskflow;
    tf::Executor executor;
    
    T* cpu = nullptr;
    T* gpu = nullptr;

    auto cputask = taskflow.emplace([&](){
      cpu = static_cast<T*>(std::calloc(n, sizeof(T)));
      REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
    });

    auto gputask = taskflow.emplace([&](tf::cudaFlow& cf) {
      dim3 g = {(n+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto h2d = cf.copy(gpu, cpu, n);
      auto kernel = cf.kernel(g, b, 0, k_set<T>, gpu, n, (T)17);
      auto d2h = cf.copy(cpu, gpu, n);
      h2d.precede(kernel);
      kernel.precede(d2h);
    });

    cputask.precede(gputask);
    
    executor.run(taskflow).wait();

    for(unsigned i=0; i<n; ++i) {
      REQUIRE(cpu[i] == (T)17);
    }

    std::free(cpu);
    REQUIRE(cudaFree(gpu) == cudaSuccess);
  }
}

TEST_CASE("Set.i8" * doctest::timeout(300)) {
  set<int8_t>();
}

TEST_CASE("Set.i16" * doctest::timeout(300)) {
  set<int16_t>();
}

TEST_CASE("Set.i32" * doctest::timeout(300)) {
  set<int32_t>();
}

// --------------------------------------------------------
// Testcase: Add
// --------------------------------------------------------
template <typename T>
void add() {

  for(unsigned n=1; n<=123456; n = n*2 + 1) {
   
    tf::Taskflow taskflow;
    tf::Executor executor;
    
    T* cpu = nullptr;
    T* gpu = nullptr;
    
    auto cputask = taskflow.emplace([&](){
      cpu = static_cast<T*>(std::calloc(n, sizeof(T)));
      REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
    });
    
    auto gputask = taskflow.emplace([&](tf::cudaFlow& cf){
      dim3 g = {(n+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto h2d = cf.copy(gpu, cpu, n);
      auto ad1 = cf.kernel(g, b, 0, k_add<T>, gpu, n, 1);
      auto ad2 = cf.kernel(g, b, 0, k_add<T>, gpu, n, 2);
      auto ad3 = cf.kernel(g, b, 0, k_add<T>, gpu, n, 3);
      auto ad4 = cf.kernel(g, b, 0, k_add<T>, gpu, n, 4);
      auto d2h = cf.copy(cpu, gpu, n);
      h2d.precede(ad1);
      ad1.precede(ad2);
      ad2.precede(ad3);
      ad3.precede(ad4);
      ad4.precede(d2h);
    });

    cputask.precede(gputask);
    
    executor.run(taskflow).wait();

    for(unsigned i=0; i<n; ++i) {
      REQUIRE(cpu[i] == 10);
    }

    std::free(cpu);
    REQUIRE(cudaFree(gpu) == cudaSuccess);
  }
}

TEST_CASE("Add.i8" * doctest::timeout(300)) {
  add<int8_t>();
}

TEST_CASE("Add.i16" * doctest::timeout(300)) {
  add<int16_t>();
}

TEST_CASE("Add.i32" * doctest::timeout(300)) {
  add<int32_t>();
}

// TODO: 64-bit fail?
//TEST_CASE("Add.i64" * doctest::timeout(300)) {
//  add<int64_t>();
//}


// --------------------------------------------------------
// Testcase: Binary Set
// --------------------------------------------------------
template <typename T>
void bset() {

  const unsigned n = 10000;

  tf::Taskflow taskflow;
  tf::Executor executor;

  T* cpu = nullptr;
  T* gpu = nullptr;
  
  auto cputask = taskflow.emplace([&](){
    cpu = static_cast<T*>(std::calloc(n, sizeof(T)));
    REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
  });

  auto gputask = taskflow.emplace([&](tf::cudaFlow& cf) {
    dim3 g = {1, 1, 1};
    dim3 b = {1, 1, 1};
    auto h2d = cf.copy(gpu, cpu, n);
    auto d2h = cf.copy(cpu, gpu, n);

    std::vector<tf::cudaTask> tasks(n+1);

    for(unsigned i=1; i<=n; ++i) {
      tasks[i] = cf.kernel(g, b, 0, k_single_set<T>, gpu, i-1, (T)17);

      auto p = i/2;
      if(p != 0) {
        tasks[p].precede(tasks[i]);
      }

      tasks[i].precede(d2h);
      h2d.precede(tasks[i]);
    }
  });

  cputask.precede(gputask);
  
  executor.run(taskflow).wait();

  for(unsigned i=0; i<n; ++i) {
    REQUIRE(cpu[i] == (T)17);
  }

  std::free(cpu);
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

TEST_CASE("BSet.i8" * doctest::timeout(300)) {
  bset<int8_t>();
}

TEST_CASE("BSet.i16" * doctest::timeout(300)) {
  bset<int16_t>();
}

TEST_CASE("BSet.i32" * doctest::timeout(300)) {
  bset<int32_t>();
}

// --------------------------------------------------------
// Testcase: Memset
// --------------------------------------------------------
TEST_CASE("Memset") {
  
  tf::Taskflow taskflow;
  tf::Executor executor;
  
  const int N = 100;

  int* cpu = new int [N];
  int* gpu = nullptr;
    
  REQUIRE(cudaMalloc(&gpu, N*sizeof(int)) == cudaSuccess);

  for(int r=1; r<=100; ++r) {

    int start = ::rand() % N;

    for(int i=0; i<N; ++i) {
      cpu[i] = 999;
    }
    
    taskflow.emplace([&](tf::cudaFlow& cf){
      dim3 g = {(unsigned)(N+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto kset = cf.kernel(g, b, 0, k_set<int>, gpu, N, 123);
      auto zero = cf.memset(gpu+start, 0x3f, (N-start)*sizeof(int));
      auto copy = cf.copy(cpu, gpu, N);
      kset.precede(zero);
      zero.precede(copy);
    });
    
    executor.run(taskflow).wait();

    for(int i=0; i<start; ++i) {
      REQUIRE(cpu[i] == 123);
    }
    for(int i=start; i<N; ++i) {
      REQUIRE(cpu[i] == 0x3f3f3f3f);
    }
  }
  
  delete [] cpu;
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

// --------------------------------------------------------
// Testcase: Memset0
// --------------------------------------------------------
template <typename T>
void memset0() {
  
  tf::Taskflow taskflow;
  tf::Executor executor;
  
  const int N = 97;

  T* cpu = new T [N];
  T* gpu = nullptr;
    
  REQUIRE(cudaMalloc(&gpu, N*sizeof(T)) == cudaSuccess);

  for(int r=1; r<=100; ++r) {

    int start = ::rand() % N;

    for(int i=0; i<N; ++i) {
      cpu[i] = (T)999;
    }
    
    taskflow.emplace([&](tf::cudaFlow& cf){
      dim3 g = {(unsigned)(N+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto kset = cf.kernel(g, b, 0, k_set<T>, gpu, N, (T)123);
      auto zero = cf.memset(gpu+start, (T)0, (N-start)*sizeof(T));
      auto copy = cf.copy(cpu, gpu, N);
      kset.precede(zero);
      zero.precede(copy);
    });
    
    executor.run(taskflow).wait();

    for(int i=0; i<start; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)123) < 1e-4);
    }
    for(int i=start; i<N; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)0) < 1e-4);
    }
  }
  
  delete [] cpu;
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

TEST_CASE("Memset0.i8") {
  memset0<int8_t>();
}

TEST_CASE("Memset0.i16") {
  memset0<int16_t>();
}

TEST_CASE("Memset0.i32") {
  memset0<int32_t>();
}

TEST_CASE("Memset0.f32") {
  memset0<float>();
}

TEST_CASE("Memset0.f64") {
  memset0<double>();
}

// --------------------------------------------------------
// Testcase: Memcpy
// --------------------------------------------------------
template <typename T>
void memcpy() {
  
  tf::Taskflow taskflow;
  tf::Executor executor;
  
  const int N = 97;

  T* cpu = new T [N];
  T* gpu = nullptr;
    
  REQUIRE(cudaMalloc(&gpu, N*sizeof(T)) == cudaSuccess);

  for(int r=1; r<=100; ++r) {

    int start = ::rand() % N;

    for(int i=0; i<N; ++i) {
      cpu[i] = (T)999;
    }
    
    taskflow.emplace([&](tf::cudaFlow& cf){
      dim3 g = {(unsigned)(N+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto kset = cf.kernel(g, b, 0, k_set<T>, gpu, N, (T)123);
      auto zero = cf.memset(gpu+start, (T)0, (N-start)*sizeof(T));
      auto copy = cf.memcpy(cpu, gpu, N*sizeof(T));
      kset.precede(zero);
      zero.precede(copy);
    });
    
    executor.run(taskflow).wait();

    for(int i=0; i<start; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)123) < 1e-4);
    }
    for(int i=start; i<N; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)0) < 1e-4);
    }
  }
  
  delete [] cpu;
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

TEST_CASE("Memcpy.i8") {
  memcpy<int8_t>();
}

TEST_CASE("Memcpy.i16") {
  memcpy<int16_t>();
}

TEST_CASE("Memcpy.i32") {
  memcpy<int32_t>();
}

TEST_CASE("Memcpy.f32") {
  memcpy<float>();
}

TEST_CASE("Memcpy.f64") {
  memcpy<double>();
}

// --------------------------------------------------------
// Testcase: fill
// --------------------------------------------------------
template <typename T>
void fill(T value) {
  
  tf::Taskflow taskflow;
  tf::Executor executor;
  
  const int N = 107;

  T* cpu = new T [N];
  T* gpu = nullptr;
    
  REQUIRE(cudaMalloc(&gpu, N*sizeof(T)) == cudaSuccess);

  for(int r=1; r<=100; ++r) {

    int start = ::rand() % N;

    for(int i=0; i<N; ++i) {
      cpu[i] = (T)999;
    }
    
    taskflow.emplace([&](tf::cudaFlow& cf){
      dim3 g = {(unsigned)(N+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto kset = cf.kernel(g, b, 0, k_set<T>, gpu, N, (T)123);
      auto fill = cf.fill(gpu+start, value, (N-start));
      auto copy = cf.copy(cpu, gpu, N);
      kset.precede(fill);
      fill.precede(copy);
    });
    
    executor.run(taskflow).wait();

    for(int i=0; i<start; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)123) < 1e-4);
    }
    for(int i=start; i<N; ++i) {
      REQUIRE(std::fabs(cpu[i] - value) < 1e-4);
    }
  }

  delete [] cpu;
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

TEST_CASE("Fill.i8") {
  fill<int8_t>(+123);
  fill<int8_t>(-123);
}

TEST_CASE("Fill.i16") {
  fill<int16_t>(+12345);
  fill<int16_t>(-12345);
}

TEST_CASE("Fill.i32") {
  fill<int32_t>(+123456789);
  fill<int32_t>(-123456789);
}

TEST_CASE("Fill.f32") {
  fill<float>(+123456789.0f);
  fill<float>(-123456789.0f);
}

// --------------------------------------------------------
// Testcase: Zero
// --------------------------------------------------------
template <typename T>
void zero() {
  
  tf::Taskflow taskflow;
  tf::Executor executor;
  
  const int N = 100;

  T* cpu = new T [N];
  T* gpu = nullptr;
    
  REQUIRE(cudaMalloc(&gpu, N*sizeof(T)) == cudaSuccess);

  for(int r=1; r<=100; ++r) {

    int start = ::rand() % N;

    for(int i=0; i<N; ++i) {
      cpu[i] = (T)999;
    }
    
    taskflow.emplace([&](tf::cudaFlow& cf){
      dim3 g = {(unsigned)(N+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto kset = cf.kernel(g, b, 0, k_set<T>, gpu, N, (T)123);
      auto zero = cf.zero(gpu+start, (N-start));
      auto copy = cf.copy(cpu, gpu, N);
      kset.precede(zero);
      zero.precede(copy);
    });
    
    executor.run(taskflow).wait();

    for(int i=0; i<start; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)123) < 1e-4);
    }
    for(int i=start; i<N; ++i) {
      REQUIRE(std::fabs(cpu[i] - (T)0) < 1e-4);
    }
  }

  delete [] cpu;
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

TEST_CASE("Zero.i8") {
  zero<int8_t>();
}

TEST_CASE("Zero.i16") {
  zero<int16_t>();
}

TEST_CASE("Zero.i32") {
  zero<int32_t>();
}

TEST_CASE("Zero.f32") {
  zero<float>();
}

// --------------------------------------------------------
// Testcase: Barrier
// --------------------------------------------------------
template <typename T>
void barrier() {

  const unsigned n = 1000;
  
  tf::Taskflow taskflow;
  tf::Executor executor;
  
  T* cpu = nullptr;
  T* gpu = nullptr;

  auto cputask = taskflow.emplace([&](){
    cpu = static_cast<T*>(std::calloc(n, sizeof(T)));
    REQUIRE(cudaMalloc(&gpu, n*sizeof(T)) == cudaSuccess);
  });

  auto gputask = taskflow.emplace([&](tf::cudaFlow& cf) {

    dim3 g = {1, 1, 1};
    dim3 b = {1, 1, 1};
    auto br1 = cf.noop();
    auto br2 = cf.noop();
    auto br3 = cf.noop();
    auto h2d = cf.copy(gpu, cpu, n);
    auto d2h = cf.copy(cpu, gpu, n);

    h2d.precede(br1);

    for(unsigned i=0; i<n; ++i) {
      auto k1 = cf.kernel(g, b, 0, k_single_set<T>, gpu, i, (T)17);
      k1.succeed(br1)
        .precede(br2);

      auto k2 = cf.kernel(g, b, 0, k_single_add<T>, gpu, i, (T)3);
      k2.succeed(br2)
        .precede(br3);
    }

    br3.precede(d2h);
  });

  cputask.precede(gputask);
  
  executor.run(taskflow).wait();

  for(unsigned i=0; i<n; ++i) {
    REQUIRE(cpu[i] == (T)20);
  }

  std::free(cpu);
  REQUIRE(cudaFree(gpu) == cudaSuccess);
}

TEST_CASE("Barrier.i8" * doctest::timeout(300)) {
  barrier<int8_t>();
}

TEST_CASE("Barrier.i16" * doctest::timeout(300)) {
  barrier<int16_t>();
}

TEST_CASE("Barrier.i32" * doctest::timeout(300)) {
  barrier<int32_t>();
}

// ----------------------------------------------------------------------------
// NestedRuns
// ----------------------------------------------------------------------------

TEST_CASE("NestedRuns") {
  
  int* cpu = nullptr;
  int* gpu = nullptr;

  constexpr unsigned n = 1000;

  cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
  REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);

  struct A {

    tf::Executor executor;
    tf::Taskflow taskflow;

    void run(int* cpu, int* gpu, unsigned n) {
      taskflow.clear();

      auto A1 = taskflow.emplace([&](tf::cudaFlow& cf) {  
        cf.copy(gpu, cpu, n);
      });

      auto A2 = taskflow.emplace([&](tf::cudaFlow& cf) { 
        dim3 g = {(n+255)/256, 1, 1};
        dim3 b = {256, 1, 1};
        cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
      });

      auto A3 = taskflow.emplace([&] (tf::cudaFlow& cf) {
        cf.copy(cpu, gpu, n);
      });

      A1.precede(A2);
      A2.precede(A3);

      executor.run_n(taskflow, 10).wait();
    }

  };
  
  struct B {

    tf::Taskflow taskflow;
    tf::Executor executor;

    A a;

    void run(int* cpu, int* gpu, unsigned n) {

      taskflow.clear();
      
      auto B0 = taskflow.emplace([] () {});
      auto B1 = taskflow.emplace([&] (tf::cudaFlow& cf) { 
        dim3 g = {(n+255)/256, 1, 1};
        dim3 b = {256, 1, 1};
        auto h2d = cf.copy(gpu, cpu, n);
        auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
        auto d2h = cf.copy(cpu, gpu, n);
        h2d.precede(kernel);
        kernel.precede(d2h);
      });
      auto B2 = taskflow.emplace([&] () { a.run(cpu, gpu, n); });
      auto B3 = taskflow.emplace([&] (tf::cudaFlow&) { 
        for(unsigned i=0; i<n; ++i) {
          cpu[i]++;
        }
      });
      
      B0.precede(B1);
      B1.precede(B2);
      B2.precede(B3);

      executor.run_n(taskflow, 100).wait();
    }
  };

  B b;
  b.run(cpu, gpu, n);

  for(unsigned i=0; i<n; i++) {
    REQUIRE(cpu[i] == 1200);
  }
    
  REQUIRE(cudaFree(gpu) == cudaSuccess);
  std::free(cpu);
}

// ----------------------------------------------------------------------------
// WorkerID
// ----------------------------------------------------------------------------

void worker_id(unsigned N, unsigned M) {
  
  tf::Taskflow taskflow;
  tf::Executor executor(N, M);

  REQUIRE(executor.num_workers() == (N + M));
  REQUIRE(executor.num_domains() == 2);

  const unsigned s = 1000;

  for(unsigned k=0; k<s; ++k) {
    
    auto cputask = taskflow.emplace([&](){
      auto id = executor.this_worker_id();
      REQUIRE(id >= 0);
      REQUIRE(id <  N);
    });
    
    auto gputask = taskflow.emplace([&](tf::cudaFlow&) {
      auto id = executor.this_worker_id();
      REQUIRE(id >= N);
      REQUIRE(id <  N+M);
    });

    auto chktask = taskflow.emplace([&] () {
      auto id = executor.this_worker_id();
      REQUIRE(id >= 0);
      REQUIRE(id <  N);
    });
    
    taskflow.emplace([&](tf::cudaFlow&) {
      auto id = executor.this_worker_id();
      REQUIRE(id >= N);
      REQUIRE(id <  N+M);
    });
    
    taskflow.emplace([&]() {
      auto id = executor.this_worker_id();
      REQUIRE(id >= 0);
      REQUIRE(id <  N);
    });

    auto subflow = taskflow.emplace([&](tf::Subflow& sf){
      auto id = executor.this_worker_id();
      REQUIRE(id >= 0);
      REQUIRE(id <  N);
      auto t1 = sf.emplace([&](){
        auto id = executor.this_worker_id();
        REQUIRE(id >= 0);
        REQUIRE(id <  N);
      });
      auto t2 = sf.emplace([&](tf::cudaFlow&){
        auto id = executor.this_worker_id();
        REQUIRE(id >= N);
        REQUIRE(id <  N+M);
      });
      t1.precede(t2);
    });

    cputask.precede(gputask);
    gputask.precede(chktask);
    chktask.precede(subflow);
  }

  executor.run_n(taskflow, 100).wait();
}

TEST_CASE("WorkerID.1C1G") {
  worker_id(1, 1);
}

TEST_CASE("WorkerID.1C2G") {
  worker_id(1, 2);
}

TEST_CASE("WorkerID.1C3G") {
  worker_id(1, 3);
}

TEST_CASE("WorkerID.1C4G") {
  worker_id(1, 4);
}

TEST_CASE("WorkerID.2C1G") {
  worker_id(2, 1);
}

TEST_CASE("WorkerID.2C2G") {
  worker_id(2, 2);
}

TEST_CASE("WorkerID.2C3G") {
  worker_id(2, 3);
}

TEST_CASE("WorkerID.2C4G") {
  worker_id(2, 4);
}

TEST_CASE("WorkerID.3C1G") {
  worker_id(3, 1);
}

TEST_CASE("WorkerID.3C2G") {
  worker_id(3, 2);
}

TEST_CASE("WorkerID.3C3G") {
  worker_id(3, 3);
}

TEST_CASE("WorkerID.3C4G") {
  worker_id(3, 4);
}

TEST_CASE("WorkerID.4C1G") {
  worker_id(4, 1);
}

TEST_CASE("WorkerID.4C2G") {
  worker_id(4, 2);
}

TEST_CASE("WorkerID.4C3G") {
  worker_id(4, 3);
}

TEST_CASE("WorkerID.4C4G") {
  worker_id(4, 4);
}

// ----------------------------------------------------------------------------
// Multiruns
// ----------------------------------------------------------------------------

void multiruns(unsigned N, unsigned M) {

  tf::Taskflow taskflow;
  tf::Executor executor(N, M);

  const unsigned n = 1000;
  const unsigned s = 1000;

  int *cpu[s] = {0};
  int *gpu[s] = {0};

  for(unsigned k=0; k<s; ++k) {
    
    int number = ::rand()%100;

    auto cputask = taskflow.emplace([&, k](){
      cpu[k] = static_cast<int*>(std::calloc(n, sizeof(int)));
      REQUIRE(cudaMalloc(&gpu[k], n*sizeof(int)) == cudaSuccess);
    });
    
    auto gputask = taskflow.emplace([&, k, number](tf::cudaFlow& cf) {
      dim3 g = {(n+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto h2d = cf.copy(gpu[k], cpu[k], n);
      auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu[k], n, number);
      auto d2h = cf.copy(cpu[k], gpu[k], n);
      h2d.precede(kernel);
      kernel.precede(d2h);
    });

    auto chktask = taskflow.emplace([&, k, number] () {
      for(unsigned i=0; i<n; ++i) {
        REQUIRE(cpu[k][i] == number);
      }
    });

    cputask.precede(gputask);
    gputask.precede(chktask);

  }

  executor.run(taskflow).wait();
}

TEST_CASE("Multiruns.1C1G") {
  multiruns(1, 1);
}

TEST_CASE("Multiruns.1C2G") {
  multiruns(1, 2);
}

TEST_CASE("Multiruns.1C3G") {
  multiruns(1, 3);
}

TEST_CASE("Multiruns.1C4G") {
  multiruns(1, 4);
}

TEST_CASE("Multiruns.2C1G") {
  multiruns(2, 1);
}

TEST_CASE("Multiruns.2C2G") {
  multiruns(2, 2);
}

TEST_CASE("Multiruns.2C3G") {
  multiruns(2, 3);
}

TEST_CASE("Multiruns.2C4G") {
  multiruns(2, 4);
}

TEST_CASE("Multiruns.3C1G") {
  multiruns(3, 1);
}

TEST_CASE("Multiruns.3C2G") {
  multiruns(3, 2);
}

TEST_CASE("Multiruns.3C3G") {
  multiruns(3, 3);
}

TEST_CASE("Multiruns.3C4G") {
  multiruns(3, 4);
}

TEST_CASE("Multiruns.4C1G") {
  multiruns(4, 1);
}

TEST_CASE("Multiruns.4C2G") {
  multiruns(4, 2);
}

TEST_CASE("Multiruns.4C3G") {
  multiruns(4, 3);
}

TEST_CASE("Multiruns.4C4G") {
  multiruns(4, 4);
}

// ----------------------------------------------------------------------------
// Subflow
// ----------------------------------------------------------------------------

TEST_CASE("Subflow") {

  tf::Taskflow taskflow;
  tf::Executor executor;
    
  int* cpu = nullptr;
  int* gpu = nullptr;
  
  const unsigned n = 1000;

  auto partask = taskflow.emplace([&](tf::Subflow& sf){

    auto cputask = sf.emplace([&](){
      cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
      REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);
    });
    
    auto gputask = sf.emplace([&](tf::cudaFlow& cf) {
      dim3 g = {(n+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto h2d = cf.copy(gpu, cpu, n);
      auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
      auto d2h = cf.copy(cpu, gpu, n);
      h2d.precede(kernel);
      kernel.precede(d2h);
    });

    cputask.precede(gputask);
  });
    
  auto chktask = taskflow.emplace([&](){
    for(unsigned i=0; i<n ;++i){
      REQUIRE(cpu[i] == 1);
    }
    REQUIRE(cudaFree(gpu) == cudaSuccess);
    std::free(cpu);
  });

  partask.precede(chktask);

  executor.run(taskflow).wait();

}

// ----------------------------------------------------------------------------
// NestedSubflow
// ----------------------------------------------------------------------------

TEST_CASE("NestedSubflow") {

  tf::Taskflow taskflow;
  tf::Executor executor;
    
  int* cpu = nullptr;
  int* gpu = nullptr;
  
  const unsigned n = 1000;
    
  auto cputask = taskflow.emplace([&](){
    cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
    REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);
  });

  auto partask = taskflow.emplace([&](tf::Subflow& sf){
    
    auto gputask1 = sf.emplace([&](tf::cudaFlow& cf) {
      dim3 g = {(n+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto h2d = cf.copy(gpu, cpu, n);
      auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
      auto d2h = cf.copy(cpu, gpu, n);
      h2d.precede(kernel);
      kernel.precede(d2h);
    });

    auto subtask1 = sf.emplace([&](tf::Subflow& sf) {
      auto gputask2 = sf.emplace([&](tf::cudaFlow& cf) {
        dim3 g = {(n+255)/256, 1, 1};
        dim3 b = {256, 1, 1};
        auto h2d = cf.copy(gpu, cpu, n);
        auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
        auto d2h = cf.copy(cpu, gpu, n);
        h2d.precede(kernel);
        kernel.precede(d2h);
      });
      
      auto subtask2 = sf.emplace([&](tf::Subflow& sf){
        sf.emplace([&](tf::cudaFlow& cf) {
          dim3 g = {(n+255)/256, 1, 1};
          dim3 b = {256, 1, 1};
          auto h2d = cf.copy(gpu, cpu, n);
          auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
          auto d2h = cf.copy(cpu, gpu, n);
          h2d.precede(kernel);
          kernel.precede(d2h);
        });
      });

      gputask2.precede(subtask2);
    });

    gputask1.precede(subtask1);
  });
    
  auto chktask = taskflow.emplace([&](){
    for(unsigned i=0; i<n ;++i){
      REQUIRE(cpu[i] == 3);
    }
    REQUIRE(cudaFree(gpu) == cudaSuccess);
    std::free(cpu);
  });

  partask.precede(chktask)
         .succeed(cputask);

  executor.run(taskflow).wait();

}

// ----------------------------------------------------------------------------
// DetachedSubflow
// ----------------------------------------------------------------------------

TEST_CASE("DetachedSubflow") {

  tf::Taskflow taskflow;
  tf::Executor executor;
    
  int* cpu = nullptr;
  int* gpu = nullptr;
  
  const unsigned n = 1000;

  taskflow.emplace([&](tf::Subflow& sf){

    auto cputask = sf.emplace([&](){
      cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
      REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);
    });
    
    auto gputask = sf.emplace([&](tf::cudaFlow& cf) {
      dim3 g = {(n+255)/256, 1, 1};
      dim3 b = {256, 1, 1};
      auto h2d = cf.copy(gpu, cpu, n);
      auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
      auto d2h = cf.copy(cpu, gpu, n);
      h2d.precede(kernel);
      kernel.precede(d2h);
    });

    cputask.precede(gputask);

    sf.detach();
  });
    
  executor.run(taskflow).wait();
  
  for(unsigned i=0; i<n ;++i){
    REQUIRE(cpu[i] == 1);
  }
  REQUIRE(cudaFree(gpu) == cudaSuccess);
  std::free(cpu);
}

// ----------------------------------------------------------------------------
// Conditional GPU tasking
// ----------------------------------------------------------------------------

TEST_CASE("Loop") {

  tf::Taskflow taskflow;
  tf::Executor executor;

  const unsigned n = 1000;
    
  int* cpu = nullptr;
  int* gpu = nullptr;

  auto cputask = taskflow.emplace([&](){
    cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
    REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);
  });

  auto gputask = taskflow.emplace([&](tf::cudaFlow& cf) {
    dim3 g = {(n+255)/256, 1, 1};
    dim3 b = {256, 1, 1};
    auto h2d = cf.copy(gpu, cpu, n);
    auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
    auto d2h = cf.copy(cpu, gpu, n);
    h2d.precede(kernel);
    kernel.precede(d2h);
  });

  auto condition = taskflow.emplace([&cpu, round=0] () mutable {
    ++round;
    for(unsigned i=0; i<n; ++i) {
      REQUIRE(cpu[i] == round);
    }
    return round >= 100;
  });

  auto freetask = taskflow.emplace([&](){
    REQUIRE(cudaFree(gpu) == cudaSuccess);
    std::free(cpu);
  });

  cputask.precede(gputask);
  gputask.precede(condition);
  condition.precede(gputask, freetask);
  
  executor.run(taskflow).wait();
}

// ----------------------------------------------------------------------------
// Predicate
// ----------------------------------------------------------------------------

TEST_CASE("Predicate") {

  tf::Taskflow taskflow;
  tf::Executor executor;

  const unsigned n = 1000;
    
  int* cpu = nullptr;
  int* gpu = nullptr;

  auto cputask = taskflow.emplace([&](){
    cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
    REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);
    REQUIRE(cudaMemcpy(gpu, cpu, n*sizeof(int), cudaMemcpyHostToDevice) == cudaSuccess);
  });

  auto gputask = taskflow.emplace([&](tf::cudaFlow& cf) {
    dim3 g = {(n+255)/256, 1, 1};
    dim3 b = {256, 1, 1};
    auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
    auto copy = cf.copy(cpu, gpu, n);
    kernel.precede(copy);
    cf.predicate([i=100]() mutable { return i-- == 0; });
  });

  auto freetask = taskflow.emplace([&](){
    for(unsigned i=0; i<n; ++i) {
      REQUIRE(cpu[i] == 100);
    }
    REQUIRE(cudaFree(gpu) == cudaSuccess);
    std::free(cpu);
  });

  cputask.precede(gputask);
  gputask.precede(freetask);
  
  executor.run(taskflow).wait();
}

// ----------------------------------------------------------------------------
// Repeat
// ----------------------------------------------------------------------------

TEST_CASE("Repeat") {

  tf::Taskflow taskflow;
  tf::Executor executor;

  const unsigned n = 1000;
    
  int* cpu = nullptr;
  int* gpu = nullptr;

  auto cputask = taskflow.emplace([&](){
    cpu = static_cast<int*>(std::calloc(n, sizeof(int)));
    REQUIRE(cudaMalloc(&gpu, n*sizeof(int)) == cudaSuccess);
    REQUIRE(cudaMemcpy(gpu, cpu, n*sizeof(int), cudaMemcpyHostToDevice) == cudaSuccess);
  });

  auto gputask = taskflow.emplace([&](tf::cudaFlow& cf) {
    dim3 g = {(n+255)/256, 1, 1};
    dim3 b = {256, 1, 1};
    auto kernel = cf.kernel(g, b, 0, k_add<int>, gpu, n, 1);
    auto copy = cf.copy(cpu, gpu, n);
    kernel.precede(copy);
    cf.repeat(100);
  });

  auto freetask = taskflow.emplace([&](){
    for(unsigned i=0; i<n; ++i) {
      REQUIRE(cpu[i] == 100);
    }
    REQUIRE(cudaFree(gpu) == cudaSuccess);
    std::free(cpu);
  });

  cputask.precede(gputask);
  gputask.precede(freetask);
  
  executor.run(taskflow).wait();
}


