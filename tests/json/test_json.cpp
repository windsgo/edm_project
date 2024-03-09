#include <json.hpp>

#include <array>
#include <cstdio>

struct Entity {
    int a;
};

namespace json::ext {
template <> class jsonization<Entity> {
public:
    json::value to_json(const Entity &t) { return t.a; }
    bool check_json(const json::value &j) {
        fprintf(stderr, "check_json !\n");

        return j.is_number();
    }
    bool from_json(const json::value &j, Entity &t) {
        fprintf(stderr, "from_json !\n");

        t.a = j.as_integer();
        return true;
    }
};
} // namespace json::ext

static void test() {
    json::value jv{1234};
    if (jv.is<Entity>()) {
        fprintf(stderr, "in is<>()\n");
        Entity e = jv.as<Entity>();
    }
}

#include <unordered_map>
#include <string>

struct Entity2 {
    std::string name;
    std::unordered_map<std::string, std::string> map;

    MEO_JSONIZATION(name, map);
};

static void test_map() {
    std::string str = R"(
        {
            "name": "123",
            "map": {
                "1": "str1",
                "2": "str2"
            }
        }
    )";

    auto ret = json::parse(str);
    if (!ret) {
        exit(-1);
    }

    auto v = std::move(*ret);

    Entity2 e = (Entity2)v;
    
}

int main() {

    test_map();

    return 0;
}