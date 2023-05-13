#include "dawn.hxx"
namespace DAWN {
void Graph::createGraphCsm(std::string& input_path, DAWN::Graph& graph)
{
  std::ifstream file(input_path);
  if (!file.is_open()) {
    std::cerr << "Error opening file " << input_path << std::endl;
    return;
  }

  std::string line;
  int         rows, cols, nnz, dim;
  while (std::getline(file, line)) {
    if (line[0] == '%')
      continue;

    std::stringstream ss(line);
    ss >> rows >> cols >> nnz >> dim;
    break;
  }
  std::cout << rows << " " << cols << " " << nnz << " " << dim << std::endl;
  file.close();

  graph.rows  = rows;
  graph.cols  = cols;
  graph.dim   = dim;
  graph.entry = 0;

  graph.coo.col = new int[nnz];
  graph.coo.row = new int[nnz];
  graph.coo.val = new float[nnz];

  std::fill_n(graph.coo.col, 0, nnz);
  std::fill_n(graph.coo.row, 0, nnz);
  std::fill_n(graph.coo.val, 0.0f, nnz);
  // std::memset(graph.coo.col, 0, sizeof(int) * nnz);
  // std::memset(graph.coo.row, 0, sizeof(int) * nnz);
  // std::memset(graph.coo.val, 0.0f, sizeof(float) * nnz);

  std::cout << "Read Input Graph" << std::endl;

  DAWN::Tool tool;

  graph.readGraph(input_path, graph);
  tool.coo2Csm(graph.rows, graph.nnz, graph.csmA, graph.coo);
  tool.transport(graph.rows, graph.nnz, graph.coo);
  tool.coo2Csm(graph.rows, graph.nnz, graph.csmB, graph.coo);
  // for (int i = 0; i < graph.rows; i++)
  //   for (int j = 0; j < graph.csmA.row[i]; j++)
  //     printf("A.col[%d][%d]\n", i, graph.csmA.col[i][j]);

  // for (int i = 0; i < graph.rows; i++)
  //   for (int j = 0; j < graph.csmB.row[i]; j++)
  //     printf("B.col[%d][%d]\n", i, graph.csmB.col[i][j]);

  delete[] graph.coo.col;
  graph.coo.col = NULL;
  delete[] graph.coo.row;
  graph.coo.row = NULL;
  delete[] graph.coo.val;
  graph.coo.val = NULL;

  std::cout << "Initialize Input Matrices" << std::endl;
}

void Graph::createGraphCsr(std::string& input_path, DAWN::Graph& graph)
{
  std::ifstream file(input_path);
  if (!file.is_open()) {
    std::cerr << "Error opening file " << input_path << std::endl;
    return;
  }

  std::string line;
  int         rows, cols, nnz, dim;
  while (std::getline(file, line)) {
    if (line[0] == '%')
      continue;

    std::stringstream ss(line);
    ss >> rows >> cols >> nnz >> dim;
    break;
  }
  std::cout << rows << " " << cols << " " << nnz << " " << dim << std::endl;
  file.close();

  graph.rows  = rows;
  graph.cols  = cols;
  graph.dim   = dim;
  graph.entry = 0;

  graph.coo.col = new int[nnz];
  graph.coo.row = new int[nnz];
  graph.coo.val = new float[nnz];

  std::fill_n(graph.coo.col, 0, nnz);
  std::fill_n(graph.coo.row, 0, nnz);
  std::fill_n(graph.coo.val, 0.0f, nnz);
  // std::memset(graph.coo.col, 0, sizeof(int) * nnz);
  // std::memset(graph.coo.row, 0, sizeof(int) * nnz);
  // std::memset(graph.coo.val, 0.0f, sizeof(float) * nnz);

  std::cout << "Read Input Graph" << std::endl;

  DAWN::Tool tool;

  graph.readGraph(input_path, graph);
  tool.coo2Csr(graph.rows, graph.nnz, graph.csrA, graph.coo);
  tool.transport(graph.rows, graph.nnz, graph.coo);
  tool.coo2Csr(graph.rows, graph.nnz, graph.csrB, graph.coo);

  delete[] graph.coo.col;
  graph.coo.col = NULL;
  delete[] graph.coo.row;
  graph.coo.row = NULL;
  delete[] graph.coo.val;
  graph.coo.val = NULL;

  std::cout << "Initialize Input Matrices" << std::endl;
}

void Graph::readGraph(std::string& input_path, DAWN::Graph& graph)
{
  std::ifstream file(input_path);
  if (!file.is_open()) {
    std::cerr << "Error opening file " << input_path << std::endl;
    return;
  }
  std::string line;

  int rows, cols;
  int i = 0;
  while (std::getline(file, line)) {
    if (line[0] == '%')
      continue;
    std::stringstream ss(line);
    ss >> rows >> cols;
    rows--;
    cols--;
    if (rows != cols) {
      graph.coo.row[i] = rows;
      graph.coo.col[i] = cols;
      graph.coo.val[i] = 1.0f;
      i++;
    }
  }
  file.close();

  graph.nnz = i;
  std::cout << "nnz: " << graph.nnz << std::endl;
}

void Graph::readGraphWeighted(std::string& input_path, DAWN::Graph& graph)
{
  std::ifstream file(input_path);
  if (!file.is_open()) {
    std::cerr << "Error opening file " << input_path << std::endl;
    return;
  }
  std::string line;

  int rows, cols, vals;
  int i = 0;
  while (std::getline(file, line)) {
    if (line[0] == '%')
      continue;
    std::stringstream ss(line);
    ss >> rows >> cols >> vals;
    rows--;
    cols--;
    if (rows != cols) {
      graph.coo.row[i] = rows;
      graph.coo.col[i] = cols;
      graph.coo.val[i] = vals;
      i++;
    }
  }
  file.close();

  graph.nnz = i;
  std::cout << "nnz: " << graph.nnz << std::endl;
}
}  // namespace DAWN