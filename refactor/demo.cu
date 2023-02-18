#include "access.h"
using namespace std;

const int kmaxRow = 10000;
const int kmaxBitsetThou = 1000;
const int kmaxBitsetTenThou = 10000;
const int kmaxBitsetHunThou = 100000;

enum AlgoType {dawn, dij, spfa};

class Matrix {
public:
	Matrix() {}
	Matrix(string _g_type, string _device, AlgoType _algo_type, int _row, int _col):
			g_type(_g_type), device(_device), algo_type(_algo_type), row(_row), col(_col) {
		if (device == "gpu" && g_type == "dense") {
			stride = (col + 31) / 32;
      		g_dense_gpu = new unsigned int[row * stride]();
		}
	}
	~Matrix() {
		if (device == "gpu" && g_type == "dense") {
			delete[] g_dense_gpu;
		}
	}
	inline void setValue(int i, int j, int value) {
		g_dense_gpu[i * stride + j] = value;
	}
	inline int getValue(int i, int j) {
		return g_dense_gpu[i * stride + j];
	}
	void readData(bool* in) {
		if (device == "gpu" && g_type == "dense") {
			for (int i = 0; i < row; ++i) {
				unsigned int tmp = 0;
				for (int j = 0; j < col; ++j) {
					tmp = (tmp << 1) | in[i * col + j];
					if (j & 31 == 31 || j == col - 1) {
						g_dense_gpu[i * stride + (j >> 5)] = tmp;
					}
				}
			}
		}
	}
	void sparseMultiplication(Matrix* A) { // this = this * A
		if (device == "gpu" && g_type == "dense") {
			unsigned int *tmp = new unsigned int[row * col];
			for (int i = 0; i < row; ++i) {
				for (int j = 0; j < col; ++j) {
					for (int k = 0; k < stride; ++k) {
						tmp[i * col + j] = this->g_dense_gpu[i * stride + k] * A->g_dense_gpu[j * stride + k];
					}
				}
			}
			unsigned int *ans = new unsigned int[row * stride];
			for (int i = 0; i < row; ++i) {
				unsigned int temp = 0;
				for (int j = 0; j < col; ++j) {
					temp = (temp << 1) | tmp[i * col + j];
					if (j & 31 == 31 || j == col - 1) {
						ans[i * stride + (j >> 5)] = temp;
					}
				}
			}
			delete []this->g_dense_gpu;
			this->g_dense_gpu = ans;
			delete []tmp;
		}
	}
	string g_type, device;
	AlgoType algo_type;
	int row, col;
	vector<pair<int, long long>> g_sparse_cpu[kmaxRow];
	vector<bitset<kmaxBitsetThou>> g_dense_cpu_thou;
	vector<bitset<kmaxBitsetTenThou>> g_dense_cpu_ten_thou;
	vector<bitset<kmaxBitsetHunThou>> g_dense_cpu_hun_thou;
	int stride;
	unsigned int *g_dense_gpu;
};

struct Node_queue {
    int u, step;
    Node_queue() {};
    Node_queue(int a, int sp) {
        u = a, step = sp;
    }
    bool operator < (const Node_queue& a)const { //重载 <
        return step > a.step;
    }
};

class Graph {
public: 
	Graph(string _g_type, string _device, int _node_num, AlgoType _algo_type):
		node_num(_node_num), algo_type(_algo_type) {
		k = 0;
		k_max = 0;
		k_diameter = 0;
		A = new Matrix(_g_type, _device, algo_type, _node_num, _node_num);
		B = new Matrix(_g_type, _device, algo_type, _node_num, _node_num);
		/* save res with one_dim ptr */
		if (algo_type == dij) {
			res = new Matrix("dense", "gpu", algo_type, _node_num, _node_num * 32);
		} else {
			res = new Matrix("dense", "gpu", algo_type, _node_num, _node_num);
		}
	}
	~Graph() {
		delete A;
		if (algo_type == dawn) {
			delete B;
		}
	}
	void readMap(string file_name, string random_flag) {
		bool* tmp = new bool[node_num * node_num]();
		if (random_flag == "true") {
			ofstream out;
			out.open(file_name);
			for (int i = 0; i < node_num; ++i) {
				for (int j = 0; j < node_num; j += 32) {
					long long ra = rand();
					long long rb = rand();
					long long rn = (ra * RAND_MAX + rb) % INT_MAX;
					for (int k = 0; k < 32 && j + k < node_num; ++k, rn >>= 1) {
						tmp[i * node_num + j + k] = rn & 1;
						if (rn & 1) {
							out << i << ' ' << j + k << '\n';
						}
					}
				}
			}
			out.close();
		} else {
			ifstream in;
			in.open(file_name);
			if (!in) {
				cout << " == File ifstream error: " << file_name << " ==";
			}
			while (in) {
				if (in.eof()) break;
				int a, b; 
				in >> a >> b;
				tmp[a * node_num + b] = 1;
			}
			in.close();
		}
		A->readData(tmp);
		delete []tmp;
	}
	void updateShortestPath(int dim) {
// #pragma omp parallel for
		for (int i = 0; i < node_num; ++i) {
			for (int j = 0; j < node_num; ++j) {
				if (i != j && B->getValue(i, j) && res->getValue(i, j) == 0) {
					res->setValue(i, j, dim);
					++k;
				}
			}
		}
	}
	void runDawn() {
		long long k_last = 0;
    	long long dim = 1;
		while(1) {
			++dim;
			B->sparseMultiplication(A);
			updateShortestPath(dim);
			if (k > k_max - 1) return ;
			if (k_diameter == dim) return ;
			if (k == k_last) return ;
			k_last = k;
		}
	}
	void runDij() {
		unsigned int INF = 0x3f3f3f3f;
		// #pragma omp parallel for
		for (int i = 0; i < node_num; i++) {
			priority_queue<Node_queue> Q;//优先队列优化
			Q.push(Node_queue(i, 0));
			unsigned int* dist = res->g_dense_gpu + i * node_num;
			bool flag[node_num + 5] = {};
			memset(dist, 0x3f, sizeof(unsigned int) * node_num);
			dist[i] = 0;
			while (!Q.empty()) {
				Node_queue it = Q.top();//优先队列列头元素为最小值
				Q.pop();
				int t = it.u;
				if (flag[t])//说明已经找到了最短距离，该节点是队列里面的重复元素
					continue;
				flag[t] = 1;
				for (int j = 0; j < node_num; j++) {
					if (!flag[j] && A->getValue(t, j) < INF) //判断与当前点有关系的点，并且自己不能到自己
						if (dist[j] > dist[t] + A->getValue(t, j)) {
							//求距离当前点的每个点的最短距离，进行松弛操作
							dist[j] = dist[t] + A->getValue(t, j);
							Q.push(Node_queue(j, dist[j]));//把更新后的最短距离压入队列中，注意：里面有重复元素
							// A->setValue(i, j, dist[j]);
						}
				}
			}
		}
	}
	void runSpfa() {}
	void runShortestPath() {
		if (algo_type == dawn) {
			runDawn();
		} else if (algo_type == dij) {
			runDij();
		} else if (algo_type == spfa) {
			runSpfa();
		}
	}
	void saveRes(string file_name) {
		ofstream out;
		out.open(file_name);
		for (int i = 0; i < res->row; ++i) {
			for (int j = 0; j < res->stride; ++j) {
				// cout << "i:" << i << " j:" << j << "  ";
				out << res->g_dense_gpu[i * res->stride + j] << ' ';
			}
			out << '\n';
		}
		out.close();
	}
	AlgoType algo_type;
	Matrix *A, *B, *res;
	long long k, k_max, k_diameter;
	int node_num;
};

Graph* readNeighboorMatrix(string file_name, 
		string g_type, string device, string random_flag, string _node_num, string algo_name) {
	int node_num = stoi(_node_num);
	/*...*/ // read row and col from file
	AlgoType algo_type;
	if (algo_name == "dawn") {
		algo_type = dawn;
	} else if (algo_name == "dij") {
		algo_type = dij;
		device = "gpu";
		g_type = "dense";
	}
	Graph* g = new Graph(g_type, device, node_num, algo_type); 
	g->readMap(file_name, random_flag); // need to read row and col again
	return g;
}

int main(int argc, char *argv[])
{
	srand(time(nullptr));
	string input_path = argv[1];
    string output_path = argv[2];
	string g_type = argv[3]; // sparse or dense
	string device = argv[4]; // cpu or gpu
	string random_flag = argv[5]; // true or false
	string node_num = argv[6]; // int
	string algo_name = argv[7];
	Graph* g = readNeighboorMatrix(input_path, g_type, device, random_flag, node_num, algo_name);
	g->runShortestPath();
	g->saveRes(output_path);
	// delete g;
    return 0;
}