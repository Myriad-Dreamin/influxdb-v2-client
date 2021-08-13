#include <influxm/client.h>
#include <vector>

// todo
namespace influx_client {

influxdb_if_inline int
query(std::string &resp, const std::string &query, const flux::Client &si) {
  std::string qs("&q=");
  detail::url_encode(qs, query);
  // "GET", "query",
  //  return detail::http_request(&si.addr, qs, "", si, &resp);
  return 0;
}

influxdb_if_inline int create_db(
    std::string &resp, const std::string &db_name, const flux::Client &si) {
  std::string qs("&q=create+database+");
  detail::url_encode(qs, db_name);
  //  return detail::http_request(&si.addr, qs, "", si, &resp);
  return 0;
}
}; // namespace influx_client

#define ASSERT_TRUE(v)                                                         \
  do {                                                                         \
    if (!(v)) {                                                                \
      printf("bad assertion: %s:%d: " #v, __func__, __LINE__);                 \
      return -1;                                                               \
    }                                                                          \
  } while (0)

void construct_test() {
  influx_client::kv_t v_is_string{"k", "v"};
  influx_client::kv_t v_is_string2{"k", std::string("a")};
#if __cplusplus >= 201700L
  influx_client::kv_t v_is_string3{"k", std::string_view("v")};
#endif

  influx_client::kv_t v_is_bool{"k", true};
  influx_client::kv_t v_is_bool2{"k", false};

  influx_client::kv_t v_is_integer{"k", 0};
  influx_client::kv_t v_is_integer2{"k", uint8_t(0)};
  influx_client::kv_t v_is_integer3{"k", uint64_t(0)};
  influx_client::kv_t v_is_integer4{"k", int32_t(0)};
  influx_client::kv_t v_is_integer5{"k", int(0)};

  influx_client::kv_t v_is_double{"k", 0.};
  influx_client::kv_t v_is_double2{"k", double(0)};
  influx_client::kv_t v_is_double3{"k", double(0.1)};
  influx_client::kv_t v_is_double4{"k", float(0.1)};
}

int main(int argc, char **argv) {
  using namespace influx_client::flux;
  if (argc != 3) {
    return 1;
  }

  Client client(
      "127.0.0.1", int(std::strtol(argv[1], nullptr, 10)), argv[2], "kfuzz",
      "test");

  int code = client.write(
      "metrics_x",
      std::vector<influx_client::tag>{
          influx_client::tag{"tag1", "value1"},
          influx_client::tag{"tag2", "value2"}},
      {{"field1", 0}, {"field2", 0}});

  ASSERT_TRUE(code == 204);

  code = client.write(
      "metrics_1", {{"tag1", "value1"}, {"tag2", "value2"}},
      {{"field1", "value3"}, {"field2", "value4"}});

  ASSERT_TRUE(code == 204);

  code = client.write(
      "metrics_2", {{"tag1", 1}, {"tag2", 0}},
      {{"field1", "value3"}, {"field2", "value4"}});

  ASSERT_TRUE(code == 204);

  code = client.write("metrics_2", {{"tag1", 1}}, {{"field1", "value3"}});

  ASSERT_TRUE(code == 204);

  std::string a = "1";
  std::vector<influx_client::kv_t> tags;
  tags.emplace_back("tag1", 1);

  code = client.write(a, tags, {{"field1", "value3"}});

  ASSERT_TRUE(code == 204);

  std::string q;

  code = client.write(a, tags, {{"field1", "value3"}}, 0, &q);

  ASSERT_TRUE(code == 204);

  client.token = "123456";
  client.reset_network_data();

  code = client.write(a, tags, {{"field1", "value3"}}, 0);

  ASSERT_TRUE(code == 0 || code == 401);

  code = client.write(a, tags, {{"field1", "value3"}}, 0, &q);

  ASSERT_TRUE(code == 0 || code == 401);

  construct_test();

  return 0;
}
