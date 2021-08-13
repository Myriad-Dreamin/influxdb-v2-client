//
// Created by Myriad-Dreamin on 2021/8/13.
//

#ifndef INFLUXDBM_CLIENT_H
#define INFLUXDBM_CLIENT_H

#include "http.h"
#include "macro.h"
#include <cassert>
#include <chrono>
#include <type_traits>

#ifndef influxdb_if_inline
#define influxdb_if_inline inline
#endif

namespace influx_client {
struct kv_t;
using tag = kv_t;
using field = kv_t;

namespace detail {
template <typename T, typename X = void>
using is_influx_string_t = typename std::enable_if<
    std::is_same<T, detail::string_view>::value ||
        std::is_same<T, std::string>::value ||
        std::is_same<const char *, T>::value || std::is_same<char *, T>::value,
    X>::type;

template <typename T, typename X = void>
using is_influx_boolean_t =
    typename std::enable_if<std::is_same<T, bool>::value, X>::type;

template <typename T, typename X = void>
using is_influx_integer_t = typename std::enable_if<
    std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, X>::type;

} // namespace detail

struct kv_t {
  std::string k;
  std::string v;
  bool q;
  template <typename T>
  kv_t(detail::string_view k, T v, macroPAssert(detail::is_influx_string_t, T))
      : k(k), v(v), q(true) {}
  template <typename T>
  kv_t(detail::string_view k, T v, macroPAssert(detail::is_influx_boolean_t, T))
      : k(k), v(v ? "true" : "false"), q(false) {}
  template <typename T>
  kv_t(detail::string_view k, T v, macroPAssert(detail::is_influx_integer_t, T))
      : k(k), v(std::to_string(v)), q(false) {}
};

namespace flux {
struct Client {
  using timestamp_t = uint64_t;

  struct sockaddr_in addr{};
  std::string host;
  std::string bucket;
  std::string organization;
  std::string token;
  std::string precision;
  std::string write_v2_header;
  int port;

  Client(
      detail::string_view host, int port, detail::string_view token,
      detail::string_view org, detail::string_view bucket)
      // detail::string_view precision = "ns"
      : host(host), port(port), token(token), organization(org), bucket(bucket),
        precision("ns") {
    reset_network_data();
  }

  void reset_network_data() {
    write_v2_header.resize(400);
    int res = (snprintf(
        &write_v2_header[0], write_v2_header.size(),
        "%s /%s?bucket=%s&org=%s&precision=%s HTTP/1.1\r\n"
        "Host: %s\r\nAuthorization: Token %s\r\n",
        "POST", "api/v2/write", bucket.c_str(), organization.c_str(),
        precision.c_str(), host.c_str(), token.c_str()));
    assert(res <= 400);
    write_v2_header.resize(res);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if ((addr.sin_addr.s_addr = inet_addr(host.c_str())) == INADDR_NONE) {
      abort();
    }
  }

  template <typename T, typename F>
  int writeIter(
      detail::string_view measurement, T tags_begin, T tags_end, F fields_begin,
      F fields_end, timestamp_t t = 0, std::string *resp = nullptr);

#define macroWriteImpl(T, F)                                                   \
  int write(                                                                   \
      detail::string_view measurement, T tags, F fields, timestamp_t t = 0,    \
      std::string *resp = nullptr) {                                           \
    return writeIter(                                                          \
        measurement, tags.begin(), tags.end(), fields.begin(), fields.end(),   \
        t, resp);                                                              \
  }

  template <typename T, typename F> macroWriteImpl(T, F);
  template <typename T> macroWriteImpl(T, std::initializer_list<kv_t>);
  template <typename T> macroWriteImpl(std::initializer_list<kv_t>, T);
  macroWriteImpl(std::initializer_list<kv_t>, std::initializer_list<kv_t>);
#undef macroWriteImpl
};
} // namespace flux

namespace detail {

template <typename B, typename T>
int putKVSeq(B &buf, int64_t &q, int64_t bufSize, T b, T e) {
  for (auto v = b; v != e; v++) {
    macroMemoryPutC(buf, ',', q, bufSize);
    macroMemoryPutStdStr(buf, v->k, q, bufSize);
    macroMemoryPutC(buf, '=', q, bufSize);
    if (v->q) {
      macroMemoryPutC(buf, '"', q, bufSize);
      size_t pos, start = 0;
      while ((pos = v->v.find_first_of('\"', start)) != std::string::npos) {
        macroMemoryCopyN(buf, v->v.c_str() + start, pos - start, q, bufSize);
        macroMemoryPutC(buf, '\\', q, bufSize);
        macroMemoryPutC(buf, v->v[pos], q, bufSize);
        start = ++pos;
      }
      macroMemoryCopyN(
          buf, v->v.c_str() + start, v->v.length() - start, q, bufSize);
      macroMemoryPutC(buf, '"', q, bufSize);
    } else {
      macroMemoryPutStdStr(buf, v->v, q, bufSize);
    }
  }
  return 0;
}

} // namespace detail

template <typename T, typename F>
int flux::Client::writeIter(
    detail::string_view measurement, T tags_begin, T tags_end, F fields_begin,
    F fields_end, timestamp_t t, std::string *resp) {
  macroAllocBuffer(buf, bufSize);
  int64_t q = 0;
  int code;
  macroMemoryPutStdStr(buf, measurement, q, bufSize);

  if (tags_begin != tags_end) {
    code = detail::putKVSeq(buf, q, bufSize, tags_begin, tags_end);
    if (code) {
      return code;
    }
  }

  if (fields_begin == fields_end) {
    assert(false);
  }
  int b = q;
  code = detail::putKVSeq(buf, q, bufSize, fields_begin, fields_end);
  buf[b] = ' ';
  if (code) {
    return code;
  }
  macroMemoryPutC(buf, ' ', q, bufSize);

  if (!t) {
    t = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
  }
  std::string ts = std::to_string(t);
  macroMemoryPutStdStr(buf, ts, q, bufSize);
  code = detail::http_request(
      &addr, write_v2_header, detail::to_string_view(buf, q), resp);
  macroFreeBuffer(buf, bufSize);
  return code;
}

} // namespace influx_client

#endif // INFLUXDBM_CLIENT_H
