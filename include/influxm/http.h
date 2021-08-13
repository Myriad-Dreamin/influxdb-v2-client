//
// Created by Myriad-Dreamin on 2021/8/13.
//

#ifndef INFLUXDBM_HTTP_H
#define INFLUXDBM_HTTP_H

#include "macro.h"
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#pragma comment(lib, "ws2_32")
typedef struct iovec {
  void *iov_base;
  size_t iov_len;
} iovec;
#ifndef writev
#define writev writev
influxdb_if_inline uint64_t writev(int sock, struct iovec *iov, int cnt) {
  uint64_t r = send(sock, (const char *)iov->iov_base, iov->iov_len, 0);
  return (r < 0 || cnt == 1) ? r : r + writev(sock, iov + 1, cnt - 1);
}
#endif

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#define closesocket close
#endif

namespace influx_client {
namespace detail {

influxdb_if_inline constexpr uint8_t bit_swap8(uint8_t n) { return n & 0xff; }
influxdb_if_inline constexpr uint16_t bit_swap16(uint16_t n) {
  return bit_swap8(n) << 8 | bit_swap8(n >> 8);
}
influxdb_if_inline constexpr uint32_t bit_swap32(uint32_t n) {
  return bit_swap16(n) << 16 | bit_swap16(n >> 16);
}

influxdb_if_inline int http_request_(
    int sock, string_view pref_header, string_view body, std::string *resp) {
  static const uint32_t rn = uint32_t('\r') << 8 | uint32_t('\n');
  static const uint32_t rn_rn = rn << 16 | rn;
  static const uint32_t rn_co = rn << 16 | uint32_t('C') << 8 | uint32_t('o');
  static const uint32_t rn_tr = rn << 16 | uint32_t('T') << 8 | uint32_t('r');
  static const int max_length = 128;
  ssize_t recv_res = 0;
  int ret_code = 0, pref = pref_header.size(), content_length = body.size();
  int target, rn_co_pos = 0, rn_tr_pos = 0;
  char buf[max_length];

  // send data
  std::string content_length_s = std::to_string(content_length);
  struct iovec iv[5]{
      iovec{(void *)(&pref_header[0]), size_t(pref)},
      iovec{
          (void *)("Content-Length: "), size_t(sizeof("Content-Length: ") - 1)},
      iovec{(void *)(&content_length_s[0]), content_length_s.size()},
      iovec{(void *)("\r\n\r\n"), size_t(4)},
      iovec{(void *)(&body[0]), size_t(content_length)},
  };
  int r = influx_http_writev(sock, iv, 5);
  if (r < ssize_t(
              iv[0].iov_len + iv[1].iov_len + iv[2].iov_len + iv[3].iov_len +
              iv[4].iov_len)) {
    return -6;
  }

  uint32_t window4 = 0;
  int i = 0, j = 0, recv_rest = max_length;
  auto getOnce = [&] {
    return (recv_res = influx_http_recv(
                sock, &buf[0], std::min(max_length, recv_rest), 0)) > 0;
  };

  /**
   * http message parser status:
   *   0: skip http version
   *   1: get status code
   *   2: walk through header
   *   3: parse header
   *   4: get body
   */
  int status = 0;
  if (resp) { // need http response
    resp->clear();
    target = 5;
  } else {
    target = 2;
  }
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
            resp->append(buf + i, recv_res - i);
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
      resp->append(buf, recv_res);
    status2:
      while (i < resp->size()) {
        window4 = (window4 << 8) + (*resp)[i++];
        switch (window4) {
        case rn_rn:
          status = 3;
          pref = i;
          goto status3;
        case rn_co:
          *(int32_t *)(&(*resp)[i - 4]) = rn_co_pos;
          rn_co_pos = i;
          break;
        case rn_tr:
          *(int32_t *)(&(*resp)[i - 4]) = rn_tr_pos;
          rn_tr_pos = i;
          break;
        default:
          break;
        }
      }
      recv_res = 0;
      break;
    case 3:
    status3 : {
      const char *resp_addr = resp->c_str();
      auto clearChain = [&resp_addr](int i, uint32_t val) {
        while (i) {
          auto pos = (int32_t *)&resp_addr[i - 4];
          i = *pos;
          *pos = val;
        }
      };
      if (ret_code == 204) {
        status = 5;
      } else {
        content_length = 0;
        i = rn_co_pos;
        while (i) {
          if (macroConstStrCmpN(resp_addr + i, "ntent-Length: ") == 0) {
            j = i + int(sizeof("ntent-Length: ")) - 1;
            while (isdigit(resp_addr[j])) {
              content_length = content_length * 10 + resp_addr[j++] - '0';
            }
          }
          auto pos = (int32_t *)&resp_addr[i - 4];
          i = *pos;
        }
        i = rn_tr_pos;
        while (i) {
          printf("%d\n", i);
          auto pos = (int32_t *)&resp_addr[i - 4];
          i = *pos;
          abort();
        }
        if (content_length) {
          recv_rest = content_length - int(resp->size()) + pref;
          status = recv_rest ? 4 : 5;
        } else {
          status = 5;
        }
      }
      clearChain(rn_co_pos, bit_swap32(rn_co));
      clearChain(rn_tr_pos, bit_swap32(rn_tr));
      (*resp)[pref - 2] = 0;
      (*resp)[pref - 1] = 0;
      printf("%s\n", resp_addr);
      recv_res = 0;
      break;
    }
    case 4:
      if (recv_rest < recv_res) {
        abort();
      }
      resp->append(buf, recv_res);
      recv_rest -= recv_res;
      if (recv_rest == 0) {
        status = 0;
      }
      break;
    }
  }
  return recv_res < 0 ? int(recv_res) : ret_code;
}

influxdb_if_inline int http_request(
    const struct sockaddr_in *addr, string_view pref_header, string_view body,
    std::string *resp) {
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
  size_t pos, start = 0;
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

} // namespace detail
} // namespace influx_client

#endif // INFLUXDBM_HTTP_H
