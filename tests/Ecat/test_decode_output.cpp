#include <fstream>
#include <iostream>
#include <sstream>

#include <queue>

// struct Data {
// public:
//     Data() { std::cout << "default ctor" << std::endl; }
//     Data(int a, int b) {
//         act_pos = a;
//         cmd_pos = b;
//     }
//     int act_pos;
//     int cmd_pos;
// };

struct data {
    double diff;
    double last;
    double now;
};

int main() {

    // std::queue<Data> queue;
    // std::cout << queue.empty() << std::endl;
    // queue.pop();
    // auto &d = queue.front();
    // int a = d.act_pos;
    // int b = d.cmd_pos;

    // std::cout << a << " " << b << "\n";

    std::ofstream ofs("decode.txt");

    std::ifstream ifs("output.bin", std::ios::binary);

    ifs.seekg(0, std::ios::end);
    int size = ifs.tellg();

    int data_num = size / sizeof(data);
    std::cout << "size: " << size << ", datas: " << data_num << std::endl;

    ifs.seekg(0, std::ios::beg);

    ofs.setf(std::ios::fixed, std::ios::floatfield);
    ofs.precision(4);

    for (int i = 0; i < data_num; ++i) {
        data data{0, 0, 0};
        ifs.read((char *)&data, sizeof(data));

        ofs << i + 1 << '\t' << data.diff << '\t' << data.last << '\t'
            << data.now << '\n';
    }

    ifs.close();
    ofs.close();

    return 0;
}