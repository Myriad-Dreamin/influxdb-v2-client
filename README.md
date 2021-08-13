
# influxdb-v2-client

C++11 header only influxdb client (for `api/v2/write`. If you need send write request to `/write`, please use [influxdb-cpp](https://github.com/orca-zhang/influxdb-cpp)).

## Use with CMake

```cmake
add_subdirectory(${repo})
target_link_libraries(your_target PUBLIC ${repo})
```

## Example

```c++
#include <influxm/client.h>
#include <vector>
int main(int argc, char **argv) {
  using namespace influx_client::flux;

  Client client(
      "127.0.0.1", /* port */ 12345, /* token */
      "xxxxxxxxke37mh-yniv5yxyyyy_5uspn-mnzzzz7g1u87babiyh-he_az"
      "-aaaapov11112lj2-ovr5bbbbso6q==",
      "organization", "bucket");
  
  /* do something with client */
}

```

### request with `initializer_list` type

```c++
void example1() {
  Client client;
  code = client.write(
      "metrics_1", {{"tag1", "value1"}, {"tag2", "value2"}},
      {{"field1", "value3"}, {"field2", "value4"}});

  ASSERT_TRUE(code == 204);
}

```

### batch request with `initializer_list` type

```c++

void example_batch_1() {
  Client client;
  char points[4096];
  int pointsSize = 4096, offset = 0;

  code = client.writes(
      {
          {
              "metrics_1",
              {{"tag1", 1}, {"tag2", 0}},
              {{"field1", "value3"}, {"field2", "value4"}},
              0,
          },
          {
              "metrics_2",
              {{"tag2", 1}, {"tag3", 0}},
              {{"field4", "value3"}, {"field5", "value4"}},
              0,
          },
      },
      points, pointsSize);
  ASSERT_TRUE(code == 204);
}

```

### request with `vector` type

```c++
void example2() {
  Client client;
  std::vector<influx_client::kv_t> tags;
  tags.emplace_back("tag1", 1);

  code = client.write("metrics", tags, {{"field1", "value3"}});
  ASSERT_TRUE(code == 204);
  
  std::vector<influx_client::kv_t> fields = tags;
  code = client.write("metrics", tags, fields);
  ASSERT_TRUE(code == 204);
}
```

### batch request with `vector` type

```c++

void example_batch_2() {
  Client client;
  char points[4096];
  int pointsSize = 4096, offset = 0;
  influx_client::point_vec Vec;

  code = client.createPoint(
      "metrics_xx", tags, {{"field1", "value3"}}, points, pointsSize);
  ASSERT_TRUE(code >= 0);
  Vec.emplace_back(points + offset, code);
  offset += code;

  code = client.writes(Vec, &q);
  ASSERT_TRUE(code == 204);

  code = client.createPoint(
      "metrics_xx", tags, {{"field1", "value3"}}, points + offset,
      pointsSize - offset);
  ASSERT_TRUE(code >= 0);
  Vec.emplace_back(points + offset, code);

  code = client.writes(Vec, &q);
  ASSERT_TRUE(code == 204);
}
```

### request with `iterator` type

```c++
void example3() {
  Client client;
  std::vector<influx_client::kv_t> tags;
  tags.emplace_back("tag1", 1);

  code = client.write("metrics", tags, {{"field1", "value3"}});
  ASSERT_TRUE(code == 204);
  
  std::vector<influx_client::kv_t> fields;
  fields.emplace_back("field1", 1);
  code = client.write(
      "metrics", tags.begin(), tags.end(), fields.begin(), fields.end());
}
```

### construct `kv_t`

```c++

void example4() {

  influx_client::kv_t v_is_string{"k", "v"};
  influx_client::kv_t v_is_string2{"k", std::string("a")};
  influx_client::kv_t v_is_string3{"k", std::string_view_ref("v")};

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

```

### Error Handling

```c++
void example5() {
  Client client;

  code = client.write(a, {}, {{"field", "value"}});
  if (code < 0 || (code / 100) != 2) {
      /* error handling */
  }
}
```

### request with timestamp

```c++
void example6() {
  Client client;

  std::string hb;
  code = client.write(
      a, {}, {{"field", "value"}},
      /* nanoseconds since epoch */ 123456789123456ULL, &hb);
}
```

### Get response header and body

```c++
void example7() {
  Client client;

  std::string hb;
  code = client.write(a, {}, {{"field", "value"}}, 0, &hb);
  if ((code / 100) == 2) {
      using namespace myriad_dreamin::string_algorithm;
      std::string [header, body] = split(hb, "\r\n\r\n");
  }
}
```
