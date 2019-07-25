/*
 * Copyright 2017-2019 Baidu Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "com_baidu_openrasp_NativePatches.h"
#ifdef __linux__
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/ethernet.h>
#include <netdb.h>
#include <netpacket/packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#endif

/*
 * Class:     com_baidu_openrasp_NativePatches
 * Method:    GetNetworkInterfaces
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_baidu_openrasp_NativePatches_GetNetworkInterfaces(JNIEnv* env, jclass cls) {
  static jclass jstrcls = nullptr;
  if (jstrcls == nullptr) {
    auto ref = env->FindClass("java/lang/String");
    jstrcls = (jclass)env->NewGlobalRef(ref);
    env->DeleteLocalRef(ref);
  }
#ifdef __linux__
  struct ifaddrs *ifaddr, *ifa;
  char host[NI_MAXHOST] = {0};
  char macp[INET6_ADDRSTRLEN] = {0};
  std::map<std::string, struct ifaddrs*> inets;
  std::map<std::string, struct ifaddrs*> inet6s;
  std::map<std::string, struct ifaddrs*> packets;
  std::vector<jstring> rst;

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    return NULL;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL) {
      continue;
    }
    if (ifa->ifa_addr->sa_family == AF_PACKET) {
      packets.emplace(ifa->ifa_name, ifa);
    } else if (ifa->ifa_addr->sa_family == AF_INET) {
      inets.emplace(ifa->ifa_name, ifa);
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      inet6s.emplace(ifa->ifa_name, ifa);
    }
  }

  for (auto it_packet : packets) {
    auto it_inet = inets.find(it_packet.first);
    auto it_inet6 = inet6s.find(it_packet.first);
    if (it_inet == inets.end() && it_inet6 == inet6s.end()) {
      continue;
    }
    rst.push_back(env->NewStringUTF(it_packet.first.c_str()));
    struct sockaddr_ll* s = (struct sockaddr_ll*)(it_packet.second->ifa_addr);
    int len = 0;
    for (int i = 0; i < 6; i++) {
      len += sprintf(macp + len, "%02x%s", (unsigned char)s->sll_addr[i], i < 5 ? "-" : "");
    }
    rst.push_back(env->NewStringUTF(macp));
    if (it_inet == inets.end() || getnameinfo(it_inet->second->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                                              NULL, 0, NI_NUMERICHOST) != 0) {
      rst.push_back(NULL);
    } else {
      rst.push_back(env->NewStringUTF(host));
    }
    if (it_inet6 == inet6s.end() || getnameinfo(it_inet6->second->ifa_addr, sizeof(struct sockaddr_in6), host,
                                                NI_MAXHOST, NULL, 0, NI_NUMERICHOST) != 0) {
      rst.push_back(NULL);
    } else {
      rst.push_back(env->NewStringUTF(host));
    }
  }
  freeifaddrs(ifaddr);

  jobjectArray jarr = env->NewObjectArray(rst.size(), jstrcls, nullptr);
  for (int i = 0; i < rst.size(); i++) {
    env->SetObjectArrayElement(jarr, i, rst[i]);
  }
  return jarr;
#else
  return env->NewObjectArray(0, jstrcls, nullptr);
#endif
}