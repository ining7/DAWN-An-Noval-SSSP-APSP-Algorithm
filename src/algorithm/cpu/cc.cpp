/**
 * @author lxrzlyr (1289539524@qq.com)
 * @date 2024-02-23
 *
 * @copyright Copyright (c) 2024
 */
#include <dawn/algorithm/cpu/cc.hxx>
float DAWN::CC_CPU::run(Graph::Graph_t& graph,
                        int source,
                        float& elapsed_time) {
  auto row = graph.rows;
  if (graph.csr.row_ptr[source] == graph.csr.row_ptr[source + 1]) {
    std::cout << "Source is isolated node, please check" << std::endl;
    exit(0);
  }
  float Closeness_Centrality = DAWN::CC_CPU::kernel(
      graph.csr.row_ptr, graph.csr.col, row, source, elapsed_time);

  return Closeness_Centrality;
}

float DAWN::CC_CPU::run_Weighted(Graph::Graph_t& graph,
                                 int source,
                                 float& elapsed_time) {
  auto row = graph.rows;
  if (graph.csr.row_ptr[source] == graph.csr.row_ptr[source + 1]) {
    std::cout << "Source is isolated node, please check" << std::endl;
    exit(0);
  }
  float Closeness_Centrality =
      DAWN::CC_CPU::kernel_Weighted(graph.csr.row_ptr, graph.csr.col,
                                    graph.csr.val, row, source, elapsed_time);

  return Closeness_Centrality;
}

float DAWN::CC_CPU::kernel(int* row_ptr,
                           int* col,
                           int row,
                           int source,
                           float& elapsed_time) {
  int step = 1;
  bool is_converged = false;
  bool* alpha = new bool[row];
  bool* beta = new bool[row];
  int* distance = new int[row];

  std::fill_n(alpha, row, false);
  std::fill_n(beta, row, false);
  std::fill_n(distance, row, 0);

#pragma omp parallel for
  for (int i = row_ptr[source]; i < row_ptr[source + 1]; i++) {
    alpha[col[i]] = true;
    distance[col[i]] = 1;
  }

  auto start = std::chrono::high_resolution_clock::now();
  while (step < row) {
    step++;

    if (!(step % 2))
      is_converged =
          DAWN::BFS_CPU::SOVMP(row_ptr, col, row, alpha, beta, distance, step);
    else
      is_converged =
          DAWN::BFS_CPU::SOVMP(row_ptr, col, row, beta, alpha, distance, step);
    if (is_converged) {
      break;
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed_time_tmp = end - start;
  elapsed_time += elapsed_time_tmp.count();

  distance[source] = 0;
  float closeness_centrality =
      (row - 1) / std::accumulate(distance, distance + row, 0);

  delete[] beta;
  beta = nullptr;
  delete[] alpha;
  alpha = nullptr;
  delete[] distance;
  distance = nullptr;

  return closeness_centrality;
}

float DAWN::CC_CPU::kernel_Weighted(int* row_ptr,
                                    int* col,
                                    float* val,
                                    int row,
                                    int source,
                                    float& elapsed_time) {
  int step = 1;
  bool is_converged = false;
  bool* alpha = new bool[row];
  bool* beta = new bool[row];
  float* distance = new float[row];
  float INF = 1.0 * 0xfffffff;

  std::fill_n(alpha, row, false);
  std::fill_n(beta, row, false);
  std::fill_n(distance, row, INF);

#pragma omp parallel for
  for (int i = row_ptr[source]; i < row_ptr[source + 1]; i++) {
    distance[col[i]] = val[i];
    alpha[col[i]] = true;
  }
  distance[source] = 0.0f;

  auto start = std::chrono::high_resolution_clock::now();
  while (step < row) {
    step++;
    if (!(step % 2))
      is_converged =
          DAWN::SSSP_CPU::GOVMP(row_ptr, col, val, row, alpha, beta, distance);
    else
      is_converged =
          DAWN::SSSP_CPU::GOVMP(row_ptr, col, val, row, beta, alpha, distance);
    if (is_converged) {
      break;
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed_time_tmp = end - start;
  elapsed_time += elapsed_time_tmp.count();

  distance[source] = 0.0f;
  float closeness_centrality =
      (1.0 * row - 1.0f) / std::accumulate(distance, distance + row, 0.0f);

  delete[] beta;
  beta = nullptr;
  delete[] alpha;
  alpha = nullptr;
  delete[] distance;
  distance = nullptr;
  return closeness_centrality;
}
