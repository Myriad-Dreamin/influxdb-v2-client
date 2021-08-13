#include <influxm/client.h>
#include <vector>

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

int main(int argc, char** argv) {
  using namespace influx_client::flux;
  if (argc != 3) {
    return 1;
  }

  Client client(
      "127.0.0.1", int(std::strtol(argv[1], nullptr, 10)), argv[2], "kfuzz", "test");

  int code = client.write(
      "metrics_x", {{"tag1", "value1"}, {"tag2", "value2"}},
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

  return 0;
}
