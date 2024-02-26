#include <fstream>
#include <iostream>
#include <sstream>

#include <queue>

struct Data {
public:
    Data() {
        std::cout << "default ctor" << std::endl;
    }
    Data(int a, int b) {
        act_pos = a;
        cmd_pos = b;
    }
    int act_pos;
    int cmd_pos;
};

int main() {

    std::queue<Data> queue;
    std::cout << queue.empty() << std::endl;
    queue.pop();
    auto &d = queue.front();
    int a = d.act_pos;
    int b = d.cmd_pos;

    std::cout << a << " " << b << "\n";

    std::ofstream ofs("decode.txt");

    std::ifstream ifs("output.bin", std::ios::binary);

    ifs.seekg(0, std::ios::end);
    int size = ifs.tellg();

    int data_num = size / sizeof(Data);
    std::cout << "size: " << size << ", datas: " << data_num << std::endl;

    ifs.seekg(0, std::ios::beg);
    for (int i = 0; i < data_num; ++i) {
        Data data{0, 0};
        ifs.read((char *)&data, sizeof(Data));

        ofs << i + 1 << '\t' << data.act_pos << '\t' << data.cmd_pos << '\n';
    }

    ifs.close();
    ofs.close();

    return 0;
}