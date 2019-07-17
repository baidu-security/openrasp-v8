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
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
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
  struct ifconf ifc;
  struct ifreq* ifreqP;
  char* buf = nullptr;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    return nullptr;
  }
  // do a dummy SIOCGIFCONF to determine the buffer size
  // SIOCGIFCOUNT doesn't work
  ifc.ifc_buf = nullptr;
  if (ioctl(sock, SIOCGIFCONF, (char*)&ifc) < 0) {
    return nullptr;
  }

  // call SIOCGIFCONF to enumerate the interfaces
  if ((buf = (char*)malloc(ifc.ifc_len)) == nullptr) {
    return nullptr;
  }
  ifc.ifc_buf = buf;
  if (ioctl(sock, SIOCGIFCONF, (char*)&ifc) < 0) {
    free(buf);
    return nullptr;
  }

  std::vector<std::string> list;
  // iterate through each interface
  ifreqP = ifc.ifc_req;
  for (int i = 0; i < ifc.ifc_len / sizeof(struct ifreq); i++, ifreqP++) {
    // ignore non IPv4 addresses
    if (ifreqP->ifr_addr.sa_family != AF_INET) {
      continue;
    }
    list.emplace_back(ifreqP->ifr_name);
    ioctl(sock, SIOCGIFADDR, ifreqP);
    list.emplace_back(inet_ntoa(((struct sockaddr_in*)&(ifreqP->ifr_addr))->sin_addr));
    ioctl(sock, SIOCGIFHWADDR, ifreqP);
    char tmp[30] = {0};
    unsigned char* ptr = (unsigned char*)ifreqP->ifr_addr.sa_data;
    sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
    list.emplace_back(tmp);
  }

  jobjectArray rst = env->NewObjectArray(list.size(), jstrcls, nullptr);
  for (int i = 0; i < list.size(); i++) {
    env->SetObjectArrayElement(rst, i, env->NewStringUTF(list[i].c_str()));
  }

  // free buffer
  free(buf);
  close(sock);
  return rst;
#else
  return env->NewObjectArray(0, jstrcls, nullptr);
#endif
}