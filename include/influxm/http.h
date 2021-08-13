//
// Created by Myriad-Dreamin on 2021/8/13.
//

#ifndef INFLUXDBM_HTTP_H
#define INFLUXDBM_HTTP_H

#include "alloc.h"
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <algorithm>
#include <windows.h>
#pragma comment(lib, "ws2_32")
typedef struct iovec {
  void *iov_base;
  size_t iov_len;
} iovec;
influxdb_if_inline uint64_t writev(int sock, struct iovec *iov, int cnt) {
  uint64_t r = send(sock, (const char *)iov->iov_base, iov->iov_len, 0);
  return (r < 0 || cnt == 1) ? r : r + writev(sock, iov + 1, cnt - 1);
}

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define closesocket close
#endif

#ifndef influxdb_if_inline
#define influxdb_if_inline inline
#endif

#ifndef influx_http_recv
#define influx_http_recv recv
#endif

namespace influx_client::detail {

influxdb_if_inline int http_request_(
    int sock, std::string_view pref_header, std::string_view body,
    std::string *resp) {
  static const uint32_t rn = uint32_t('\r') << 8 | uint32_t('\n');
  static const uint32_t rn_rn = rn << 16 | rn;
  static const uint32_t rn_co = rn << 16 | uint32_t('C') << 8 | uint32_t('o');
  static const uint32_t rn_tr = rn << 16 | uint32_t('T') << 8 | uint32_t('r');

  ssize_t recv_res = 0;
  int ret_code = 0, pref = pref_header.size(), content_length = body.size();
  int max_length = std::max<std::size_t>(pref + 32, 128), target;
  char buf[max_length];
  bool chunked, has_rn_co, has_rn_tr;

  // construct header, body
  memcpy(buf, pref_header.data(), pref);
  macroMemoryPutConst(buf, "Content-Length: ", pref, max_length);
  std::string content_length_s = std::to_string(content_length);
  macroMemoryPutStdStr(buf, content_length_s, pref, max_length);
  macroMemoryPutConst(buf, "\r\n\r\n", pref, max_length);

  // send data
  struct iovec iv[2]{
      iovec{(void *)(&buf[0]), size_t(pref)},
      iovec{(void *)(&body[0]), size_t(content_length)},
  };
  int r = writev(sock, iv, 2);
  if (r < ssize_t(iv[0].iov_len + iv[1].iov_len)) {
    return -6;
  }

  // send data
  /**
   * status:
   *   0: skip http version
   *   1: get status code
   *   2: walk through header
   *   3: parse header
   *   4: get body
   */
  int status = 0, i = 0;
  uint32_t window4 = 0;
  if (resp) { // need http response
    resp->clear();
    target = 5;
  } else {
    target = 2;
  }

  auto getOnce = [&] {
    return (recv_res = influx_http_recv(sock, &buf[0], max_length, 0)) > 0;
  };
  while (status != target) {
    if (recv_res == 0 && !getOnce()) {
      break;
    }
    switch (status) {
    default:
      status = target;
      break;
    case 0:
      for (i = 0; i < recv_res;) {
        if (buf[i++] == ' ') {
          goto status1;
        }
      }
      recv_res = 0;
      break;
    case 1:
      for (i = 0; i < recv_res; i++) {
        status1:
        if (std::isdigit(buf[i])) {
          ret_code = ret_code * 10 + buf[i] - '0';
        } else {
          status = 2;
          if (resp) {
            resp->append(buf, i, recv_res - i);
            i = 0;
            goto status2;
          }
          recv_res -= i;
          break;
        }
      }
      recv_res = 0;
      break;
    case 2:
      resp->append(buf, 0, recv_res);
    status2:
      while (i < resp->size()) {
        window4 = (window4 << 8) + (*resp)[i++];
        switch (window4) {
        case rn_rn:
          status = 3;
          pref = i;
          goto status3;
        case rn_co:
          has_rn_co = true;
          break;
        case rn_tr:
          has_rn_tr = true;
          break;
        default:
          break;
        }
      }
    case 3:
    status3:
      if (ret_code == 204) {
        status = 5;
      } else {

        (*resp)[pref - 2] = 0;
        (*resp)[pref - 1] = 0;
        printf("%s\n", resp->c_str());
        if (has_rn_co) {
          int col_pos = resp->find("Content-Length", 0, pref);
        } else if (has_rn_tr) {
          int col_pos = resp->find("Transfer-Encoding", 0, pref);
        }
        status = 5;
      }
      break;
    }
  }
  return recv_res < 0 ? int(recv_res) : ret_code;
}

influxdb_if_inline int http_request(
    const struct sockaddr_in *addr, std::string_view pref_header,
    std::string_view body, std::string *resp) {
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -2;

  if (connect(sock, (struct sockaddr *)(addr), sizeof(*addr)) < 0) {
    closesocket(sock);
    return -3;
  }

  int ret = http_request_(sock, pref_header, body, resp);
  closesocket(sock);
  return ret;
}

influxdb_if_inline unsigned char to_hex(unsigned char x) {
  return x > 9 ? x + 55 : x + 48;
}

influxdb_if_inline void url_encode(std::string &out, const std::string &src) {
  size_t pos = 0, start = 0;
  while (
      (pos = src.find_first_not_of(
          "abcdefghijklmnopqrstuvwxyqABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~",
          start)) != std::string::npos) {
    out.append(src.c_str() + start, pos - start);
    if (src[pos] == ' ')
      out += "+";
    else {
      out += '%';
      out += to_hex((unsigned char)src[pos] >> 4);
      out += to_hex((unsigned char)src[pos] & 0xF);
    }
    start = ++pos;
  }
  out.append(src.c_str() + start, src.length() - start);
}

} // namespace influx_client::detail

#endif // INFLUXDBM_HTTP_H
